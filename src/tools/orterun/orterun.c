/* -*- C -*-
 *
 * Copyright (c) 2004-2005 The Trustees of Indiana University.
 *                         All rights reserved.
 * Copyright (c) 2004-2005 The Trustees of the University of Tennessee.
 *                         All rights reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

#include "orte_config.h"

#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <errno.h>
#include <signal.h>

#include "include/orte_constants.h"

#include "event/event.h"
#include "util/proc_info.h"
#include "util/argv.h"
#include "util/path.h"
#include "util/cmd_line.h"
#include "util/sys_info.h"
#include "util/output.h"
#include "util/universe_setup_file_io.h"
#include "util/show_help.h"
#include "threads/condition.h"

#include "mca/base/base.h"
#include "mca/ns/ns.h"
#include "mca/gpr/gpr.h"
#include "mca/rmgr/rmgr.h"

#include "runtime/runtime.h"
#include "runtime/orte_wait.h"

extern char** environ;

struct ompi_event term_handler;
struct ompi_event int_handler;
orte_jobid_t jobid = ORTE_JOBID_MAX;

static int create_app(int argc, char* argv[], orte_app_context_t **app);


static void exit_callback(int fd, short event, void *arg)
{
    fprintf(stderr, "orterun: abnormal exit\n");
    exit(1);
}

static void signal_callback(int fd, short flags, void *arg)
{
    int ret;
    struct timeval tv = { 5, 0 };
    ompi_event_t* event;

    static int signalled = 0;
    if (0 != signalled++) {
         return;
    }

    if (jobid != ORTE_JOBID_MAX) {
        ret = orte_rmgr.terminate_job(jobid);
        if (ORTE_SUCCESS != ret) {
            jobid = ORTE_JOBID_MAX;
        }
    }

    if (NULL != (event = (ompi_event_t*)malloc(sizeof(ompi_event_t)))) {
        ompi_evtimer_set(event, exit_callback, NULL);
        ompi_evtimer_add(event, &tv);
    }
}


/*
 * setup globals for catching orterun command line options
 */
struct {
    bool help;
    bool version;
    int num_procs;
    char *hostfile;
    char *env_val;
    char *wd;
} orterun_globals;

/*
 * define the orterun context table for obtaining parameters
 */
orte_context_value_names_t orterun_context_tbl[] = {
    /* start with usual help and version stuff */
    {{NULL, NULL, NULL}, "help", 0, ORTE_BOOL, (void*)&orterun_globals.help, (void*)false, NULL},
    {{NULL, NULL, NULL}, "version", 0, ORTE_BOOL, (void*)&orterun_globals.version, (void*)false, NULL},
    {{NULL, NULL, NULL}, "n", 1, ORTE_INT, (void*)&orterun_globals.num_procs, (void*)1, NULL},
    {{NULL, NULL, NULL}, "np", 1, ORTE_INT, (void*)&orterun_globals.num_procs, (void*)1, NULL},
    {{"hostfile", NULL, NULL}, "hostfile", 1, ORTE_STRING, (void*)&(orterun_globals.hostfile), NULL, NULL},
    {{NULL, NULL, NULL}, "x", 1, ORTE_STRING, (void*)&(orterun_globals.env_val), NULL, NULL},
    {{NULL, NULL, NULL}, "wd", 1, ORTE_STRING, (void*)&(orterun_globals.wd), NULL, NULL},
    {{NULL, NULL, NULL}, NULL, 0, ORTE_NULL, NULL, NULL, NULL} /* terminate the table */
};


int main(int argc, char *argv[], char* env[])
{
    ompi_cmd_line_t cmd_line;
    orte_app_context_t **apps;
    int rc, i, temp_argc, num_apps, app_num;
    char **temp_argv;

    /* Intialize our Open RTE environment */

    OBJ_CONSTRUCT(&cmd_line, ompi_cmd_line_t);
    if (ORTE_SUCCESS != (rc = orte_init(&cmd_line, argc, argv))) {
        ompi_show_help("help-orterun.txt", "orterun:init-failure", true,
                       "orte_init()", rc);
        return rc;
    }
    OBJ_DESTRUCT(&cmd_line);

     /* Prep to start the application */

    ompi_event_set(&term_handler, SIGTERM, OMPI_EV_SIGNAL,
                   signal_callback, NULL);
    ompi_event_add(&term_handler, NULL);
    ompi_event_set(&int_handler, SIGINT, OMPI_EV_SIGNAL,
                   signal_callback, NULL);
    ompi_event_add(&int_handler, NULL);

    /* Count how many apps we're going to have */

    for (num_apps = 1, i = 0; i < argc; ++i) {
        if (0 == strcmp(argv[i], ":")) {
            ++num_apps;
        }
    }
    apps = malloc(sizeof(orte_app_context_t **) * num_apps);
    if (NULL == apps) {
        /* JMS show_help */
        return ORTE_ERR_OUT_OF_RESOURCE;
    }

    /* Make the apps */

    temp_argc = 0;
    temp_argv = NULL;
    ompi_argv_append(&temp_argc, &temp_argv, argv[0]);
    
    for (app_num = 0, i = 1; i < argc; ++i) {
        if (0 == strcmp(argv[i], ":")) {
            
            /* Make an app with this argv */

            if (ompi_argv_count(temp_argv) > 1) {
                create_app(temp_argc, temp_argv, &(apps[app_num++]));
            
                /* Reset the temps */
            
                temp_argc = 0;
                temp_argv = NULL;
                ompi_argv_append(&temp_argc, &temp_argv, argv[0]);
            } else {
                --num_apps;
            }
        } else {
            ompi_argv_append(&temp_argc, &temp_argv, argv[i]);
        }
    }
    if (ompi_argv_count(temp_argv) > 1) {
        create_app(temp_argc, temp_argv, &(apps[app_num]));
    } else {
        ompi_argv_free(temp_argv);
        --num_apps;
    }

    /* Spawn the job */

    rc = orte_rmgr.spawn(apps, num_apps, &jobid, NULL);
    if (ORTE_SUCCESS != rc) {
        /* JMS show_help */
        ompi_output(0, "orterun: spawn failed with errno=%d\n", rc);
    }

    /* All done */

    for (i = 0; i < num_apps; ++i) {
        OBJ_RELEASE(apps[i]);
    }
    free(apps);
    orte_finalize();
    return rc;
}


static int create_app(int argc, char* argv[], orte_app_context_t **app_ptr)
{
    ompi_cmd_line_t cmd_line;
    char cwd[OMPI_PATH_MAX];
    int i, rc;
    char *param, *value, *value2;
    orte_app_context_t *app;
    extern char **environ;

    /* Parse application command line options. */

    app = OBJ_NEW(orte_app_context_t);
    OBJ_CONSTRUCT(&cmd_line, ompi_cmd_line_t);

    /* parse my context */
    if (ORTE_SUCCESS != (rc = orte_parse_context(orterun_context_tbl, &cmd_line, argc, argv))) {
        OBJ_RELEASE(app);
        return rc;
    }
    
    /* check for help and version requests */
    if (orterun_globals.help) {
        char *args = NULL;
        args = ompi_cmd_line_get_usage_msg(&cmd_line);
        ompi_show_help("help-orterun.txt", "orterun:usage", false,
                       argv[0], args);
        free(args);
        OBJ_RELEASE(app);
        return ORTE_SUCCESS;
    }

    if (orterun_globals.version) {
        printf("Open MPI v%s\n", OMPI_VERSION);
        OBJ_RELEASE(app);
        return ORTE_SUCCESS;
    }

    /* Setup application context */

    ompi_cmd_line_get_tail(&cmd_line, &app->argc, &app->argv);
    if (0 == app->argc) {
        ompi_show_help("help-orterun.txt", "orterun:no-application", true, argv[0], argv[0]);
        OBJ_RELEASE(app);
        return ORTE_ERR_NOT_FOUND;
    }

    /* Did the user request to export any environment variables? */

    app->env = NULL;
    app->num_env = 0;
    if (ompi_cmd_line_is_taken(&cmd_line, "x")) {
        for (i = 0; i < ompi_cmd_line_get_ninsts(&cmd_line, "x"); ++i) {
            param = ompi_cmd_line_get_param(&cmd_line, "x", i, 0);

            if (NULL != strchr(param, '=')) {
                ompi_argv_append(&app->num_env, &app->env, param);
            } else {
                value = getenv(param);
                if (NULL != value) {
                    if (NULL != strchr(value, '=')) {
                        ompi_argv_append(&app->num_env, &app->env, value);
                    } else {
                        asprintf(&value2, "%s=%s", param, value);
                        ompi_argv_append(&app->num_env, &app->env, value2);
                    }
                } else {
                    ompi_output(0, "Warning: could not find environment variable \"%s\"\n", param);
                }
            }
            free(param);
        }
    }

    /* What cwd do we want? */

    if (NULL != orterun_globals.wd) {
        app->cwd = strdup(orterun_globals.wd);
    } else {
        getcwd(cwd, sizeof(cwd));
        app->cwd = strdup(cwd);
    }

    /* Get the numprocs */

    app->num_procs = orterun_globals.num_procs;
    if (0 == app->num_procs) {
        app->num_procs = 1; 
    }

    /* Find the argv[0] in the path */

    app->app = ompi_path_findv(app->argv[0], 0, environ, app->cwd); 
    if (NULL == app->app) {
        ompi_show_help("help-orterun.txt", "orterun:no-application", true, argv[0], argv[0]);
        OBJ_RELEASE(app);
        return ORTE_ERR_NOT_FOUND;
    }

    /* All done */

    *app_ptr = app;
    OBJ_DESTRUCT(&cmd_line);
    return rc;
}


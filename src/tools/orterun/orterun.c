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
struct ompi_event exit_handler;
orte_jobid_t jobid = ORTE_JOBID_MAX;


static void
exit_callback(int fd, short event, void *arg)
{
    printf("we failed to exit cleanly :(\n");
    exit(1);
}

static void
signal_callback(int fd, short event, void *arg)
{
    int ret;
    struct timeval tv;

    if (jobid != ORTE_JOBID_MAX) {
        ret = orte_rmgr.terminate_job(jobid);
        if (ORTE_SUCCESS != ret) {
            jobid = ORTE_JOBID_MAX;
        }
    }

    tv.tv_sec = 3;
    tv.tv_usec = 0;
    ompi_evtimer_set(&exit_handler, exit_callback, NULL);
    ompi_evtimer_add(&exit_handler, &tv);
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
    {{NULL, NULL, NULL}, "np", 1, ORTE_INT, (void*)&orterun_globals.num_procs, (void*)0, NULL},
    {{"hostfile", NULL, NULL}, "hostfile", 1, ORTE_STRING, (void*)&(orterun_globals.hostfile), NULL, NULL},
    {{NULL, NULL, NULL}, "x", 1, ORTE_STRING, (void*)&(orterun_globals.env_val), NULL, NULL},
    {{NULL, NULL, NULL}, "wd", 1, ORTE_STRING, (void*)&(orterun_globals.wd), NULL, NULL},
    {{NULL, NULL, NULL}, NULL, 0, ORTE_NULL, NULL, NULL, NULL} /* terminate the table */
};


int
main(int argc, char *argv[], char* env[])
{
    orte_app_context_t app;
    orte_app_context_t *apps[1];
    ompi_cmd_line_t cmd_line;
    char cwd[OMPI_PATH_MAX];
    int i, rc;
    char *param, *value, *value2;

    /* Parse application command line options. */
    OBJ_CONSTRUCT(&app, orte_app_context_t);
    OBJ_CONSTRUCT(&cmd_line, ompi_cmd_line_t);

    /* parse my context */
    if (ORTE_SUCCESS != (rc = orte_parse_context(orterun_context_tbl, &cmd_line, argc, argv))) {
        return rc;
    }
    
    /* check for help and version requests */
    if (orterun_globals.help) {
        char *args = NULL;
        args = ompi_cmd_line_get_usage_msg(&cmd_line);
        ompi_show_help("help-orterun.txt", "orterun:usage", false,
                       argv[0], args);
        free(args);
        return 1;
    }

    if (orterun_globals.version) {
        /* show version message */
        printf("...showing off my version!\n");
        exit(1);
    }

    /* Intialize our Open RTE environment */

    if (ORTE_SUCCESS != (rc = orte_init(&cmd_line, argc, argv))) {
        ompi_show_help("help-orterun.txt", "orterun:init-failure", true,
                       "orte_init()", rc);
        return rc;
    }

     /* Prep to start the application */

    ompi_event_set(&term_handler, SIGTERM, OMPI_EV_SIGNAL,
                   signal_callback, NULL);
    ompi_event_add(&term_handler, NULL);
    ompi_event_set(&int_handler, SIGINT, OMPI_EV_SIGNAL,
                   signal_callback, NULL);
    ompi_event_add(&int_handler, NULL);

    /* Setup application context */

    apps[0] = &app;
    ompi_cmd_line_get_tail(&cmd_line, &app.argc, &app.argv);

    if(app.argc == 0) {
        ompi_show_help("help-orterun.txt", "orterun:no-application", true, argv[0], argv[0]);
        return 1;
    }

    /* Did the user request to export any environment variables? */

    app.env = NULL;
    app.num_env = 0;
    if (ompi_cmd_line_is_taken(&cmd_line, "x")) {
        for (i = 0; i < ompi_cmd_line_get_ninsts(&cmd_line, "x"); ++i) {
            param = ompi_cmd_line_get_param(&cmd_line, "x", i, 0);

            if (NULL != strchr(param, '=')) {
                ompi_argv_append(&app.num_env, &app.env, param);
            } else {
                value = getenv(param);
                if (NULL != value) {
                    if (NULL != strchr(value, '=')) {
                        ompi_argv_append(&app.num_env, &app.env, value);
                    } else {
                        asprintf(&value2, "%s=%s", param, value);
                        ompi_argv_append(&app.num_env, &app.env, value2);
                    }
                } else {
                    fprintf(stderr, "Warning: could not find environment variable \"%s\"\n", param);
                }
            }
            free(param);
        }
    }

    /* What cwd do we want? */

    if (NULL != orterun_globals.wd) {
        app.cwd = strdup(orterun_globals.wd);
    } else {
        getcwd(cwd, sizeof(cwd));
        app.cwd = strdup(cwd);
    }

    /* Get the numprocs */

    app.num_procs = orterun_globals.num_procs;

    /* Find the argv[0] in the path */

    app.app = ompi_path_findv(app.argv[0], 0, env, app.cwd); 
    if(NULL == app.app) {
        ompi_show_help("help-orterun.txt", "orterun:no-application", true, argv[0], argv[0]);
        return 1;
    }

    /* spawn it */

    rc = orte_rmgr.spawn(apps, 1, &jobid, NULL);
    if(ORTE_SUCCESS != rc) {
        ompi_output(0, "orterun: spawn failed with errno=%d\n", rc);
    }

    /* All done */

    orte_finalize();
    OBJ_DESTRUCT(&app);
    OBJ_DESTRUCT(&cmd_line);
    return rc;
}


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

/* enable orterun to block until app completes */
ompi_condition_t orterun_cond;
ompi_mutex_t orterun_lock;
bool orterun_complete = false;


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



int
main(int argc, char *argv[], char* env[])
{
    orte_app_context_t app;
    orte_app_context_t *apps[1];
    ompi_cmd_line_t cmd_line;
    char cwd[OMPI_PATH_MAX];
    int rc;


    /*
     * Parse application command line options.
     */
    OBJ_CONSTRUCT(&app, orte_app_context_t);
    OBJ_CONSTRUCT(&cmd_line, ompi_cmd_line_t);

    ompi_cmd_line_make_opt(&cmd_line, 'v', "version", 0,
			   "Show version of Open MPI and this program");
    ompi_cmd_line_make_opt(&cmd_line, 'h', "help", 0,
			   "Show help for this function");
    ompi_cmd_line_make_opt3(&cmd_line, 'n', "np", "np", 1,
                            "Number of processes to start");
    ompi_cmd_line_make_opt3(&cmd_line, '\0', "hostfile", "hostfile", 1,
			    "Host description file");

    if (ORTE_SUCCESS != ompi_cmd_line_parse(&cmd_line, true, argc, argv)) {
        char *args = NULL;
        args = ompi_cmd_line_get_usage_msg(&cmd_line);
        ompi_show_help("help-orterun.txt", "orterun:usage", false,
                       argv[0], args);
        free(args);
        return 1;
    }

    if (ompi_cmd_line_is_taken(&cmd_line, "help") || 
        ompi_cmd_line_is_taken(&cmd_line, "h")) {
        char *args = NULL;
        args = ompi_cmd_line_get_usage_msg(&cmd_line);
        ompi_show_help("help-orterun.txt", "orterun:usage", false,
                       argv[0], args);
        free(args);
        return 1;
    }

    if (ompi_cmd_line_is_taken(&cmd_line, "version") ||
	ompi_cmd_line_is_taken(&cmd_line, "v")) {
	printf("...showing off my version!\n");
	return 1;
    }

#if 0
    /* get our hostfile, if we have one */
    if (ompi_cmd_line_is_taken(&cmd_line, "hostfile")) {
        /* BWB - XXX - fix me.  We really should be setting this via
         * an API rather than setenv.  But we don't have such an API just
         * yet. */
        char *buf = NULL;
        asprintf(&buf, "OMPI_MCA_hostfile=%s", 
                 ompi_cmd_line_get_param(&cmd_line, "hostfile", 0, 0));
        /* yeah, it leaks.  Can't do nothin' about that */
        putenv(buf);
   }
#endif

    /* get our numprocs */
    if (ompi_cmd_line_is_taken(&cmd_line, "np")) {
        app.num_procs = atoi(ompi_cmd_line_get_param(&cmd_line, "np", 0, 0));
    } else {
        app.num_procs = 1;
    }

    /*
     * Intialize our Open RTE environment
     */

    if (ORTE_SUCCESS != (rc = orte_init(&cmd_line, argc, argv))) {
        ompi_show_help("help-orterun.txt", "orterun:init-failure", true,
                       "orte_init()", rc);
	return rc;
    }


    /*****    PREP TO START THE APPLICATION    *****/
    ompi_event_set(&term_handler, SIGTERM, OMPI_EV_SIGNAL,
                   signal_callback, NULL);
    ompi_event_add(&term_handler, NULL);
    ompi_event_set(&int_handler, SIGINT, OMPI_EV_SIGNAL,
                   signal_callback, NULL);
    ompi_event_add(&int_handler, NULL);

    /* 
     * Setup application context 
     */
    apps[0] = &app;
    ompi_cmd_line_get_tail(&cmd_line, &app.argc, &app.argv);

    if(app.argc == 0) {
        ompi_show_help("help-orterun.txt", "orterun:no-application", true, argv[0], argv[0]);
        return 1;
    }
    app.env = NULL;
    app.num_env = 0;
    getcwd(cwd,sizeof(cwd));
    app.cwd = strdup(cwd);
    app.app = ompi_path_findv(app.argv[0], 0, env, app.cwd); 
 
    if(NULL == app.app) {
        ompi_show_help("help-orterun.txt", "orterun:no-application", true, argv[0], argv[0]);
        return 1;
    }


    /* setup to block until app completes */
    OBJ_CONSTRUCT(&orterun_lock, ompi_mutex_t);
    OBJ_CONSTRUCT(&orterun_cond, ompi_condition_t);

    /* spawn it */
    rc = orte_rmgr.spawn(apps, 1, &jobid, NULL);
    if(ORTE_SUCCESS != rc) {
        ompi_output(0, "orterun: spawn failed with errno=%d\n", rc);
    } else {
#if 0
        /* block until completion */
        OMPI_THREAD_LOCK(&orterun_lock);
        while(orterun_complete == false) {
            ompi_condition_wait(&orterun_cond, &orterun_lock);
        }
        OMPI_THREAD_UNLOCK(&orterun_lock);
#endif
        orte_finalize();
    }

    OBJ_DESTRUCT(&orterun_lock);
    OBJ_DESTRUCT(&orterun_cond);
    OBJ_DESTRUCT(&app);
    OBJ_DESTRUCT(&cmd_line);
    return rc;
}


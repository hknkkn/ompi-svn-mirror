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
#include "util/cmd_line.h"
#include "util/sys_info.h"
#include "util/output.h"
#include "util/universe_setup_file_io.h"
#include "util/show_help.h"

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



int
main(int argc, char *argv[])
{
    ompi_cmd_line_t *cmd_line = NULL;
    int num_procs = 1;
    char cwd[MAXPATHLEN];
    orte_app_context_t app;
    int ret;

    /*
     * Parse application specific command line options.
     */

    cmd_line = OBJ_NEW(ompi_cmd_line_t);
    ompi_cmd_line_make_opt(cmd_line, 'v', "version", 0,
			   "Show version of Open MPI and this program");
    ompi_cmd_line_make_opt(cmd_line, 'h', "help", 0,
			   "Show help for this function");
    ompi_cmd_line_make_opt3(cmd_line, 'n', "np", "np", 1,
                            "Number of processes to start");
    ompi_cmd_line_make_opt3(cmd_line, '\0', "hostfile", "hostfile", 1,
			    "Host description file");

    if (ORTE_SUCCESS != ompi_cmd_line_parse(cmd_line, true, argc, argv)) {
        char *args = NULL;
        args = ompi_cmd_line_get_usage_msg(cmd_line);
        ompi_show_help("help-orterun.txt", "orterun:usage", false,
                       argv[0], args);
        free(args);
        return 1;
    }

    if (ompi_cmd_line_is_taken(cmd_line, "help") || 
        ompi_cmd_line_is_taken(cmd_line, "h")) {
        char *args = NULL;
        args = ompi_cmd_line_get_usage_msg(cmd_line);
        ompi_show_help("help-orterun.txt", "orterun:usage", false,
                       argv[0], args);
        free(args);
        return 1;
    }

    if (ompi_cmd_line_is_taken(cmd_line, "version") ||
	ompi_cmd_line_is_taken(cmd_line, "v")) {
	printf("...showing off my version!\n");
	return 1;
    }

#if 0
    /* get our hostfile, if we have one */
    if (ompi_cmd_line_is_taken(cmd_line, "hostfile")) {
        /* BWB - XXX - fix me.  We really should be setting this via
         * an API rather than setenv.  But we don't have such an API just
         * yet. */
        char *buf = NULL;
        asprintf(&buf, "OMPI_MCA_hostfile=%s", 
                 ompi_cmd_line_get_param(cmd_line, "hostfile", 0, 0));
        /* yeah, it leaks.  Can't do nothin' about that */
        putenv(buf);
   }
#endif

    /* get our numprocs */
    if (ompi_cmd_line_is_taken(cmd_line, "np")) {
        num_procs = atoi(ompi_cmd_line_get_param(cmd_line, "np", 0, 0));
    }

    /*
     * Intialize our Open RTE environment
     */

    if (ORTE_SUCCESS != orte_init(cmd_line)) {
        ompi_show_help("help-orterun.txt", "orterun:init-failure", true,
                       "orte_init()", ret);
	return ret;
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
    OBJ_CONSTRUCT(&app, orte_app_context_t);
    ompi_cmd_line_get_tail(cmd_line, &app.argc, &app.argv);

    if(app.argc == 0) {
        ompi_show_help("help-orterun.txt", "orterun:no-application", true, argv[0], argv[0]);
        return 1;
    }
    return 0;
    
#if 0

    /*
     * build environment to be passed
     */

    mca_pcm_base_build_base_env(environ, &(sched->envc), &(sched->env));
    /* set initial contact info */
    if (ompi_process_info.seed) {  /* i'm the seed - direct them towards me */
	my_contact_info = mca_oob_get_contact_info();
    } else { /* i'm not the seed - direct them to it */
	my_contact_info = strdup(ompi_universe_info.ns_replica);
    }
    asprintf(&tmp, "OMPI_MCA_ns_base_replica=%s", my_contact_info);
    ompi_argv_append(&(sched->envc), &(sched->env), tmp);
    free(tmp);
    asprintf(&tmp, "OMPI_MCA_gpr_base_replica=%s", my_contact_info);
    ompi_argv_append(&(sched->envc), &(sched->env), tmp);
    free(tmp);
    if (NULL != ompi_universe_info.name) {
	asprintf(&tmp, "OMPI_universe_name=%s", ompi_universe_info.name);
	ompi_argv_append(&(sched->envc), &(sched->env), tmp);
	free(tmp);
    }
    if (ompi_cmd_line_is_taken(cmd_line, "tmpdir")) {  /* user specified the tmp dir base */
	asprintf(&tmp, "OMPI_tmpdir_base=%s", ompi_cmd_line_get_param(cmd_line, "tmpdir", 0, 0));
	ompi_argv_append(&(sched->envc), &(sched->env), tmp);
	free(tmp);
    }

    getcwd(cwd, MAXPATHLEN);
    sched->cwd = strdup(cwd);
    sched->nodelist = nodelist;

    if (sched->argc == 0) {
        ompi_show_help("help-orterun.txt", "orterun:no-application", true,
                       argv[0], argv[0]);
	return 1;
    }

#endif
}


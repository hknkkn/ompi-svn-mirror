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
#include "util/session_dir.h"
#include "util/output.h"
#include "util/os_path.h"
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
orte_jobid_t new_jobid = ORTE_JOBID_MAX;

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

    if (new_jobid != ORTE_JOBID_MAX) {
        ret = orte_rmgr.terminate_job(new_jobid);
        if (OMPI_SUCCESS != ret) {
            new_jobid = ORTE_JOBID_MAX;
        }
    }
#if 0
    tv.tv_sec = 3;
    tv.tv_usec = 0;
    ompi_evtimer_set(&exit_handler, exit_callback, NULL);
    ompi_evtimer_add(&exit_handler, &tv);
#endif
}

/*
 * setup globals for catching mpirun command line options
 */
struct {
    bool help;
    bool version;
    int num_procs;
    char *hostfile;
    char *env;
    char *wd;
} mpirun_globals;

/*
 * define the mpirun context table for obtaining parameters
 */
orte_context_value_names_t mpirun_context_tbl[] = {
    /* start with usual help and version stuff */
    {{NULL, NULL, NULL}, "help", 0, ORTE_BOOL, (void*)&mpirun_globals.help, (void*)false, NULL},
    {{NULL, NULL, NULL}, "version", 0, ORTE_BOOL, (void*)&mpirun_globals.version, (void*)false, NULL},
    {{NULL, NULL, NULL}, "np", 1, ORTE_INT, (void*)&mpirun_globals.num_procs, (void*)0, NULL},
    {{"hostfile", NULL, NULL}, "hostfile", 1, ORTE_STRING, (void*)&(mpirun_globals.hostfile), NULL, NULL},
    {{NULL, NULL, NULL}, "x", 1, ORTE_STRING, (void*)&(mpirun_globals.env), NULL, NULL},
    {{NULL, NULL, NULL}, "wd", 1, ORTE_STRING, (void*)&(mpirun_globals.wd), NULL, NULL},
    {{NULL, NULL, NULL}, NULL, 0, ORTE_NULL, NULL, NULL, NULL} /* terminate the table */
};

int
main(int argc, char *argv[])
{
#if 0
    bool multi_thread = false;
    bool hidden_thread = false;
    int ret;
    ompi_cmd_line_t *cmd_line = NULL;
    ompi_list_t *nodelist = NULL;
    ompi_list_t schedlist;
    int num_procs = 1;
    char cwd[MAXPATHLEN];
    char *my_contact_info, *tmp, *jobidstring;
    char *contact_file, *filenm, *segment;
    ompi_registry_notify_id_t rc_tag;
    ompi_rte_process_status_t *proc_status;
    ompi_list_t *status_list;
    ompi_registry_value_t *value;
        


    /* setup to check command line options */
    cmd_line = OBJ_NEW(ompi_cmd_line_t);

    /* parse my context */
    if (ORTE_SUCCESS != (ret = orte_parse_context(mpirun_context_tbl, cmd_line, argc, argv))) {
        return ret;
    }
    
    /* check for help and version requests */
    if (mpirun_globals.help) {
        char *args = NULL;
        args = ompi_cmd_line_get_usage_msg(cmd_line);
        ompi_show_help("help-mpirun.txt", "mpirun:usage", false,
                       argv[0], args);
        free(args);
        return 1;
    }

    if (mpirun_globals.version) {
        /* show version message */
        printf("...showing off my version!\n");
        exit(1);
    }

    /*
     * Start the Open Run Time Environment
     */
    if (ORTE_SUCCESS != (ret = orte_init(cmd_line, argc, argv))) {
        ompi_show_help("help-mpirun.txt", "mpirun:init-failure", true,
                       "orte_init()", ret);
	   return ret;
    }

    /* Finish setting up the RTE - contains commands
     * that need to be inside a compound command, if one is active
     */
    if (OMPI_SUCCESS != (ret = orte_init_complete())) {
        fprintf(stderr, "failed in orte_init_complete");
	    return ret;
    }

    /* if i'm the seed, get my contact info and write my setup file for others to find */
    if (orte_process_info.seed) {
        if (NULL != orte_universe_info.seed_uri) {
         free(orte_universe_info.seed_uri);
         orte_universe_info.seed_uri = NULL;
        }
        orte_universe_info.seed_uri = orte_rml.get_uri();
        contact_file = orte_os_path(false, orte_process_info.universe_session_dir,
                 "universe-setup.txt", NULL);

        if (ORTE_SUCCESS != (ret = orte_write_universe_setup_file(contact_file))) {
            if (mpirun_globals.debug) {
                ompi_output(0, "mpirun: couldn't write setup file");
          }
      } else if (mpirun_globals.debug) {
          ompi_output(0, "mpirun: wrote setup file");
       }
    }


    /*****    PREP TO START THE APPLICATION    *****/
    ompi_event_set(&term_handler, SIGTERM, OMPI_EV_SIGNAL,
                   signal_callback, NULL);
    ompi_event_add(&term_handler, NULL);
    ompi_event_set(&int_handler, SIGINT, OMPI_EV_SIGNAL,
                   signal_callback, NULL);
    ompi_event_add(&int_handler, NULL);

    /* discover resources */
    if (ORTE_SUCCESS != (ret = orte_rmgr.query())) {
        ORTE_ERROR_LOG(ret);
        goto CLEANUP;
    }
    
    /* setup the jobid for the application */
    if (ORTE_SUCCESS != (ret = orte_rmgr.create(&new_jobid))) {
        ORTE_ERROR_LOG(ret);
        orte_finalize();
        return ret;
    }

    /* allocate resources to this job */
    if (ORTE_SUCCESS != (ret = orte_rmgr.query())) {
        ORTE_ERROR_LOG(ret);
        goto CLEANUP;
    }

    /*
     * Process mapping
     */
    OBJ_CONSTRUCT(&schedlist,  ompi_list_t);
    sched = OBJ_NEW(ompi_rte_node_schedule_t);
    ompi_list_append(&schedlist, (ompi_list_item_t*) sched);
    ompi_cmd_line_get_tail(cmd_line, &(sched->argc), &(sched->argv));

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
        ompi_show_help("help-mpirun.txt", "mpirun:no-application", true,
                       argv[0], argv[0]);
	return 1;
    }


    /*
     * register to monitor the startup and shutdown processes
     */
    /* setup segment for this job */
    if (ORTE_SUCCESS != orte_name_services.convert_jobid_to_string(jobidstring, new_jobid)) {
        return ORTE_ERROR;
    }
    asprintf(&segment, "%s-%s", OMPI_RTE_JOB_STATUS_SEGMENT, jobidstring);

    /* register a synchro on the segment so we get notified when everyone registers */
    rc_tag = ompi_registry.synchro(
	     OMPI_REGISTRY_SYNCHRO_MODE_LEVEL|OMPI_REGISTRY_SYNCHRO_MODE_ONE_SHOT|
	     OMPI_REGISTRY_SYNCHRO_MODE_STARTUP,
	     OMPI_REGISTRY_OR,
	     segment,
	     NULL,
	     num_procs,
	     ompi_rte_all_procs_registered, NULL);
    /* register a synchro on the segment so we get notified when everyone is gone
     */
    rc_tag = ompi_registry.synchro(
         OMPI_REGISTRY_SYNCHRO_MODE_DESCENDING|OMPI_REGISTRY_SYNCHRO_MODE_ONE_SHOT|
         OMPI_REGISTRY_SYNCHRO_MODE_STARTUP,
	     OMPI_REGISTRY_OR,
	     segment,
	     NULL,
	     0,
	     ompi_rte_all_procs_unregistered, NULL);

    /*
     * spawn procs
     */
    if (OMPI_SUCCESS != (ret = ompi_rte_spawn_procs(spawn_handle, new_jobid, &schedlist))) {
        ompi_show_help("help-mpirun.txt", "mpirun:error-spawning",
                       true, argv[0], ret);
	return 1;
    }
    
   
    if (OMPI_SUCCESS != (ret = ompi_rte_monitor_procs_registered())) {
        ompi_show_help("help-mpirun.txt", "mpirun:proc-reg-failed", 
                       true, argv[0], ret);
	ompi_rte_job_shutdown(new_jobid);
	return -1;
    } else {
	ompi_rte_job_startup(new_jobid);
	ompi_rte_monitor_procs_unregistered();
    }

    /* remove signal handler */
    ompi_event_del(&term_handler);
    ompi_event_del(&int_handler);

    /*
     * Determine if the processes all exited normally - if not, flag the output of mpirun
     */
    ret = 0;
    status_list = ompi_registry.get(OMPI_REGISTRY_OR, segment, NULL);
    while (NULL != (value = (ompi_registry_value_t*)ompi_list_remove_first(status_list))) {
	proc_status = ompi_rte_unpack_process_status(value);
	if (OMPI_PROC_TERMINATING != proc_status->status_key) {
	    ret = -1;
	}
	if (0 != proc_status->exit_code) {
	    ret = proc_status->exit_code;
	}
    }

    /*
     * Clean up
     */
    if (NULL != nodelist) ompi_rte_deallocate_resources(spawn_handle, new_jobid, nodelist);
    if (NULL != cmd_line) OBJ_RELEASE(cmd_line);
    if (NULL != spawn_handle) OBJ_RELEASE(spawn_handle);

    /* eventually, mpirun won't be the seed and so won't have to do this.
     * for now, though, remove the universe-setup.txt file so the directories
     * can cleanup
     */
    if (ompi_process_info.seed) {
	filenm = ompi_os_path(false, ompi_process_info.universe_session_dir, "universe-setup.txt", NULL);
	unlink(filenm);
    }

    OBJ_DESTRUCT(&schedlist);

    ompi_rte_finalize();
    mca_base_close();
    ompi_finalize();

    return ret;
#endif
    return 0;
}


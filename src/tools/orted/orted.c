/*
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
/** @file **/

#include "orte_config.h"

#include <stdio.h>
#include <ctype.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <errno.h>

#include "include/orte_constants.h"

#include "threads/mutex.h"
#include "threads/condition.h"

#include "dps/dps.h"
#include "util/output.h"
#include "util/show_help.h"
#include "util/sys_info.h"
#include "util/os_path.h"
#include "util/cmd_line.h"
#include "util/proc_info.h"
#include "util/session_dir.h"
#include "util/printf.h"
#include "util/daemon_init.h"
#include "util/universe_setup_file_io.h"

#include "mca/base/base.h"
#include "mca/errmgr/errmgr.h"
#include "mca/ns/ns.h"
#include "mca/gpr/gpr.h"
#include "mca/rml/rml.h"
#include "mca/soh/soh.h"

#include "runtime/runtime.h"

#include "tools/orted/orted.h"

orted_globals_t orted_globals;

static void orte_daemon_recv(int status, orte_process_name_t* sender,
			     orte_buffer_t *buffer, orte_rml_tag_t tag,
			     void* cbdata);


int main(int argc, char *argv[])
{
    int ret = 0;
    ompi_cmd_line_t *cmd_line = NULL;
    char *contact_file;
    char *filenm, *segment;

    /* setup to check common command line options that just report and die */
    cmd_line = OBJ_NEW(ompi_cmd_line_t);

    ompi_cmd_line_make_opt(cmd_line, 'v', "version", 0,
            "Show version of Open MPI and this program");

    ompi_cmd_line_make_opt(cmd_line, 'h', "help", 0,
            "Show help for this function");


    /* parse the local commands */
    if (OMPI_SUCCESS != ompi_cmd_line_parse(cmd_line, true, argc, argv)) {
        char *args = NULL;
        args = ompi_cmd_line_get_usage_msg(cmd_line);
        ompi_show_help("help-orted.txt", "orted:usage", false,
                       argv[0], args);
        free(args);
        return 1;
    }

    /* check for help and version requests */
    if (ompi_cmd_line_is_taken(cmd_line, "help") || 
        ompi_cmd_line_is_taken(cmd_line, "h")) {
        char *args = NULL;
        args = ompi_cmd_line_get_usage_msg(cmd_line);
        ompi_show_help("help-orted.txt", "orted:usage", false,
                       argv[0], args);
        free(args);
        return 1;
    }

    if (ompi_cmd_line_is_taken(cmd_line, "version") ||
        ompi_cmd_line_is_taken(cmd_line, "v")) {
        /* BWB - show version message */
        printf("...showing off my version!\n");
        exit(1);
    }

   /* check for debug flag */
    if (0 > (ret = mca_base_param_register_int("daemon","debug", NULL, NULL, false))) {
        ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
        return ORTE_ERR_BAD_PARAM;
    }
    if (ORTE_SUCCESS != (ret = mca_base_param_lookup_int(ret, (int*)&orted_globals.debug))) {
        ORTE_ERROR_LOG(ret);
        return ret;
    }

    /* set debug flag for now */
    orted_globals.debug = true;
fprintf(stderr, "daemon debug %d\n", (int)orted_globals.debug);
    
    /* check to see if I'm a bootproxy */
    if (orte_universe_info.bootproxy) { /* go fork/exec somebody and die */
        if (ORTE_SUCCESS != (ret = orte_daemon_bootproxy())) {
            ORTE_ERROR_LOG(ret);
        }
        orte_finalize();
        exit(1);
    }
    
    /* if not, that means I'm a daemon - daemonize myself */
#ifndef WIN32
    if (!orted_globals.debug) {
        ompi_output(0, "orted: daemonizing");
        orte_daemon_init(NULL);
    } else {
        ompi_output(0, "orted: debug mode - not daemonizing");
    }
#endif

    /* Okay, now on to serious business
     * First, ensure the process info structure in instantiated and initialized
     * and set the daemon flag to true
     */
    orte_proc_info();
    orte_process_info.daemon = true;
     
    if (!orte_process_info.seed) { /* if I'm not the seed... */
        /* start recording the compound command that starts us up */
       /* orte_gpr.begin_compound_cmd(); */
    }

    /*
     * Intialize the Open RTE
     */
    if (ORTE_SUCCESS != (ret = orte_init(cmd_line, argc, argv))) {
        fprintf(stderr, "orted: failed to init rte\n");
        return ret;
    }

    /* setup the thread lock and condition variable */
    OBJ_CONSTRUCT(&orted_globals.mutex, ompi_mutex_t);
    OBJ_CONSTRUCT(&orted_globals.condition, ompi_condition_t);

    /*
     *  Set my process status to "starting". Note that this must be done
     *  after the rte init is completed.
     */
    if (ORTE_SUCCESS != (ret = orte_soh.set_proc_soh(orte_process_info.my_name,
                                                     ORTE_PROC_STATE_STARTING, 0))) {
        ORTE_ERROR_LOG(ret);
        return ret;
    }

	if (!orte_process_info.seed) {  /* if I'm not the seed... */
	    /* execute the compound command - no return data requested
	    *  we'll get it all from the startup message
	    */
	    orte_gpr.exec_compound_cmd();
		
	    /* wait to receive startup message and info distributed */
	    if (ORTE_SUCCESS != (ret = orte_wait_startup_msg())) {
		    printf("ompid: failed to see all procs register\n");
		    return ret;
	    }
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
	    ompi_output(0, "ompid: contact_file %s", contact_file);

	    if (OMPI_SUCCESS != (ret = orte_write_universe_setup_file(contact_file))) {
	        if (orted_globals.debug) {
		        ompi_output(0, "[%d,%d,%d] ompid: couldn't write setup file", ORTE_NAME_ARGS(orte_process_info.my_name));
	        }
	    } else if (orted_globals.debug) {
	        ompi_output(0, "[%d,%d,%d] ompid: wrote setup file", ORTE_NAME_ARGS(orte_process_info.my_name));
	    }
    }


    if (orted_globals.debug) {
	    ompi_output(0, "[%d,%d,%d] ompid: issuing callback", ORTE_NAME_ARGS(orte_process_info.my_name));
    }

     /* register the daemon main callback function */
    ret = orte_rml.recv_buffer_nb(ORTE_RML_NAME_ANY, ORTE_RML_TAG_DAEMON, 0, orte_daemon_recv, NULL);
    if (ret != ORTE_SUCCESS && ret != ORTE_ERR_NOT_IMPLEMENTED) {
	    ORTE_ERROR_LOG(ret);
	    return ret;
    }

   /* go through the universe fields and see what else I need to do
     * - could be setup a virtual machine, spawn a console, etc.
     */

    if (orted_globals.debug) {
	    ompi_output(0, "[%d,%d,%d] ompid: setting up event monitor", ORTE_NAME_ARGS(orte_process_info.my_name));
    }

     /* setup and enter the event monitor */
    OMPI_THREAD_LOCK(&orted_globals.mutex);

    while (false == orted_globals.exit_condition) {
	    ompi_condition_wait(&orted_globals.condition, &orted_globals.mutex);
    }

    OMPI_THREAD_UNLOCK(&orted_globals.mutex);

    if (orted_globals.debug) {
	   ompi_output(0, "[%d,%d,%d] ompid: mutex cleared - finalizing", ORTE_NAME_ARGS(orte_process_info.my_name));
    }

    /* if i'm the seed, remove the universe-setup file */
    if (orte_process_info.seed) {
	   filenm = orte_os_path(false, orte_process_info.universe_session_dir, "universe-setup.txt", NULL);
	   unlink(filenm);
    }

    /* finalize the system */
    orte_finalize();

    if (orted_globals.debug) {
	   ompi_output(0, "[%d,%d,%d] ompid: done - exiting", ORTE_NAME_ARGS(orte_process_info.my_name));
    }

    exit(0);
}

static void orte_daemon_recv(int status, orte_process_name_t* sender,
			     orte_buffer_t *buffer, orte_rml_tag_t tag,
			     void* cbdata)
{
    orte_buffer_t *answer;
    orte_daemon_cmd_flag_t command;
    int ret;
    size_t n;
    char *contact_info;

    OMPI_THREAD_LOCK(&orted_globals.mutex);

    if (orted_globals.debug) {
	   ompi_output(0, "[%d,%d,%d] ompid: received message", ORTE_NAME_ARGS(orte_process_info.my_name));
    }

    answer = OBJ_NEW(orte_buffer_t);
    if (NULL == answer) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        goto DONE;
    }

    n = 1;
    if (ORTE_SUCCESS != (ret = orte_dps.unpack(buffer, &command, &n, ORTE_DAEMON_CMD))) {
        ORTE_ERROR_LOG(ret);
	    goto CLEANUP;
    }

        /****    EXIT COMMAND    ****/
    if (ORTE_DAEMON_EXIT_CMD == command) {
	    orted_globals.exit_condition = true;
	    ompi_condition_signal(&orted_globals.condition);

	/****     CONTACT QUERY COMMAND    ****/
    } else if (ORTE_DAEMON_CONTACT_QUERY_CMD == command) {
	   /* send back contact info */

	   contact_info = orte_rml.get_uri();

	   if (NULL == contact_info) {
           ORTE_ERROR_LOG(ORTE_ERROR);
           goto DONE;
       }
       
	   if (ORTE_SUCCESS != (ret = orte_dps.pack(answer, &contact_info, 1, ORTE_STRING))) {
            ORTE_ERROR_LOG(ret);
            goto DONE;
       }

	    if (0 > orte_rml.send_buffer(sender, answer, tag, 0)) {
              ORTE_ERROR_LOG(ORTE_ERR_COMM_FAILURE);
	    }
	}

 CLEANUP:
    OBJ_RELEASE(answer);

 DONE:
    /* reissue the non-blocking receive */
    ret = orte_rml.recv_buffer_nb(ORTE_RML_NAME_ANY, ORTE_RML_TAG_DAEMON, 0, orte_daemon_recv, NULL);
    if (ret != ORTE_SUCCESS && ret != ORTE_ERR_NOT_IMPLEMENTED) {
        ORTE_ERROR_LOG(ret);
    }

    OMPI_THREAD_UNLOCK(&orted_globals.mutex);
    return;
}

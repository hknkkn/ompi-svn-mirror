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
#include <fcntl.h>
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
#include "util/univ_info.h"
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

/*
 * define the orted context table for obtaining parameters
 */
orte_context_value_names_t orted_context_tbl[] = {
    /* start with usual help and version stuff */
    {{NULL, NULL, NULL}, "help", 0, ORTE_BOOL, (void*)&(orted_globals.help), (void*)false, NULL},
    {{NULL, NULL, NULL}, "version", 0, ORTE_BOOL, (void*)&(orted_globals.version), (void*)false, NULL},
    {{"daemon", "debug", NULL}, "debug", 0, ORTE_BOOL, (void*)&(orted_globals.debug), (void*)false, NULL},
    {{NULL, NULL, NULL}, NULL, 0, ORTE_NULL, NULL, NULL, NULL} /* terminate the table */
};


int main(int argc, char *argv[])
{
    int ret = 0;
    ompi_cmd_line_t *cmd_line = NULL;
    char *contact_path = NULL;
    char *log_path = NULL;

    /* setup to check common command line options that just report and die */
    cmd_line = OBJ_NEW(ompi_cmd_line_t);

    /* parse my context */
    if (ORTE_SUCCESS != (ret = orte_parse_context(orted_context_tbl, cmd_line, argc, argv))) {
        return ret;
    }
    
    /* check for help and version requests */
    if (orted_globals.help) {
        char *args = NULL;
        args = ompi_cmd_line_get_usage_msg(cmd_line);
        ompi_show_help("help-orted.txt", "orted:usage", false,
                       argv[0], args);
        free(args);
        return 1;
    }

    if (orted_globals.version) {
        /* show version message */
        printf("...showing off my version!\n");
        exit(1);
    }

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
    if (ORTE_SUCCESS != (ret = orte_init())) {
        fprintf(stderr, "orted: failed to init rte\n");
        return ret;
    }

    /*
     * Detach from controlling terminal - setup stdin/stdout/stderr 
     */

    if (orted_globals.debug == false || orte_universe_info.bootproxy > 0) {
        int fd;
        char log_file[PATH_MAX];

        /* connect input to /dev/null */
        fd = open("/dev/null", O_RDONLY);
        if(fd > STDIN_FILENO) {
            dup2(fd, STDIN_FILENO);
            close(fd);
        }

        /* connect output to a log file in the session directory */
        sprintf(log_file, "orted-%d-%s.log", 
            orte_process_info.my_name->jobid, orte_system_info.nodename);
	    log_path = orte_os_path(false, 
            orte_process_info.tmpdir_base, 
            orte_process_info.top_session_dir, 
            log_file, 
            NULL);

        fd = open(log_path, O_RDWR|O_CREAT|O_TRUNC, 0666);
        if(fd < 0) {
             fd = open("/tmp/orted.log", O_RDWR|O_CREAT|O_TRUNC, 0666);
        }
        if(fd >= 0) {
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            if(fd != STDOUT_FILENO && fd != STDERR_FILENO) {
               close(fd);
            }
        }

        /* detach from controlling terminal */
        if(orted_globals.debug == false) {
            orte_daemon_init(NULL);
        }
    }

    /* check to see if I'm a bootproxy */
    if (orte_universe_info.bootproxy) { /* perform bootproxy-specific things */
        ompi_output(0, "orted: bootproxy(%d)\n", orte_universe_info.bootproxy);
        if (ORTE_SUCCESS != (ret = orte_daemon_bootproxy())) {
            ORTE_ERROR_LOG(ret);
        }
        if (ORTE_SUCCESS != (ret = orte_finalize())) {
            ORTE_ERROR_LOG(ret);
        }
        exit(ret);
    }

    /* setup the thread lock and condition variable */
    OBJ_CONSTRUCT(&orted_globals.mutex, ompi_mutex_t);
    OBJ_CONSTRUCT(&orted_globals.condition, ompi_condition_t);

    /*
     *  Set my process status to "starting". Note that this must be done
     *  after the rte init is completed.
     */
    if (ORTE_SUCCESS != (ret = orte_soh.set_proc_soh(orte_process_info.my_name,
                                                     ORTE_PROC_STATE_RUNNING, 0))) {
        ORTE_ERROR_LOG(ret);
        return ret;
    }

	if (!orte_process_info.seed) {  /* if I'm not the seed... */
	    /* execute the compound command - no return data requested
	    *  we'll get it all from the startup message
	    */
/*	    orte_gpr.exec_compound_cmd(); */
		
	    /* wait to receive startup message and info distributed */
/*	    if (ORTE_SUCCESS != (ret = orte_wait_startup_msg())) {
		    printf("ompid: failed to see all procs register\n");
		    return ret;
	    }
*/
	}
	
    /* if i'm the seed, get my contact info and write my setup file for others to find */
    if (orte_process_info.seed) {
	    if (NULL != orte_universe_info.seed_uri) {
	        free(orte_universe_info.seed_uri);
	        orte_universe_info.seed_uri = NULL;
	    }
	    orte_universe_info.seed_uri = orte_rml.get_uri();
	    contact_path = orte_os_path(false, orte_process_info.universe_session_dir,
				    "universe-setup.txt", NULL);
	    ompi_output(0, "ompid: contact_file %s", contact_path);

	    if (OMPI_SUCCESS != (ret = orte_write_universe_setup_file(contact_path))) {
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

    /* cleanup */
    if (NULL != contact_path) {
	    unlink(contact_path);
    }
    if (NULL != log_path) {
        unlink(log_path);
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

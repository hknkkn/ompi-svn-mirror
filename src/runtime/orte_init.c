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

#include <sys/types.h>
#include <unistd.h>

#include "include/constants.h"
#include "event/event.h"
#include "util/output.h"
#include "threads/mutex.h"
#include "mca/mca.h"
#include "mca/base/base.h"
#include "mca/base/mca_base_param.h"
#include "mca/rml/base/base.h"
#include "mca/errmgr/base/base.h"
#include "mca/ns/base/base.h"
#include "mca/gpr/base/base.h"
#include "mca/rmgr/base/base.h"
#include "util/proc_info.h"
#include "util/session_dir.h"
#include "util/sys_info.h"
#include "util/cmd_line.h"

#include "runtime/runtime.h"
#include "runtime/runtime_internal.h"
#include "runtime/orte_wait.h"

/**
 * Initialze and setup a process in the ORTE.
 *
 * @retval ORTE_SUCCESS Upon success.
 * @retval ORTE_ERROR Upon failure.
 *
 * This function performs 
 * 
 * Just a note for developer: 

 * So there are 3 ways in which an application can be started
 * 1) rte_boot, followed by mpirun
 * 2) mpirun (alone)
 * 3) singleton (./a.out)
 * 
 * Case 1) If the rte has already been booted, then mpirun will accept
 * an optional command line parameter --universe=[rte universe name]
 * which says which universe this application wants to be a part
 * of. mpirun will then package this universe name and send it to the
 * processes it will be starting off (fork/exec) on local or remote
 * node.The packaging mechanism can be either command line parameter
 * to the a.out it forks or make it part of environment
 * (implementation dependent).  
 *
 * Case 2) When mpirun is done alone and no universe is present, then
 * the mpirun starts off the universe (using rte_boot), then
 * fork/execs the processes, passin g along the [universe_name]. 
 *
 * Case 3) For a singleton, if there is alrady an existing rte
 * universe which it wants to join, it can specify that using the
 * --universe command line. So it will do 
 *
 * $ ./a.out --universe=[universe_name]
 * 
 * In this case, MPI_Init will have to be called as MPI_Init(&argc, &argv)

 * If it does not want to join any existing rte, then it just starts
 * off as ./a.out with no command line option. In that case, MPI_Init
 * does not necesaarily needs to passed argc and argv. Infact if argc
 * and argv are not passed or just have one entry (the command name),
 * then MPI_Init would assume that new rte universe needs to be
 * started up.
 *
 *
 * MPI_Init() will look at its argc, argv. If it find the universe
 * name there, fine. Else it looks at the environment variables for
 * universe_name. If it finds there, fine again. Under such
 * conditions, it joins the existing rte universe. If no universe
 * name is found, it calls rte_boot to start off a new rte universe.
 *
 * For singleton, MPI_Init() do:
 *
 * if (I am a singleton) and (there is no universe)
 *    do rte_boot
 *
 * But if I am not a singleton, then I have been started by mpirun and
 * already provided a universe_name to join. So I wont ever start a
 * universe under such conditons. mpirun will pass me the
 * universe_name (either mpirun would have started the universe, or it
 * would have been already started by rte_boot)
 */

/* globals used by RTE */
int orte_debug_flag=0;
orte_universe_t orte_universe_info = {
    /* .name =                */    NULL,
    /* .host =                */    NULL,
    /* .uid =                 */    NULL,
    /* .persistence =         */    false,
    /* .scope =               */    NULL,
    /* .console =             */    false,
    /* .seed_uri =            */    NULL,
    /* .console_connected =   */    false,
    /* .scriptfile =          */    NULL,
};

int orte_init(ompi_cmd_line_t *cmd_line, bool *allow_multi_user_threads, bool *have_hidden_threads)
{
    int ret;
    bool user_threads, hidden_threads;
    char *universe, *jobid_str, *procid_str;
    pid_t pid;

    *allow_multi_user_threads = true;
    *have_hidden_threads = false;

    ret =  mca_base_param_register_int("orte", "debug", NULL, NULL, 1);
    mca_base_param_lookup_int(ret, &orte_debug_flag);

    /* ensure the error manager is open */
    if (ORTE_SUCCESS != (ret = orte_errmgr_base_open())) {
        ompi_output(0, "orte_init: failed to open errmgr - aborting");
        return ret;
    }
    
    /* ensure that orte_process_info structure has been initialized */
    orte_proc_info();
    
    /*
     * Initialize the event library 
    */
    if (OMPI_SUCCESS != (ret = ompi_event_init())) {
	    ORTE_ERROR_LOG(ret);
	    return ret;
    }

    /*
     * Internal startup
     */
    if (OMPI_SUCCESS != (ret = orte_wait_init())) {
	    ORTE_ERROR_LOG(ret);
	    return ret;
    }


    /*
     * Name Server - just do the open so we can access base components
     */
    if (OMPI_SUCCESS != (ret = orte_ns_base_open())) {
        ORTE_ERROR_LOG(ret);
        return ret;
    }

    /*
     * Runtime Messaging Layer
     */
    if (OMPI_SUCCESS != (ret = orte_rml_base_open())) {
        ORTE_ERROR_LOG(ret);
	    return ret;
    }

    /* parse ORTE environmental variables and fill corresponding info structures */
    orte_parse_environ();

    if (NULL != cmd_line) {
        	/* parse the cmd_line for rte options - override settings from enviro, where necessary
        	 * copy everything into enviro variables for passing later on
        	 */
        	orte_parse_cmd_line(cmd_line);
        
        	/* parse the cmd_line for daemon options - gets all the options relating
        	 * specifically to seed behavior, in case i'm a seed, but also gets
        	 * options about scripts and hostfiles that might be of use to me
        	 * overrride enviro variables where necessary
        	 */
        	orte_parse_daemon_cmd_line(cmd_line);
    }

    /* check for existing universe to join */
    if (ORTE_SUCCESS != (ret = orte_universe_exists())) {
        	if (orte_debug_flag) {
        	    ompi_output(0, "orte_init: could not join existing universe");
        	}
        	if (ORTE_ERR_NOT_FOUND != ret) {
        	    /* if it exists but no contact could be established,
        	     * define unique name based on current one.
        	     * and start new universe with me as seed
        	     */
        	    universe = strdup(orte_universe_info.name);
        	    free(orte_universe_info.name);
        	    orte_universe_info.name = NULL;
        	    pid = getpid();
        	    if (0 > asprintf(&orte_universe_info.name, "%s-%d", universe, pid)) {
                ompi_output(0, "orte_init: failed to create unique universe name");
                return ret;
             }
	    }

        	orte_process_info.seed = true;
        	if (NULL != orte_process_info.ns_replica) {
        	    free(orte_process_info.ns_replica);
        	    orte_process_info.ns_replica = NULL;
        	}
        	if (NULL != orte_process_info.gpr_replica) {
        	    free(orte_process_info.gpr_replica);
        	    orte_process_info.gpr_replica = NULL;
        	}
    }

    /* init thread flags */
    user_threads = true;
    hidden_threads = false;

    /*
     * Name Server - base already opened, so just complete the selection
     * of the proper module
     */
    if (ORTE_SUCCESS != (ret = orte_ns_base_select(&user_threads, &hidden_threads))) {
        ORTE_ERROR_LOG(ret);
        return ret;
    }
    *allow_multi_user_threads &= user_threads;
    *have_hidden_threads |= hidden_threads;

    /*
     * Registry 
     */
    if (ORTE_SUCCESS != (ret = orte_gpr_base_open())) {
        ORTE_ERROR_LOG(ret);
        return ret;
    }
    user_threads = true;
    hidden_threads = false;
    if (ORTE_SUCCESS != (ret = orte_gpr_base_select(&user_threads, 
						   &hidden_threads))) {
        ORTE_ERROR_LOG(ret);
        return ret;
    }
    *allow_multi_user_threads &= user_threads;
    *have_hidden_threads |= hidden_threads;
 
    /*****    SET MY NAME    *****/
    if (ORTE_SUCCESS != (ret = orte_ns.set_my_name())) {
        ORTE_ERROR_LOG(ret);
        return ret;
    }
    
    /* setup my session directory */
    if (ORTE_SUCCESS != (ret = orte_ns.get_jobid_string(&jobid_str, orte_process_info.my_name))) {
        ORTE_ERROR_LOG(ret);
        return ret;
    }
    if (ORTE_SUCCESS != (ret = orte_ns.get_vpid_string(&procid_str, orte_process_info.my_name))) {
        ORTE_ERROR_LOG(ret);
        return ret;
    }
 
    if (orte_debug_flag) {
	    ompi_output(0, "[%d,%d,%d] setting up session dir with",
                    ORTE_NAME_ARGS(*orte_process_info.my_name));
	    if (NULL != orte_process_info.tmpdir_base) {
	        ompi_output(0, "\ttmpdir %s", orte_process_info.tmpdir_base);
	    }
        	ompi_output(0, "\tuniverse %s", orte_universe_info.name);
        	ompi_output(0, "\tuser %s", orte_system_info.user);
        	ompi_output(0, "\thost %s", orte_system_info.nodename);
        	ompi_output(0, "\tjobid %s", jobid_str);
        	ompi_output(0, "\tprocid %s", procid_str);
    }
    if (ORTE_SUCCESS != (ret = orte_session_dir(true,
                                orte_process_info.tmpdir_base,
                                orte_system_info.user,
                                orte_system_info.nodename, NULL,
                                orte_universe_info.name,
                                jobid_str, procid_str))) {
        if (jobid_str != NULL) free(jobid_str);
        if (procid_str != NULL) free(procid_str);
        ORTE_ERROR_LOG(ret);
        return ret;
    }

    /* setup the resource manager */
    if (ORTE_SUCCESS != (ret = orte_rmgr_base_open())) {
        ORTE_ERROR_LOG(ret);
        return ret;
    }
    
     /* 
     * All done 
     */

    return ORTE_SUCCESS;
}


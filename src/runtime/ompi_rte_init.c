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

/* #define _GNU_SOURCE */

#include "ompi_config.h"

#include <sys/types.h>
#include <unistd.h>

#include "include/constants.h"
#include "event/event.h"
#include "util/output.h"
#include "threads/mutex.h"
#include "mca/mca.h"
#include "mca/base/base.h"
#include "mca/base/mca_base_param.h"
#include "mca/pcmclient/base/base.h"
#include "mca/oob/oob.h"
#include "mca/ns/base/base.h"
#include "mca/gpr/base/base.h"
#include "util/proc_info.h"
#include "util/session_dir.h"
#include "util/sys_info.h"
#include "util/cmd_line.h"

#include "runtime/runtime.h"
#include "runtime/runtime_internal.h"
#include "runtime/ompi_rte_wait.h"

/**
 * Initialze and setup a process in the OMPI RTE.
 *
 * @retval OMPI_SUCCESS Upon success.
 * @retval OMPI_ERROR Upon failure.
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
int ompi_rte_debug_flag=0;
ompi_universe_t ompi_universe_info = {
    /* .name =                */    NULL,
    /* .host =                */    NULL,
    /* .uid =                 */    NULL,
    /* .pid =                 */    0,
    /* .persistence =         */    false,
    /* .scope =               */    NULL,
    /* .probe =               */    false,
    /* .console =             */    false,
    /* .ns_replica =          */    NULL,
    /* .gpr_replica =         */    NULL,
    /* .seed_contact_info =    */   NULL,
    /* .console_connected =   */    false,
    /* .scriptfile =          */    NULL,
    /* .hostfile =            */    NULL
};

static void printname(char *location);

int ompi_rte_init(ompi_cmd_line_t *cmd_line, bool *allow_multi_user_threads, bool *have_hidden_threads)
{
    int ret, cmpval;
    bool user_threads, hidden_threads;
    char *universe, *jobid_str, *procid_str;
    pid_t pid;
    orte_jobid_t jobid;
    orte_vpid_t vpid;
    orte_process_name_t illegal_name={ORTE_CELLID_MAX, ORTE_JOBID_MAX, ORTE_VPID_MAX};
    orte_process_name_t *new_name;

    *allow_multi_user_threads = true;
    *have_hidden_threads = false;

    ret =  mca_base_param_register_int("ompi", "rte", "debug", NULL, 0);
    mca_base_param_lookup_int(ret, &ompi_rte_debug_flag);


    /*
     * Initialize the event library 
    */
    if (OMPI_SUCCESS != (ret = ompi_event_init())) {
	    ompi_output(0, "ompi_rte_init: ompi_event_init failed with error status: %d\n", ret);
	    return ret;
    }

    /*
     * Internal startup
     */
    if (OMPI_SUCCESS != (ret = ompi_rte_internal_init_spawn())) {
	/* JMS show_help */
	printf("show_help: ompi_rte_init failed in ompi_rte_internal_init_spawn\n");
	return ret;
    }
    if (OMPI_SUCCESS != (ret = ompi_rte_wait_init())) {
	/* JMS show_help */
	printf("show_help: ompi_rte_init failed in ompi_rte_wait_init\n");
	return ret;
    }


    /*
     * Out of Band Messaging
     */
    if (OMPI_SUCCESS != (ret = mca_oob_base_open())) {
	/* JMS show_help */
	printf("show_help: ompi_rte_init failed in oob_base_open\n");
	return ret;
    }
    user_threads = true;
    hidden_threads = false;


    /*
     * Name Server - just do the open so we can access base components
     */
    if (OMPI_SUCCESS != (ret = orte_ns_base_open())) {
	/* JMS show_help */
	printf("show_help: ompi_rte_init failed in ns_base_open\n");
	return ret;
    }

    /*
     * Process Control and Monitoring Client - just open for now
     */
    if (OMPI_SUCCESS != (ret = mca_pcmclient_base_open())) {
	/* JMS show_help */
	printf("show_help: ompi_rte_init failed in pcmclient_base_open\n");
	return ret;
    }

    printname("component open");

    /*
     * Process Control and Monitoring Client -
     * just complete selection of proper module
     */
    user_threads = true;
    hidden_threads = false;
    if (OMPI_SUCCESS != (ret = mca_pcmclient_base_select(&user_threads, 
							 &hidden_threads))) {
	printf("show_help: ompi_rte_init failed in pcmclient_base_select\n");
	/* JMS show_help */
	return ret;
    }
    *allow_multi_user_threads &= user_threads;
    *have_hidden_threads |= hidden_threads;

    printname("pcm_select");

    /* complete setup of OOB */
    if (OMPI_SUCCESS != (ret = mca_oob_base_init(&user_threads, 
						 &hidden_threads))) {
	/* JMS show_help */
	printf("show_help: ompi_rte_init failed in mca_oob_base_init()\n");
	return ret;
    }
    *allow_multi_user_threads &= user_threads;
    *have_hidden_threads |= hidden_threads;

    printname("oob_init");

    /* parse non-PCM environmental variables and fill corresponding info structures */
    ompi_rte_parse_environ();

    printname("parse_environ");

    if (NULL != cmd_line) {
	/* parse the cmd_line for rte options - override settings from enviro, where necessary
	 * copy everything into enviro variables for passing later on
	 */
	ompi_rte_parse_cmd_line(cmd_line);

	/* parse the cmd_line for daemon options - gets all the options relating
	 * specifically to seed behavior, in case i'm a seed, but also gets
	 * options about scripts and hostfiles that might be of use to me
	 * overrride enviro variables where necessary
	 */
	ompi_rte_parse_daemon_cmd_line(cmd_line);
    }

    printname("cmd_line");

    /* check for existing universe to join */
    if (OMPI_SUCCESS != (ret = ompi_rte_universe_exists())) {
	if (ompi_rte_debug_flag) {
	    ompi_output(0, "ompi_mpi_init: could not join existing universe");
	}
	if (OMPI_ERR_NOT_FOUND != ret) {
	    /* if it exists but no contact could be established,
	     * define unique name based on current one.
	     * and start new universe with me as seed
	     */
	    universe = strdup(ompi_universe_info.name);
	    free(ompi_universe_info.name);
	    ompi_universe_info.name = NULL;
	    pid = getpid();
	    if (0 > asprintf(&ompi_universe_info.name, "%s-%d", universe, pid) && ompi_rte_debug_flag) {
		ompi_output(0, "mpi_init: error creating unique universe name");
	    }
	}

	ompi_process_info.my_universe = strdup(ompi_universe_info.name);
	ompi_process_info.seed = true;
	if (NULL != ompi_universe_info.ns_replica) {
	    free(ompi_universe_info.ns_replica);
	    ompi_universe_info.ns_replica = NULL;
	}
	if (NULL != ompi_process_info.ns_replica) {
	    free(ompi_process_info.ns_replica);
	    ompi_process_info.ns_replica = NULL;
	}
	if (NULL != ompi_universe_info.gpr_replica) {
	    free(ompi_universe_info.gpr_replica);
	    ompi_universe_info.gpr_replica = NULL;
	}
	if (NULL != ompi_process_info.gpr_replica) {
	    free(ompi_process_info.gpr_replica);
	    ompi_process_info.gpr_replica = NULL;
	}
    }

    printname("univ_exists");

    /*
     * Name Server - base already opened in stage1, so just complete the selection
     * of the proper module
     */
    user_threads = true;
    hidden_threads = false;
    if (OMPI_SUCCESS != (ret = orte_ns_base_select(&user_threads,
						  &hidden_threads))) {
	/* JMS show_help */
	printf("show_help: ompi_rte_init failed in ns_base_select\n");
	return ret;
    }
    *allow_multi_user_threads &= user_threads;
    *have_hidden_threads |= hidden_threads;

    /*
     * Registry 
     */
    if (OMPI_SUCCESS != (ret = mca_gpr_base_open())) {
	/* JMS show_help */
	printf("show_help: ompi_rte_init failed in mca_gpr_base_open()\n");
	return ret;
    }
    user_threads = true;
    hidden_threads = false;
    if (OMPI_SUCCESS != (ret = mca_gpr_base_select(&user_threads, 
						   &hidden_threads))) {
	/* JMS show_help */
	printf("show_help: ompi_rte_init failed in mca_gpr_base_select()\n");
	return ret;
    }
    *allow_multi_user_threads &= user_threads;
    *have_hidden_threads |= hidden_threads;
 
    printname("gpr_select");

    /*****    SET MY NAME IF NOT ALREADY PROVIDED IN ENVIRONMENT   *****/
    if (ORTE_SUCCESS != (ret = orte_name_services.compare(&cmpval, ORTE_NS_CMP_ALL, 
                                      ompi_rte_get_self(), 
                                      &illegal_name))) {
        return ret;
    }
    if (0 == cmpval) {
        /* name not previously set */
	   if (ompi_process_info.seed || NULL == ompi_process_info.ns_replica) {
            /* seed or singleton - couldn't join existing univ */
            if (ORTE_SUCCESS != (ret = orte_name_services.create_process_name(new_name, 0,0,0))) {
                return ret;
            }
	       *ompi_rte_get_self() = *new_name;
            free(new_name);
	       printname("singleton/seed");
	   } else {  
            /* not seed or singleton - name server exists elsewhere - get a name for me */
	       if (ORTE_SUCCESS != (ret = orte_name_services.create_jobid(&jobid))) {
                return ret;
           }
	       if (ORTE_SUCCESS != (ret = orte_name_services.reserve_range(jobid, 1, &vpid))) {
                return ret;
           }
           if (ORTE_SUCCESS != (ret = orte_name_services.create_process_name(new_name, 0, jobid, vpid))) {
                return ret;
           }
	       *ompi_rte_get_self() = *new_name;
            free(new_name);
	       printname("name_server_provided");
	   }
    }

    /* setup my session directory */
    if (ORTE_SUCCESS != (ret = orte_name_services.get_jobid_string(jobid_str, ompi_rte_get_self()))) {
        return ret;
    }
    if (ORTE_SUCCESS != (ret = orte_name_services.get_vpid_string(procid_str, ompi_rte_get_self()))) {
        return ret;
    }
 
    if (ompi_rte_debug_flag) {
	    ompi_output(0, "[%d,%d,%d] setting up session dir with",
                    ompi_rte_get_self()->cellid, 
                    ompi_rte_get_self()->jobid, 
                    ompi_rte_get_self()->vpid);
	    if (NULL != ompi_process_info.tmpdir_base) {
	       ompi_output(0, "\ttmpdir %s", ompi_process_info.tmpdir_base);
	    }
        	ompi_output(0, "\tuniverse %s", ompi_process_info.my_universe);
        	ompi_output(0, "\tuser %s", ompi_system_info.user);
        	ompi_output(0, "\thost %s", ompi_system_info.nodename);
        	ompi_output(0, "\tjobid %s", jobid_str);
        	ompi_output(0, "\tprocid %s", procid_str);
    }
    if (OMPI_ERROR == ompi_session_dir(true,
				       ompi_process_info.tmpdir_base,
				       ompi_system_info.user,
				       ompi_system_info.nodename, NULL, 
				       ompi_process_info.my_universe,
				       jobid_str, procid_str)) {
	if (jobid_str != NULL) free(jobid_str);
	if (procid_str != NULL) free(procid_str);
	exit(-1);
    }

     /* 
     * All done 
     */

    return OMPI_SUCCESS;
}

static void printname(char *loc)
{
    if (ompi_rte_debug_flag) {
	if (NULL == ompi_rte_get_self()) {
	    ompi_output(0, "My name after %s has NOT been set", loc);
	} else {
	    ompi_output(0, "My name after %s is [%d,%d,%d]", loc, ORTE_NAME_ARGS(*ompi_rte_get_self()));
	}
    }
}

orte_process_name_t*
ompi_rte_get_self(void)
{
    if (NULL == mca_pcmclient.pcmclient_get_self) {
        errno = OMPI_ERR_NOT_IMPLEMENTED;
        return NULL;
    }

    return mca_pcmclient.pcmclient_get_self();
}


int
ompi_rte_get_peers(orte_process_name_t **peers, size_t *npeers)
{
    orte_process_name_t *useless;
    orte_process_name_t **peers_p;

    if (NULL == mca_pcmclient.pcmclient_get_peers) {
        return OMPI_ERR_NOT_IMPLEMENTED;
    }

    if (NULL == peers) {
        /* the returned value is a pointer to a static buffer, so no
           free is neeeded.  This is therefore completely safe.  Yay */
        peers_p = &useless;
    } else {
        peers_p = peers;
    }

    return mca_pcmclient.pcmclient_get_peers(peers_p, npeers);
}


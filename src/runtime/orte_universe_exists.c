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

/**
 * @file
 *
 * Setup command line options for the Open MPI Run Time Environment
 */


#include "ompi_config.h"

#include <string.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include "include/constants.h"
#include "util/output.h"
#include "util/sys_info.h"
#include "util/proc_info.h"
#include "util/os_path.h"
#include "util/session_dir.h"
#include "util/universe_setup_file_io.h"

#include "mca/oob/base/base.h"
#include "mca/ns/ns.h"

#include "runtime/runtime.h"


static struct timeval ompi_rte_ping_wait = {2, 0};


int orte_universe_exists()
{
    char *contact_file;
    int ret;
    orte_process_name_t proc={0,0,0};
/*    bool ns_found=false, gpr_found=false; */
    bool ping_success=false;

    /* TEMPORARY */
    return ORTE_ERR_NOT_FOUND;

    /* if both ns_replica and gpr_replica were provided, check for contact with them */
    if (NULL != orte_process_info.ns_replica && NULL != orte_process_info.gpr_replica) {
	return OMPI_SUCCESS;
    }

/* 	/\* ping to verify ns_replica alive *\/ */
/* 	if (OMPI_SUCCESS != mca_oob_ping(orte_process_info.ns_replica, &ompi_rte_ping_wait)) { */
/* 	    if (ompi_rte_debug_flag) { */
/* 		ompi_output(0, "univ_exists: ns_replica ping failed"); */
/* 	    } */
/* 	    free(ompi_universe_info.ns_replica); */
/* 	    if (NULL != orte_process_info.ns_replica) { */
/* 		free(orte_process_info.ns_replica); */
/* 		orte_process_info.ns_replica = NULL; */
/* 	    } */
/* 	} else {  /\* name server found *\/ */
/* 	    ns_found = true; */
/* 	} */
/* 	/\* next check gpr - first see if it's the same as ns. if so, don't bother *\/ */

/* 	if (0 != ns_base_compare(OMPI_NS_CMP_ALL, orte_process_info.ns_replica, */
/* 				 orte_process_info.gpr_replica)) { */
/* 	    /\* ping to verify gpr_replica alive *\/ */
/* 	    if (OMPI_SUCCESS != mca_oob_ping(orte_process_info.gpr_replica, &ompi_rte_ping_wait)) { */
/* 		if (ompi_rte_debug_flag) { */
/* 		    ompi_output(0, "univ_exists: gpr_replica ping failed"); */
/* 		} */
/* 		free(ompi_universe_info.gpr_replica); */
/* 		if (NULL != orte_process_info.gpr_replica) { */
/* 		    free(orte_process_info.gpr_replica); */
/* 		    orte_process_info.gpr_replica = NULL; */
/* 		} */
/* 	    } else {  */
/* 		gpr_found = true; */
/* 	    } */
/* 	} else { /\* the same - no point in checking *\/ */
/* 	    gpr_found = ns_found; */
/* 	} */

/* 	if (ns_found && gpr_found) { /\* success on both counts - report it *\/ */
/* 	    return OMPI_SUCCESS; */
/* 	} */
/*     } */

    /* if we are missing one or both, we need to get the missing info. first check
     * to see if seed_contact_info already provided. if so, then contact that daemon
     * to get missing info.
     */

    /* otherwise, need to find an initial "seed" contact point so we can get the info.
     * check if local or remote host specified
     */

    if (0 != strncmp(orte_universe_info.host, orte_system_info.nodename, strlen(orte_system_info.nodename))) { /* remote host specified */
	ompi_output(0, "remote hosts not currently supported");
	return OMPI_ERR_NOT_IMPLEMENTED;
    }

    if (orte_debug_flag) {
	ompi_output(0, "looking for session directory");
    }

    /* check to see if local universe already exists */
    if (OMPI_SUCCESS == orte_session_dir(false,
					 orte_process_info.tmpdir_base,
					 orte_system_info.user,
					 orte_system_info.nodename,
					 NULL,
					 orte_universe_info.name,
					 NULL,
					 NULL)) { /* found */

	if (orte_debug_flag) {
	    ompi_output(0, "check for contact info file");
	}

	/* check for "contact-info" file. if present, read it in. */
	contact_file = orte_os_path(false, orte_process_info.universe_session_dir,
				    "universe-setup.txt", NULL);

	if (OMPI_SUCCESS != (ret = ompi_read_universe_setup_file(contact_file))) {
	    if (orte_debug_flag) {
		ompi_output(0, "could not read contact file %s", contact_file);
	    }
	    return ret;
	}

	if (orte_debug_flag) {
	    ompi_output(0, "contact info read");
	}

	if (!orte_universe_info.console) {  /* if we aren't trying to connect a console */
	    if (!orte_universe_info.persistence ||   /* not persistent... */
		(0 == strncmp(orte_universe_info.scope, "exclusive", strlen("exclusive")))) {  /* ...or no connection allowed */
		/* also need to check "local" and that we did not specify the exact
		 * matching universe name
		 */
		if (orte_debug_flag) {
		    ompi_output(0, "connection not allowed");
		}
		return OMPI_ERR_NO_CONNECTION_ALLOWED;
	    }
	}

	if (orte_debug_flag) {
	    ompi_output(0, "contact info to set: %s", orte_universe_info.seed_uri);
	}


	/* if persistent, set contact info... */
	if (OMPI_SUCCESS != mca_oob_set_contact_info(orte_universe_info.seed_uri)) { /* set contact info */
	    if (orte_debug_flag) {
		ompi_output(0, "error setting oob contact info - please report error to bugs@open-mpi.org\n");
	    }
	    return OMPI_ERR_FATAL;
	}

	mca_oob_parse_contact_info(orte_universe_info.seed_uri, &proc, NULL);

	if (orte_debug_flag) {
	    ompi_output(0, "contact info set: %s", orte_universe_info.seed_uri);
	    ompi_output(0, "issuing ping: %d %d %d", proc.cellid, proc.jobid, proc.vpid);
	}


	/* ...and ping to verify it's alive */
	ping_success = false;
	if (OMPI_SUCCESS == mca_oob_ping(&proc, &ompi_rte_ping_wait)) {
	    ping_success = true;
	}
	if (!ping_success) {
	    if (orte_debug_flag) {
		ompi_output(0, "ping failed");
	    }
	    return OMPI_ERR_CONNECTION_FAILED;
	}

	if (NULL != orte_process_info.ns_replica) {
	    free(orte_process_info.ns_replica);
	    orte_process_info.ns_replica = NULL;
	}
    if (ORTE_SUCCESS != (ret = orte_ns.copy_process_name(&orte_process_info.ns_replica, &proc))) {
        return ret;
    }

	if (NULL != orte_process_info.gpr_replica) {
	    free(orte_process_info.gpr_replica);
	    orte_process_info.gpr_replica = NULL;
	}
    if (ORTE_SUCCESS != (ret = orte_ns.copy_process_name(&orte_process_info.gpr_replica, &proc))) {
        return ret;
    }

	/* request ns_replica and gpr_replica info for this process
	 * only request info required - check ns_found/gpr_found
	 */

	return OMPI_SUCCESS;
    }

    return OMPI_ERR_NOT_FOUND;
}

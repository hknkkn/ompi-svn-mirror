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
 * Parse command line options for the Open MPI Run Time Environment. This program MUST be called before
 * any call to ompi_rte_init_stage1 and/or ompi_rte_init_stage2 !!!
 *
 */
#include "ompi_config.h"

#include <stdlib.h>
#include <string.h>

#include "mca/oob/base/base.h"
#include "mca/ns/ns.h"

#include "util/output.h"
#include "util/cmd_line.h"
#include "util/sys_info.h"
#include "util/proc_info.h"

#include "runtime/runtime.h"

void ompi_rte_parse_cmd_line(ompi_cmd_line_t *cmd_line)
{
    char *universe=NULL, *nsreplica=NULL, *gprreplica=NULL, *tmp=NULL;


    /* get universe name and store it, if user specified it */
    /* otherwise, stick with default name */

    if (ompi_cmd_line_is_taken(cmd_line, "universe") ||
	ompi_cmd_line_is_taken(cmd_line, "u")) {
	if (NULL == ompi_cmd_line_get_param(cmd_line, "universe", 0, 0)) {
	    ompi_output(0, "error retrieving universe name - please report error to bugs@open-mpi.org\n");
	    return;
        }
        universe = strdup(ompi_cmd_line_get_param(cmd_line, "universe", 0, 0));


	if (NULL != (tmp = strchr(universe, ':'))) { /* name contains remote host */
	    /* get the host name, and the universe name separated */
	    /* could be in form remote-uid@remote-host:universe */
	    *tmp = '\0';
	    tmp++;
	    ompi_universe_info.name = strdup(tmp);
	    if (NULL != (tmp = strchr(universe, '@'))) {  /* remote name includes remote uid */
		*tmp = '\0';
		tmp++;
		if (NULL != ompi_universe_info.host) {  /* overwrite it */
		    free(ompi_universe_info.host);
		    ompi_universe_info.host = NULL;
		}
		ompi_universe_info.host = strdup(tmp);
		if (NULL != ompi_universe_info.uid) {
		    free(ompi_universe_info.uid);
		    ompi_universe_info.uid = NULL;
		}
		ompi_universe_info.uid = strdup(universe);
	    } else {  /* no remote id - just remote host */
		if (NULL != ompi_universe_info.host) {
		    free(ompi_universe_info.host);
		    ompi_universe_info.host = NULL;
		}
		ompi_universe_info.host = strdup(universe);
	    }
	} else { /* no remote host - just universe name provided */
	    if (NULL != ompi_universe_info.name) {
		free(ompi_universe_info.name);
		ompi_universe_info.name = NULL;
	    }
	    ompi_universe_info.name = strdup(universe);
	}
    }

    /* copy the universe name into the process_info structure */
    if (NULL != ompi_universe_info.name) {
	if (NULL != ompi_process_info.my_universe) {
	    free(ompi_process_info.my_universe);
	    ompi_process_info.my_universe = NULL;
	}
	ompi_process_info.my_universe = strdup(ompi_universe_info.name);
    } else {  /* set it to default value */
	ompi_universe_info.name = strdup("default-universe");
	if (NULL != ompi_process_info.my_universe) { /* overwrite it */
	    free(ompi_process_info.my_universe);
	    ompi_process_info.my_universe = NULL;
	}
	ompi_process_info.my_universe = strdup(ompi_universe_info.name);
    }

    /* and set the appropriate enviro variable */
    setenv("OMPI_universe_name", ompi_universe_info.name, 1);

    /* get the temporary directory name for the session directory, if provided on command line */
    if (ompi_cmd_line_is_taken(cmd_line, "tmpdir")) {
	if (NULL == ompi_cmd_line_get_param(cmd_line, "tmpdir", 0, 0)) {
	    ompi_output(0, "error retrieving tmpdir name - please report error to bugs@open-mpi.org\n");
	    return;
	}
	if (NULL != ompi_process_info.tmpdir_base) { /* overwrite it */
	    free(ompi_process_info.tmpdir_base);
	    ompi_process_info.tmpdir_base = NULL;
	}
	ompi_process_info.tmpdir_base = strdup(ompi_cmd_line_get_param(cmd_line, "tmpdir", 0, 0));
	setenv("OMPI_tmpdir_base", ompi_process_info.tmpdir_base, 1);
    } /* otherwise, leave it alone */

    /* see if name server replica provided */
    if (ompi_cmd_line_is_taken(cmd_line, "nsreplica")) {
	if (NULL == ompi_cmd_line_get_param(cmd_line, "nsreplica", 0, 0)) {
	    ompi_output(0, "error retrieving name server replica - please report error to bugs@open-mpi.org");
	    return;
	}
	nsreplica = strdup(ompi_cmd_line_get_param(cmd_line, "nsreplica", 0, 0));
	if (NULL != ompi_universe_info.ns_replica) {
	    free(ompi_universe_info.ns_replica);
	    ompi_universe_info.ns_replica = NULL;
	}
	ompi_universe_info.ns_replica = strdup(nsreplica);
	if (NULL == ompi_process_info.ns_replica) {
        if (ORTE_SUCCESS != orte_name_services.create_process_name(ompi_process_info.ns_replica, 0, 0, 0)) {
            return;
        }
	}
	mca_oob_parse_contact_info(ompi_universe_info.ns_replica,
				   ompi_process_info.ns_replica, NULL);
	setenv("OMPI_MCA_ns_base_replica", nsreplica, 1);  /* set the ns_replica enviro variable */
    } /* otherwise, leave it alone */

    /* see if GPR replica provided */
    if (ompi_cmd_line_is_taken(cmd_line, "gprreplica")) {
	if (NULL == ompi_cmd_line_get_param(cmd_line, "gprreplica", 0, 0)) {
	    ompi_output(0, "error retrieving GPR replica - please report error to bugs@open-mpi.org");
	    return;
	}
	gprreplica = strdup(ompi_cmd_line_get_param(cmd_line, "gprreplica", 0, 0));
	if (NULL != ompi_universe_info.gpr_replica) {
	    free(ompi_universe_info.gpr_replica);
	    ompi_universe_info.gpr_replica = NULL;
	}
	ompi_universe_info.gpr_replica = strdup(nsreplica);
	if (NULL == ompi_process_info.gpr_replica) {
        if (ORTE_SUCCESS != orte_name_services.create_process_name(ompi_process_info.gpr_replica, 0, 0, 0)) {
            return;
        }
	}
	mca_oob_parse_contact_info(ompi_universe_info.gpr_replica,
				   ompi_process_info.gpr_replica, NULL);
	setenv("OMPI_MCA_gpr_base_replica", gprreplica, 1);  /* set the gpr_replica enviro variable */
    }  /* otherwise leave it alone */
}

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
 * Parse environmental paramater options for the Open Run Time Environment. This function
 * MUST be called BEFORE calling any of the rte command line parsers, AFTER opening
 * the name services, and BEFORE checking for universe existence.
 *
 * NOTE: Sets all key structure values to defaults if no environ value provided!!
 *
 */
#include "orte_config.h"

#include <string.h>

#include "util/sys_info.h"
#include "util/proc_info.h"

#include "mca/base/base.h"
#include "mca/base/mca_base_param.h"
#include "mca/errmgr/errmgr.h"
#include "mca/ns/ns.h"
#include "mca/rml/rml.h"

#include "runtime/runtime.h"

int orte_parse_environ(void)
{
    int id, rc;

    /* ensure that sys_info and proc_info have been run */
    orte_sys_info();
    orte_proc_info();

    /* collect the universe-specific info */
    if (0 > (id = mca_base_param_register_int("seed", NULL, NULL, NULL, 1))) {
        ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
        return ORTE_ERR_BAD_PARAM;
    }
    if (ORTE_SUCCESS != (rc = mca_base_param_lookup_int(id, (int*)&orte_process_info.seed))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }

    if (0 > (id = mca_base_param_register_string("universe", "uri", NULL, NULL, NULL))) {
        ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
        return ORTE_ERR_BAD_PARAM;
    }
    if (ORTE_SUCCESS != (rc = mca_base_param_lookup_string(id, &orte_universe_info.seed_uri))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
	if (NULL != orte_universe_info.seed_uri) {  /* overwrite */
	   if (ORTE_SUCCESS != (rc = orte_rml.set_uri(orte_universe_info.seed_uri))) {
            ORTE_ERROR_LOG(rc);
            return rc;
       }
    } else {
	   if (NULL != orte_universe_info.seed_uri) {
	       free(orte_universe_info.seed_uri);
	       orte_universe_info.seed_uri = NULL;
	   }
    }

    id = mca_base_param_register_string("universe", "scope", NULL, NULL, "exclusive");
    mca_base_param_lookup_string(id, &orte_universe_info.scope);

    id = mca_base_param_register_int("universe", "persistence", NULL, NULL, 1);
    mca_base_param_lookup_int(id, (int*)&orte_universe_info.persistence);

    id = mca_base_param_register_int("universe", "console", NULL, NULL, 1);
    mca_base_param_lookup_int(id, (int*)&orte_universe_info.console);

    id = mca_base_param_register_string("universe", "script", NULL, NULL, NULL);
    mca_base_param_lookup_string(id, &orte_universe_info.scriptfile);

    id = mca_base_param_register_string("universe", "name", NULL, NULL, "default-universe");
    mca_base_param_lookup_string(id, &orte_universe_info.name);

    id = mca_base_param_register_string("universe", "host", NULL, NULL, NULL);
    mca_base_param_lookup_string(id, &orte_universe_info.host);

    id = mca_base_param_register_string("universe", "host", "uid", NULL, NULL);
    mca_base_param_lookup_string(id, &orte_universe_info.uid);

    /* collect the process-specific info */
    id = mca_base_param_register_string("tmpdir", "base", NULL, NULL, "/tmp");
    mca_base_param_lookup_string(id, &orte_process_info.tmpdir_base);

    id = mca_base_param_register_string("gpr", "replica", "uri", NULL, NULL);
    mca_base_param_lookup_string(id, &orte_process_info.gpr_replica_uri);
    if (NULL != orte_process_info.gpr_replica_uri) {
        if (ORTE_SUCCESS != (rc = orte_rml.set_uri(orte_process_info.gpr_replica_uri))) {
            ORTE_ERROR_LOG(rc);
            return rc;
        }
        /* provide memory for the replica name */
        if (ORTE_SUCCESS != orte_ns.create_process_name(&orte_process_info.gpr_replica, 0, 0, 0)) {
            ORTE_ERROR_LOG(rc);
            return rc;
        }
        /* populate the name with the correct info */
        	if (ORTE_SUCCESS != (rc = orte_rml.parse_uris(orte_process_info.gpr_replica_uri,
        				                    orte_process_info.gpr_replica, NULL))) {
            ORTE_ERROR_LOG(rc);
            return rc;
         }
    } else {
	   if (NULL != orte_process_info.gpr_replica) {
	       free(orte_process_info.gpr_replica);
	       orte_process_info.gpr_replica = NULL;
	   }
    }

    id = mca_base_param_register_string("ns", "replica", "uri", NULL, NULL);
    mca_base_param_lookup_string(id, &orte_process_info.ns_replica_uri);
    if (NULL != orte_process_info.ns_replica_uri) {
        if (ORTE_SUCCESS != (rc = orte_rml.set_uri(orte_process_info.ns_replica_uri))) {
            ORTE_ERROR_LOG(rc);
            return rc;
        }
       /* provide memory for the replica name */
        if (ORTE_SUCCESS != orte_ns.create_process_name(&orte_process_info.ns_replica, 0, 0, 0)) {
            ORTE_ERROR_LOG(rc);
            return rc;
        }
        /* populate the name with the correct info */
        if (ORTE_SUCCESS != (rc = orte_rml.parse_uris(orte_process_info.ns_replica_uri,
                                           orte_process_info.ns_replica, NULL))) {
            ORTE_ERROR_LOG(rc);
            return rc;
        }
    } else {
        	if (NULL != orte_process_info.ns_replica) {
        	    free(orte_process_info.ns_replica);
        	    orte_process_info.ns_replica = NULL;
        	}
    }

    return ORTE_SUCCESS;
}

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
 * Parse command line options for the Open Run Time Environment. This program MUST be called before
 * any call to orte_init, but after orte_parse_environ to ensure that variables are correctly set !!!
 *
 */
#include "orte_config.h"

#include <stdlib.h>
#include <string.h>

#include "mca/base/mca_base_param.h"

#include "mca/rml/rml.h"
#include "mca/ns/ns.h"
#include "mca/errmgr/errmgr.h"

#include "util/output.h"
#include "util/cmd_line.h"
#include "util/sys_info.h"
#include "util/proc_info.h"

#include "runtime/runtime.h"

int orte_parse_cmd_line(ompi_cmd_line_t *cmd_line)
{
    char *universe=NULL, *nsreplica=NULL, *gprreplica=NULL, *tmp=NULL;
    int rc, id;

    /* get universe name and store it, if user specified it */
    /* otherwise, stick with what was obtained from parse_environ */

    if (ompi_cmd_line_is_taken(cmd_line, "universe") ||
	    ompi_cmd_line_is_taken(cmd_line, "u")) {
	   if (NULL == ompi_cmd_line_get_param(cmd_line, "universe", 0, 0)) {
            ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
            return ORTE_ERR_BAD_PARAM;
        }
        universe = strdup(ompi_cmd_line_get_param(cmd_line, "universe", 0, 0));
        if (NULL == universe) {
            ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
            return ORTE_ERR_OUT_OF_RESOURCE;
        }
        
        /* clean out any pre-existing names - we will rebuild as specified */
        if (NULL != orte_universe_info.name) {
            free(orte_universe_info.name);
            orte_universe_info.name = NULL;
         }
         if (NULL != orte_universe_info.host) {
             free(orte_universe_info.host);
             orte_universe_info.host = NULL;
         }
         if (NULL != orte_universe_info.uid) {
              free(orte_universe_info.uid);
              orte_universe_info.uid = NULL;
         }

        /* rebuild and set the appropriate MCA parameters */

	   if (NULL != (tmp = strchr(universe, ':'))) { /* name contains remote host */
	       /* get the host name, and the universe name separated */
	       /* could be in form remote-uid@remote-host:universe */
	       *tmp = '\0';
	       tmp++;
	       orte_universe_info.name = strdup(tmp);
            if (NULL == orte_universe_info.name) {
                ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                return ORTE_ERR_OUT_OF_RESOURCE;
            }
	       if (NULL != (tmp = strchr(universe, '@'))) {  /* remote name includes remote uid */
                *tmp = '\0';
                tmp++;
                orte_universe_info.host = strdup(tmp);
                if (NULL == orte_universe_info.host) {
                    ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                    return ORTE_ERR_OUT_OF_RESOURCE;
                }
                orte_universe_info.uid = strdup(universe);
                if (NULL == orte_universe_info.uid) {
                    ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                    return ORTE_ERR_OUT_OF_RESOURCE;
                }
                if (0 > (id = mca_base_param_register_string("universe", "host", "uid", NULL, NULL))) {
                    ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
                    return ORTE_ERR_BAD_PARAM;
                }
                if (ORTE_SUCCESS != (rc = mca_base_param_lookup_string(id, &orte_universe_info.uid))) {
                    ORTE_ERROR_LOG(rc);
                    return rc;
                }
	       } else {  /* no remote id - just remote host */
		      orte_universe_info.host = strdup(universe);
              if (NULL == orte_universe_info.host) {
                  ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                  return ORTE_ERR_OUT_OF_RESOURCE;
              }
	       }
            if (0 > (id = mca_base_param_register_string("universe", "host", NULL, NULL, NULL))) {
                ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
                return ORTE_ERR_BAD_PARAM;
            }
            if (ORTE_SUCCESS != (rc = mca_base_param_set_string(id, orte_universe_info.name))) {
                ORTE_ERROR_LOG(rc);
                return rc;
            }
	   } else { /* no remote host - just universe name provided */
	       orte_universe_info.name = strdup(universe);
           if (NULL == orte_universe_info.name) {
               ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
               return ORTE_ERR_OUT_OF_RESOURCE;
           }
	   }
        if (0 > (id = mca_base_param_register_string("universe", "name", NULL, NULL, "default-universe"))) {
            ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
            return ORTE_ERR_BAD_PARAM;
        }
        if (ORTE_SUCCESS != (rc = mca_base_param_set_string(id, orte_universe_info.name))) {
            ORTE_ERROR_LOG(rc);
            return rc;
        }
    }

    /* get the temporary directory name for the session directory, if provided on command line */
    if (ompi_cmd_line_is_taken(cmd_line, "tmpdir")) {
        	if (NULL == ompi_cmd_line_get_param(cmd_line, "tmpdir", 0, 0)) {
            ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
        	    return ORTE_ERR_BAD_PARAM;
        	}
        	if (NULL != orte_process_info.tmpdir_base) { /* overwrite it */
        	    free(orte_process_info.tmpdir_base);
        	    orte_process_info.tmpdir_base = NULL;
        	}
        	orte_process_info.tmpdir_base = strdup(ompi_cmd_line_get_param(cmd_line, "tmpdir", 0, 0));
        if (NULL == orte_process_info.tmpdir_base) {
            ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
            return ORTE_ERR_OUT_OF_RESOURCE;
        }
        /* set the corresponding MCA parameter */
        if (0 > (id = mca_base_param_register_string("tmpdir", "base", NULL, NULL, "/tmp"))) {
            ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
            return ORTE_ERR_BAD_PARAM;
        }
        if (ORTE_SUCCESS != (rc = mca_base_param_set_string(id, orte_process_info.tmpdir_base))) {
            ORTE_ERROR_LOG(rc);
            return rc;
        }
    } /* otherwise, leave it alone */

    /* see if GPR replica provided on command line */
    if (ompi_cmd_line_is_taken(cmd_line, "gprreplica")) {
           if (NULL == ompi_cmd_line_get_param(cmd_line, "gprreplica", 0, 0)) {
               ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
               return ORTE_ERR_BAD_PARAM;
         }
          gprreplica = strdup(ompi_cmd_line_get_param(cmd_line, "gprreplica", 0, 0));
          if (NULL == gprreplica) {
              ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
              return ORTE_ERR_OUT_OF_RESOURCE;
          }
          if (NULL == orte_process_info.gpr_replica) {
              if (ORTE_SUCCESS != (rc =
                    orte_ns.create_process_name(&orte_process_info.gpr_replica, 0, 0, 0))) {
                  ORTE_ERROR_LOG(rc);
                  return rc;
              }
           }
            if (ORTE_SUCCESS != (rc = orte_rml.parse_uris(gprreplica,
                          orte_process_info.gpr_replica, NULL))) {
                ORTE_ERROR_LOG(rc);
                return rc;
         }
        if (0 > (id = mca_base_param_register_string("gpr", "replica", "uri", NULL, NULL))) {
            ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
            return ORTE_ERR_BAD_PARAM;
        }
        if (ORTE_SUCCESS != (rc = mca_base_param_set_string(id, orte_process_info.ns_replica_uri))) {
            ORTE_ERROR_LOG(rc);
            return rc;
        }
     }  /* otherwise leave it alone */

    /* see if name services replica provided on command line */
    if (ompi_cmd_line_is_taken(cmd_line, "nsreplica")) {
        	if (NULL == ompi_cmd_line_get_param(cmd_line, "nsreplica", 0, 0)) {
        	    ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
            return ORTE_ERR_BAD_PARAM;
        	}
        	nsreplica = strdup(ompi_cmd_line_get_param(cmd_line, "nsreplica", 0, 0));
         if (NULL == nsreplica) {
            ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
            return ORTE_ERR_OUT_OF_RESOURCE;
         }
        	if (NULL == orte_process_info.ns_replica) {
            if (ORTE_SUCCESS != (rc =
                    orte_ns.create_process_name(&orte_process_info.ns_replica, 0, 0, 0))) {
                ORTE_ERROR_LOG(rc);
                return rc;
            }
        	}
        	if (ORTE_SUCCESS != (rc = orte_rml.parse_uris(nsreplica,
        				   orte_process_info.ns_replica, NULL))) {
            ORTE_ERROR_LOG(rc);
            return rc;
         }
        if (0 > (id = mca_base_param_register_string("ns", "replica", "uri", NULL, NULL))) {
            ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
            return ORTE_ERR_BAD_PARAM;
        }
        if (ORTE_SUCCESS != (rc = mca_base_param_set_string(id, orte_process_info.ns_replica_uri))) {
            ORTE_ERROR_LOG(rc);
            return rc;
        }
    } /* otherwise, leave it alone */
    
    return ORTE_SUCCESS;
}

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
/** @file:
 *
 * The Open MPI general purpose registry - implementation.
 *
 */

/*
 * includes
 */

#include "orte_config.h"

#include "util/output.h"
#include "util/proc_info.h"
#include "mca/ns/ns_types.h"
#include "mca/errmgr/errmgr.h"

#include "gpr_replica_api.h"

int orte_gpr_replica_put(orte_gpr_addr_mode_t mode,
                         int cnt, orte_gpr_value_t **values)
{
    int rc, i;
    int8_t action_taken;
    orte_gpr_value_t *val;
    orte_gpr_replica_segment_t *seg=NULL;
    orte_gpr_replica_itag_t *itags=NULL;

    /* protect ourselves against errors */
    if (NULL == values) {
        	if (orte_gpr_replica_globals.debug) {
        	    ompi_output(0, "[%d,%d,%d] gpr replica: error in input - put rejected",
                                ORTE_NAME_ARGS(*(orte_process_info.my_name)));
        	}
        	return ORTE_ERROR;
    }

    if (orte_gpr_replica_globals.compound_cmd_mode) {
	   return orte_gpr_base_pack_put(orte_gpr_replica_globals.compound_cmd,
				     mode, cnt, values);
    }

    OMPI_THREAD_LOCK(&orte_gpr_replica_globals.mutex);

    for (i=0; i < cnt; i++) {
        val = values[i];
        
        /* find the segment */
        if (ORTE_SUCCESS != (rc = orte_gpr_replica_find_seg(&seg, true, val->segment))) {
            OMPI_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);
            return rc;
        }
    
        /* convert tokens to array of itags */
        /* first, protect against errors - must be at least one token! */
        if (NULL == val->tokens) {
            ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
            OMPI_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);
            return ORTE_ERR_BAD_PARAM;
        }
        if (ORTE_SUCCESS != (rc = orte_gpr_replica_get_itag_list(&itags, seg,
                                            val->tokens, &(val->num_tokens)))) {
            OMPI_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);
            return rc;
        }
    
        if (ORTE_SUCCESS != (rc = orte_gpr_replica_put_fn(mode, seg, itags, val->num_tokens,
    				val->cnt, val->keyvals, &action_taken))) {
            goto CLEANUP;
        }
    
        free(itags);
        itags = NULL;
    }

#if 0    
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_check_subscriptions(seg, action_taken))) {
        goto CLEANUP;
    }

    if (ORTE_SUCCESS != (rc = orte_gpr_replica_check_synchros(seg))) {
        goto CLEANUP;
    }
#endif

CLEANUP:
    /* release list of itags */
    if (NULL != itags) {
	   free(itags);
    }

    OMPI_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);

#if 0    
    if (ORTE_SUCCESS == rc) {
        return orte_gpr_replica_process_callbacks();
    }
#endif

    return rc;

}


int orte_gpr_replica_put_nb(orte_gpr_addr_mode_t addr_mode,
                      int cnt, orte_gpr_value_t **values,
                      orte_gpr_notify_cb_fn_t cbfunc, void *user_tag)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}


int orte_gpr_replica_get(orte_gpr_addr_mode_t addr_mode,
                         char *segment, char **tokens, char **keys,
                         int *cnt, orte_gpr_value_t ***values)
{
    orte_gpr_replica_segment_t *seg=NULL;
    orte_gpr_replica_itag_t *tokentags=NULL, *keytags=NULL;
    int num_tokens=0, num_keys=0, rc;

    *cnt = 0;
    *values = NULL;
    
    /* protect against errors */
    if (NULL == segment) {
	   return ORTE_ERR_BAD_PARAM;
    }

    if (orte_gpr_replica_globals.compound_cmd_mode) {
	   return orte_gpr_base_pack_get(orte_gpr_replica_globals.compound_cmd,
                                        addr_mode, segment, tokens, keys);
    }

    OMPI_THREAD_LOCK(&orte_gpr_replica_globals.mutex);

    /* find the segment */
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_find_seg(&seg, true, segment))) {
        OMPI_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);
        return rc;
    }

    /* convert tokens to array of itags */
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_get_itag_list(&tokentags, seg,
                                                        tokens, &num_tokens))) {
        goto CLEANUP;
    }

    /* convert keys to array of itags */
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_get_itag_list(&keytags, seg,
                                                        keys, &num_keys))) {
        goto CLEANUP;
    }

    if (ORTE_SUCCESS != (rc = orte_gpr_replica_get_fn(addr_mode, seg,
                                            tokentags, num_tokens,
                                            keytags, num_keys,
                                            cnt, values))) {
        goto CLEANUP;
    }
    
CLEANUP:
    if (NULL != tokentags) {
	   free(tokentags);
    }

    if (NULL != keytags) {
      free(keytags);
    }

    OMPI_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);
    return rc;

}


int orte_gpr_replica_get_nb(orte_gpr_addr_mode_t addr_mode,
                                char *segment, char **tokens, char **keys,
                                orte_gpr_notify_cb_fn_t cbfunc, void *user_tag)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

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

#include "dps/dps.h"

#include "mca/ns/ns.h"

#include "gpr_replica_api.h"

int
orte_gpr_replica_subscribe(orte_gpr_addr_mode_t addr_mode,
			  orte_gpr_notify_action_t action,
			  char *segment, char **tokens, char **keys,
               orte_gpr_notify_id_t *local_idtag,
			  orte_gpr_notify_cb_fn_t cb_func, void *user_tag)
{
    int rc;
    orte_gpr_replica_segment_t *seg;
    orte_gpr_replica_itag_t *token_itags, *key_itags;
    int num_tokens, num_keys;
    orte_gpr_notify_id_t remote_idtag;
    orte_gpr_replica_act_sync_t flag;

    *local_idtag = ORTE_GPR_NOTIFY_ID_MAX;
    flag.trig_action = action;
    
    /* protect against errors */
    if (NULL == segment) {
	   return ORTE_ERR_BAD_PARAM;
    }

    OMPI_THREAD_LOCK(&orte_gpr_replica_globals.mutex);

    /* locate the segment */
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_find_seg(&seg, true, segment))) {
        OMPI_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);
        return rc;
    }

    if (orte_gpr_replica_globals.compound_cmd_mode) {

        	if (ORTE_SUCCESS == (rc = orte_gpr_base_pack_subscribe(orte_gpr_replica_globals.compound_cmd,
        				    addr_mode, action,
        				    segment, tokens, keys))) {
        
       	   /* enter request on notify tracking system */
        	   rc = orte_gpr_replica_enter_notify_request(local_idtag, seg,
                                    ORTE_GPR_SUBSCRIBE_CMD, &flag,
                                    NULL, 0, cb_func, user_tag);
        
            if (ORTE_SUCCESS == rc) {
        	       rc = orte_dps.pack(orte_gpr_replica_globals.compound_cmd, local_idtag, 1, ORTE_GPR_NOTIFY_ID);
            }
        }
        OMPI_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);
        return rc;
    }

    /* convert tokens to itags */
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_get_itag_list(&token_itags,
                         seg, tokens, &num_tokens))) {
        OMPI_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);
        return rc;
    }

    /* convert keys to itags */
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_get_itag_list(&key_itags,
                         seg, keys, &num_keys))) {
        OMPI_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);
        return rc;
    }

    /* enter request on notify tracking system */
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_enter_notify_request(local_idtag, seg,
                                    ORTE_GPR_SUBSCRIBE_CMD, &flag,
                                    NULL, 0, cb_func, user_tag))) {
        OMPI_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);
        return rc;
    }

    /* register subscription */
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_subscribe_fn(addr_mode, seg,
				      token_itags, num_tokens, key_itags, num_keys, *local_idtag))) {
        orte_gpr_replica_remove_notify_request(*local_idtag, &remote_idtag);
        OMPI_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);
        return rc;
    }

    /* check subscriptions */
    orte_gpr_replica_check_subscriptions(seg, ORTE_GPR_REPLICA_SUBSCRIBER_ADDED);

    if (NULL != key_itags) {
	   free(key_itags);
    }

    if (NULL != token_itags) {
      free(token_itags);
    }

    OMPI_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);

    if (ORTE_SUCCESS == rc) {
        orte_gpr_replica_process_callbacks();
    }
    
    return rc;
}


int orte_gpr_replica_unsubscribe(orte_gpr_notify_id_t sub_number)
{
    int rc;

    if (orte_gpr_replica_globals.compound_cmd_mode) {
	   return orte_gpr_base_pack_unsubscribe(orte_gpr_replica_globals.compound_cmd,
                        sub_number);
    }

    OMPI_THREAD_LOCK(&orte_gpr_replica_globals.mutex);

    rc = orte_gpr_replica_unsubscribe_fn(sub_number);

    OMPI_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);

    return rc;
}

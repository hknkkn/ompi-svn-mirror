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
 * The Open MPI general purpose registry - support functions.
 *
 */

/*
 * includes
 */

#include "orte_config.h"

#include "mca/errmgr/errmgr.h"
#include "mca/ns/ns.h"

#include "gpr_replica_fn.h"

static int orte_gpr_replica_trig_op_add_target(orte_gpr_replica_triggers_t *trig,
                                               orte_gpr_replica_container_t *cptr,
                                               orte_gpr_replica_itagval_t *iptr);

int
orte_gpr_replica_enter_notify_request(orte_gpr_notify_id_t *local_idtag,
                                      orte_gpr_replica_segment_t *seg,
                                      orte_gpr_cmd_flag_t cmd,
                                      orte_gpr_replica_act_sync_t *flag,
                                      orte_process_name_t *requestor,
                                      orte_gpr_notify_id_t remote_idtag,
                                      orte_gpr_notify_cb_fn_t cb_func,
                                      void *user_tag)
{
    orte_gpr_replica_triggers_t *trig;
    int rc;

    *local_idtag = ORTE_GPR_NOTIFY_ID_MAX;
    
    trig = OBJ_NEW(orte_gpr_replica_triggers_t);
    if (NULL == trig) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }

    trig->cmd = cmd;
    if (ORTE_GPR_SUBSCRIBE_CMD == cmd) {
        trig->flag.trig_action = flag->trig_action;
    } else if (ORTE_GPR_SYNCHRO_CMD == cmd) {
        trig->flag.trig_synchro = flag->trig_synchro;
    } else {
        OBJ_RELEASE(trig);
        ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
        return ORTE_ERR_BAD_PARAM;
    }

    trig->seg = seg;
    if (NULL != requestor) {
        if (ORTE_SUCCESS != (rc = orte_ns.copy_process_name(&(trig->requestor),
                                            requestor))) {
              ORTE_ERROR_LOG(rc);
              return rc;
        }
    } else {
         trig->requestor = NULL;
    }
    trig->remote_idtag = remote_idtag;
    trig->callback = cb_func;
    trig->user_tag = user_tag;

    if (0 > (rc = orte_pointer_array_add(orte_gpr_replica.triggers, trig))) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    trig->local_idtag = (orte_gpr_notify_id_t)rc;
    *local_idtag = trig->local_idtag;

    return ORTE_SUCCESS;
}


int
orte_gpr_replica_remove_notify_request(orte_gpr_notify_id_t local_idtag,
                                       orte_gpr_notify_id_t *remote_idtag)
{
    orte_gpr_replica_triggers_t *trig;

    trig = (orte_gpr_replica_triggers_t*)((orte_gpr_replica.triggers)->addr[local_idtag]);
    if (NULL == trig) {
        ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
        return ORTE_ERR_BAD_PARAM;
    }
    *remote_idtag = trig->remote_idtag;
    OBJ_RELEASE(trig);
    orte_pointer_array_set_item(orte_gpr_replica.triggers, local_idtag, NULL);

    return ORTE_SUCCESS;
}


int orte_gpr_replica_init_trigger(orte_gpr_replica_segment_t *seg,
                                  orte_gpr_replica_triggers_t *trig)
{
    int rc;
    
    /* find all the targets to which this trigger applies */
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_init_targets(seg, trig))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    
    /* if synchro, initialize the count */
    
    /* if subscribe, see if initial data requested - if so, queue callback */
    
    return ORTE_SUCCESS;
}


int orte_gpr_replica_init_targets(orte_gpr_replica_segment_t *seg,
                                  orte_gpr_replica_triggers_t *trig)
{
    int i, j, rc;
    int num_keys, num_tokens;
    orte_gpr_replica_container_t **cptr;
    orte_gpr_replica_itagval_t **iptr;
    
    num_tokens = orte_value_array_get_size(&(trig->tokentags));
    num_keys = orte_value_array_get_size(&(trig->keytags));

    /* if no tokens were provided, then it automatically applies to all
     * containers - if no keys were provided as well, then add all containers
     * and all keyvals to the list via wildcard
     */
    if (0 == num_tokens && 0 == num_keys) {
        if (ORTE_SUCCESS != (rc = orte_gpr_replica_trig_op_add_target(trig, NULL, NULL))) {
            ORTE_ERROR_LOG(rc);
        }
        return rc;
    }
    
    /* if tokens provided, but no keys provided, then this applies only to
     * the specified container(s) - find them and add to the target list
     */
    if (0 == num_keys) {
        cptr = (orte_gpr_replica_container_t**)((seg->containers)->addr);
        for (i=0; i < (seg->containers)->size; i++) {
            if (NULL != cptr[i] && orte_gpr_replica_check_itag_list(trig->token_addr_mode,
                    num_tokens,
                    ORTE_VALUE_ARRAY_GET_BASE(&(trig->tokentags), orte_gpr_replica_itag_t),
                    cptr[i]->num_itags, cptr[i]->itags)) {  /* got this container */
                if (ORTE_SUCCESS != (rc = orte_gpr_replica_trig_op_add_target(trig, cptr[i], NULL))) {
                    return rc;
                }
            }
        }
        return ORTE_SUCCESS;
    }
    
    /* if no tokens specified but keys provided, then this applies to all itagvals
     * regardless of container - find them and add to target list
     */
    if (0 == num_tokens) {
        cptr = (orte_gpr_replica_container_t**)((seg->containers)->addr);
        for (i=0; i < (seg->containers)->size; i++) {
            if (NULL != cptr[i]) {  /* for every container */
                iptr = (orte_gpr_replica_itagval_t**)((cptr[i]->itagvals)->addr);
                for (j=0; j < (cptr[i]->itagvals)->size; j++) {
                    if (NULL != iptr[j] && orte_gpr_replica_check_itag_list(ORTE_GPR_REPLICA_OR,
                            num_keys,
                            ORTE_VALUE_ARRAY_GET_BASE(&(trig->keytags), orte_gpr_replica_itag_t),
                            1, &(iptr[j]->itag))) {  /* got this value */
                        if (ORTE_SUCCESS != (rc = orte_gpr_replica_trig_op_add_target(trig, cptr[i], iptr[j]))) {
                            return rc;
                        }
                    }
                }
            }
        }
        return ORTE_SUCCESS;
    }
    
    /* if both tokens and keys specified, then this applies to specific containers
     * and itagvals - find them and add to the target list
     */
    cptr = (orte_gpr_replica_container_t**)((seg->containers)->addr);
    for (i=0; i < (seg->containers)->size; i++) {
        if (NULL != cptr[i] && orte_gpr_replica_check_itag_list(trig->token_addr_mode,
                    num_tokens,
                    ORTE_VALUE_ARRAY_GET_BASE(&(trig->tokentags), orte_gpr_replica_itag_t),
                    cptr[i]->num_itags, cptr[i]->itags)) {  /* got this container */
             iptr = (orte_gpr_replica_itagval_t**)((cptr[i]->itagvals)->addr);
             for (j=0; j < (cptr[i]->itagvals)->size; j++) {
                 if (NULL != iptr[j] && orte_gpr_replica_check_itag_list(ORTE_GPR_REPLICA_OR,
                            num_keys,
                            ORTE_VALUE_ARRAY_GET_BASE(&(trig->keytags), orte_gpr_replica_itag_t),
                            1, &(iptr[j]->itag))) {  /* got this value */
                     if (ORTE_SUCCESS != (rc = orte_gpr_replica_trig_op_add_target(trig, cptr[i], iptr[j]))) {
                         return rc;
                     }
                 }
            }
        }
    }

    return ORTE_SUCCESS;     
}


static int
orte_gpr_replica_trig_op_add_target(orte_gpr_replica_triggers_t *trig,
                                    orte_gpr_replica_container_t *cptr,
                                    orte_gpr_replica_itagval_t *iptr)
{
    orte_gpr_replica_target_t *target;
    
    target = (orte_gpr_replica_target_t*)malloc(sizeof(orte_gpr_replica_target_t));
    if (NULL == target) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    target->cptr = cptr;
    target->iptr = iptr;
    
    if (0 > orte_pointer_array_add(trig->targets, target)) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    return ORTE_SUCCESS;
}


int orte_gpr_replica_register_callback(orte_gpr_replica_triggers_t *trig)
{
    orte_gpr_replica_callbacks_t *cb;
    int rc;
    
    /* process request */
    cb = OBJ_NEW(orte_gpr_replica_callbacks_t);
    if (NULL == cb) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    /* construct the message */
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_construct_notify_message(&(cb->message), trig))) {
        ORTE_ERROR_LOG(rc);
        OBJ_RELEASE(cb);
        return rc;
    }
    
    /* queue the callback */
    if (NULL == trig->requestor) {  /* local request - queue local callback */
        cb->requestor = NULL;
        cb->cb_func = trig->callback;
        cb->user_tag = trig->user_tag;
        cb->remote_idtag = ORTE_GPR_NOTIFY_ID_MAX;
        if (orte_gpr_replica_globals.debug) {
           ompi_output(0, "[%d,%d,%d] process_trig: queueing local message\n",
                        ORTE_NAME_ARGS(orte_process_info.my_name));
        }
  
    } else {  /* remote request - queue remote callback */
        if (ORTE_SUCCESS != (rc = orte_ns.copy_process_name(&(cb->requestor), trig->requestor))) {
            ORTE_ERROR_LOG(rc);
            OBJ_RELEASE(cb);
            return rc;
        }
        cb->cb_func = NULL;
        cb->user_tag = NULL;
        cb->remote_idtag = trig->remote_idtag;
        if (orte_gpr_replica_globals.debug) {
            ompi_output(0, "[%d,%d,%d] process_trig: queueing message for [%d,%d,%d] with idtag %d using remoteid %d\n",
                   ORTE_NAME_ARGS(orte_process_info.my_name), ORTE_NAME_ARGS(cb->requestor),
                    (int)cb->remote_idtag, (int)trig->remote_idtag);
        }
  
    }
    ompi_list_append(&orte_gpr_replica.callbacks, &cb->item);

    if (orte_gpr_replica_globals.debug) {
        ompi_output(0, "[%d,%d,%d] gpr replica-process_trig: complete",
            ORTE_NAME_ARGS(orte_process_info.my_name));
    }

    return ORTE_SUCCESS;
}


int orte_gpr_replica_construct_notify_message(orte_gpr_notify_message_t **msg,
                                              orte_gpr_replica_triggers_t *trig)
{
    int rc;
    
    /* construct the message */
    *msg = OBJ_NEW(orte_gpr_notify_message_t);
    if (NULL == *msg) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    (*msg)->idtag = trig->local_idtag;
    (*msg)->segment = strdup((trig->seg)->name);
    (*msg)->cmd = trig->cmd;
    if (ORTE_GPR_SYNCHRO_CMD == trig->cmd) {
        (*msg)->flag.trig_action = trig->flag.trig_action;
    } else {
        (*msg)->flag.trig_synchro = trig->flag.trig_synchro;
    }

    /* NEED TO FIX THIS LINE */
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_get_fn(trig->token_addr_mode, trig->seg,
        ORTE_VALUE_ARRAY_GET_BASE(&(trig->tokentags), orte_gpr_replica_itag_t),
        (int)orte_value_array_get_size(&(trig->tokentags)),
        ORTE_VALUE_ARRAY_GET_BASE(&(trig->keytags), orte_gpr_replica_itag_t),
        (int)orte_value_array_get_size(&(trig->keytags)),
        &((*msg)->cnt), &((*msg)->values))))  {
        ORTE_ERROR_LOG(rc);
    }

    return rc;
}


int orte_gpr_replica_check_trigger(orte_gpr_replica_segment_t *seg,
                    orte_gpr_replica_triggers_t *trig,
                    orte_gpr_replica_container_t *cptr,
                    orte_gpr_replica_itagval_t *iptr,
                    int8_t action)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}


int orte_gpr_replica_purge_subscriptions(orte_process_name_t *proc)
{
#if 0
    mca_gpr_replica_segment_t *seg;
    mca_gpr_replica_notify_request_tracker_t *trig, *next;
    mca_gpr_replica_trigger_list_t *trig, *next_trig;
    int cmpval1, cmpval2;

    if (NULL == proc) {  /* protect against errors */
	return OMPI_ERROR;
    }

    /* locate any notification events that have proc as the recipient
     */
    for (trig = (mca_gpr_replica_notify_request_tracker_t*)ompi_list_get_first(&mca_gpr_replica_notify_request_tracker);
	 trig != (mca_gpr_replica_notify_request_tracker_t*)ompi_list_get_end(&mca_gpr_replica_notify_request_tracker);) {
	    next = (mca_gpr_replica_notify_request_tracker_t*)ompi_list_get_next(trig);
        if (ORTE_SUCCESS != orte_name_services.compare(&cmpval1, ORTE_NS_CMP_ALL, proc, trig->requestor)) {
            return OMPI_ERROR;
        }
        if (ORTE_SUCCESS != orte_name_services.compare(&cmpval2, ORTE_NS_CMP_ALL, proc, ompi_rte_get_self())) {
            return OMPI_ERROR;
        }
	if ((NULL != trig->requestor && 0 == cmpval1) ||
	    (NULL == trig->requestor && 0 == cmpval2)) {

	    /* ...find the associated subscription... */
	    if (NULL != trig->segptr) {
		seg = trig->segptr;
		for (trig = (mca_gpr_replica_trigger_list_t*)ompi_list_get_first(&seg->triggers);
		     trig != (mca_gpr_replica_trigger_list_t*)ompi_list_get_end(&seg->triggers);
		     ) {
		    next_trig = (mca_gpr_replica_trigger_list_t*)ompi_list_get_next(trig);
		    if (trig->local_idtag == trig->local_idtag) { /* found it */
			/* ...delete it... */
			ompi_list_remove_item(&seg->triggers, &trig->item);
		    }
		    trig = next_trig;
		}
	    }
	    /* ...and delete me too! */
	    ompi_list_remove_item(&mca_gpr_replica_notify_request_tracker, &trig->item);
	    OBJ_RELEASE(trig);
	}
	trig = next;
    }
#endif
    return ORTE_ERR_NOT_IMPLEMENTED;
}



int orte_gpr_replica_check_synchros(orte_gpr_replica_segment_t *seg)
{
#if 0
    mca_gpr_replica_trigger_list_t *trig;
    ompi_registry_notify_message_t *notify_msg;
    mca_gpr_replica_trigger_list_t* next;
    bool still_valid=false;

    /* search the segment and re-compute the trigger levels */

    /* check for trigger conditions */
    for (trig = (mca_gpr_replica_trigger_list_t*)ompi_list_get_first(&seg->triggers);
	 trig != (mca_gpr_replica_trigger_list_t*)ompi_list_get_end(&seg->triggers);
         ) {
	next = (mca_gpr_replica_trigger_list_t*)ompi_list_get_next(trig);
	still_valid = true;
	if (((OMPI_REGISTRY_SYNCHRO_MODE_ASCENDING & trig->synch_mode)
	     && (trig->count >= trig->trigger)
	     && (MCA_GPR_REPLICA_TRIGGER_BELOW_LEVEL == trig->above_below)) ||
	    ((OMPI_REGISTRY_SYNCHRO_MODE_DESCENDING & trig->synch_mode)
	     && (trig->count <= trig->trigger)
	     && (MCA_GPR_REPLICA_TRIGGER_ABOVE_LEVEL == trig->above_below)) ||
	    (OMPI_REGISTRY_SYNCHRO_MODE_LEVEL & trig->synch_mode && trig->count == trig->trigger) ||
	    (OMPI_REGISTRY_SYNCHRO_MODE_GT_EQUAL & trig->synch_mode && trig->count >= trig->trigger)) {

		if (mca_gpr_replica_debug) {
			ompi_output(0, "[%d,%d,%d] synchro fired on segment %s trigger level %d",
						ORTE_NAME_ARGS(orte_process_info.my_name), seg->name, trig->trigger);
		}
	    notify_msg = mca_gpr_replica_construct_notify_message(seg, trig);
	    notify_msg->trig_action = OMPI_REGISTRY_NOTIFY_NONE;
	    notify_msg->trig_synchro = trig->synch_mode;
	    still_valid = mca_gpr_replica_process_triggers(seg, trig, notify_msg);

	}
    /* if one-shot, remove request from tracking system */
    if ((OMPI_REGISTRY_SYNCHRO_MODE_ONE_SHOT & trig->synch_mode) ||
    (OMPI_REGISTRY_NOTIFY_ONE_SHOT & trig->action)) {
  ompi_list_remove_item(&mca_gpr_replica_notify_request_tracker, &trig->item);
   OBJ_RELEASE(trig);

	if (still_valid) {
	    if (trig->count > trig->trigger) {
		trig->above_below = MCA_GPR_REPLICA_TRIGGER_ABOVE_LEVEL;
	    } else if (trig->count == trig->trigger) {
		trig->above_below = MCA_GPR_REPLICA_TRIGGER_AT_LEVEL;
	    }
	}
	trig = next;
    }
    return OMPI_SUCCESS;
#endif
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_gpr_replica_check_subscriptions(orte_gpr_replica_segment_t *seg, int8_t action_taken)
{
#if 0
    mca_gpr_replica_trigger_list_t *trig;
    ompi_registry_notify_message_t *notify_msg;
    mca_gpr_replica_trigger_list_t* next;
    bool still_valid=false;

    if (!seg->triggers_active) {  /* triggers are not active */
	return;
    }

    for (trig = (mca_gpr_replica_trigger_list_t*)ompi_list_get_first(&seg->triggers);
	 trig != (mca_gpr_replica_trigger_list_t*)ompi_list_get_end(&seg->triggers);
         ) {
	next = (mca_gpr_replica_trigger_list_t*)ompi_list_get_next(trig);
	if ((OMPI_REGISTRY_NOTIFY_ALL & trig->action) ||
	    ((OMPI_REGISTRY_NOTIFY_ADD_ENTRY & trig->action) && (MCA_GPR_REPLICA_OBJECT_ADDED == action_taken)) ||
	    ((OMPI_REGISTRY_NOTIFY_MODIFICATION & trig->action) && (MCA_GPR_REPLICA_OBJECT_UPDATED == action_taken)) ||
	    ((OMPI_REGISTRY_NOTIFY_DELETE_ENTRY & trig->action) && (MCA_GPR_REPLICA_OBJECT_DELETED == action_taken)) ||
	    ((OMPI_REGISTRY_NOTIFY_ADD_SUBSCRIBER & trig->action) && (MCA_GPR_REPLICA_SUBSCRIBER_ADDED == action_taken))) {
		if (mca_gpr_replica_debug) {
			ompi_output(0, "[%d,%d,%d] trigger fired on segment %s",
						ORTE_NAME_ARGS(orte_process_info.my_name), seg->name);
		}
	    notify_msg = mca_gpr_replica_construct_notify_message(seg, trig);
	    notify_msg->trig_action = trig->action;
	    notify_msg->trig_synchro = OMPI_REGISTRY_SYNCHRO_MODE_NONE;
	    still_valid = mca_gpr_replica_process_triggers(seg, trig, notify_msg);
	}
	if (still_valid) {
	    if (trig->count > trig->trigger) {
		trig->above_below = MCA_GPR_REPLICA_TRIGGER_ABOVE_LEVEL;
	    } else if (trig->count == trig->trigger) {
		trig->above_below = MCA_GPR_REPLICA_TRIGGER_AT_LEVEL;
	    }
	}
	trig = next;
    }
#endif
    return ORTE_ERR_NOT_IMPLEMENTED;
}

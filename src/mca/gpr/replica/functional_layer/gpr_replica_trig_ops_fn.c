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

#include "mca/ns/ns.h"

#include "gpr_replica_fn.h"


int
orte_gpr_replica_remove_trigger(orte_gpr_notify_id_t idtag)
{
    orte_gpr_replica_notify_tracker_t *trackptr;
    int rc;

    /* find request on notify tracking system */
    trackptr = (orte_gpr_replica_notify_tracker_t*)((orte_gpr_replica.triggers)->addr[idtag]);
    if (NULL == trackptr) {
        return ORTE_ERR_BAD_PARAM;
    }
    
    /* purge this trigger from the specified segment */
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_purge_trigger(trackptr->segptr, idtag))) {
        return rc;
    }
    
    /* remove trigger */
    return orte_pointer_array_set_item(orte_gpr_replica.triggers, idtag, NULL);
}


int
orte_gpr_replica_purge_trigger(orte_gpr_replica_segment_t *seg, orte_gpr_notify_id_t idtag)
{
    int i, j, k, m;
    orte_gpr_replica_container_t **cptr;
    orte_gpr_replica_itagval_t **iptr;
    
    /* check the segment itself */
    j = (int)orte_value_array_get_size(&(seg->triggers));
    for (i=0; i < j; i++) {
        if (idtag == ORTE_VALUE_ARRAY_GET_ITEM(&(seg->triggers), orte_gpr_notify_id_t, i)) {
           orte_value_array_remove_item(&(seg->triggers), i);
           break;
        }
    }
    
    /* for each container */
    cptr = (orte_gpr_replica_container_t**)((seg->containers)->addr);
    for (i=0; i < (seg->containers)->size; i++) {
        if (NULL != cptr[i]) {
            /* check the container's triggers */
            j = (int)orte_value_array_get_size(&(cptr[i]->triggers));
            for (k=0; k < j; k++) {
                if (idtag == ORTE_VALUE_ARRAY_GET_ITEM(&(cptr[i]->triggers),
                                                orte_gpr_notify_id_t, k)) {
                    orte_value_array_remove_item(&(cptr[i]->triggers), k);
                    break;
                }
            }
            /* for each itagval in the container */
            iptr = (orte_gpr_replica_itagval_t**)((cptr[i]->itagvals)->addr);
            for (j=0; j < (cptr[i]->itagvals)->size; j++) {
                if (NULL != iptr[j]) {
                    /* check the itagval's triggers */
                    k = (int)orte_value_array_get_size(&(iptr[j]->triggers));
                    for (m=0; m < k; m++) {
                        if (idtag == ORTE_VALUE_ARRAY_GET_ITEM(&(iptr[j]->triggers),
                                                    orte_gpr_notify_id_t, m)) {
                            orte_value_array_remove_item(&(iptr[j]->triggers), m);
                            break;
                        }
                    }
                }
            }
        }
    }
    
    return ORTE_SUCCESS;
}


orte_gpr_notify_message_t
*orte_gpr_replica_construct_notify_message(orte_gpr_replica_segment_t *seg,
                    orte_gpr_replica_notify_tracker_t *trig)
{
#if 0
    ompi_list_t *reg_entries;
    ompi_registry_value_t *reg, *obj;
    ompi_registry_notify_message_t *msg;
    mca_gpr_replica_key_t *keyptr;
    char **tokptr;
    int i;

    if (mca_gpr_replica_debug) {
	ompi_output(0, "[%d,%d,%d] notification message being created for trig on segment %s",
				ORTE_NAME_ARGS(*ompi_rte_get_self()), seg->name);
    }

    reg_entries = OBJ_NEW(ompi_list_t);
    mca_gpr_replica_get_nl(reg_entries, trig->addr_mode, seg, trig->keys, trig->num_keys);

    msg = OBJ_NEW(ompi_registry_notify_message_t);
    msg->segment = strdup(seg->name);
    msg->owning_job = seg->owning_job;
    msg->num_tokens = trig->num_keys;
    if(0 < trig->num_keys) {
        msg->tokens = (char**)malloc(trig->num_keys*(sizeof(char*)));
	keyptr = trig->keys;
	tokptr = msg->tokens;
	for (i=0; i < (int)msg->num_tokens; i++, keyptr++, tokptr++) {
	    *tokptr = mca_gpr_replica_get_token(seg, *keyptr);
	}
    } else {
        msg->tokens = NULL;
    }
 
   while (NULL != (reg = (ompi_registry_value_t*)ompi_list_remove_first(reg_entries))) {
	obj = OBJ_NEW(ompi_registry_value_t);
	obj->object = (ompi_registry_object_t)malloc(reg->object_size);
	memcpy(obj->object, reg->object, reg->object_size);
	obj->object_size = reg->object_size;
	ompi_list_append(&msg->data, &obj->item);
	OBJ_RELEASE(reg);
    }

    OBJ_RELEASE(reg_entries);
    if (mca_gpr_replica_debug) {
	ompi_output(0, "[%d,%d,%d] gpr replica-construct_notify: msg built",
                    ORTE_NAME_ARGS(*ompi_rte_get_self()));
    }

    return msg;
#endif
    return NULL;
}

bool orte_gpr_replica_process_triggers(orte_gpr_replica_segment_t *seg,
                      orte_gpr_replica_notify_tracker_t *trig,
                   orte_gpr_notify_message_t *message)
{
#if 0
    mca_gpr_replica_notify_request_tracker_t *trackptr;
    bool found;
    mca_gpr_replica_callbacks_t *cb;

    if (mca_gpr_replica_debug) {
	ompi_output(0, "[%d,%d,%d] gpr replica: process_trig entered",
                    ORTE_NAME_ARGS(*ompi_rte_get_self()));
    }

    /* find corresponding notify request */
    found = false;
    for (trackptr = (mca_gpr_replica_notify_request_tracker_t*)ompi_list_get_first(&mca_gpr_replica_notify_request_tracker);
	 trackptr != (mca_gpr_replica_notify_request_tracker_t*)ompi_list_get_end(&mca_gpr_replica_notify_request_tracker);
	 trackptr = (mca_gpr_replica_notify_request_tracker_t*)ompi_list_get_next(trackptr)) {
        	if (trackptr->local_idtag == trig->local_idtag) {
        	    found = true;
        	    break;
        	}
    }

    if (!found) {  /* didn't find request */
	ompi_output(0, "Notification error - request not found");
	return true;
    }

    /* process request */
    cb = OBJ_NEW(mca_gpr_replica_callbacks_t);
    if (NULL == trackptr->requestor) {  /* local request - queue callback fn with their tag */
		cb->requestor = NULL;
		cb->cb_func = trackptr->callback;
		cb->user_tag = trackptr->user_tag;
		cb->message = message;
		cb->remote_idtag = OMPI_REGISTRY_NOTIFY_ID_MAX;
		if (mca_gpr_replica_debug) {
			ompi_output(0, "[%d,%d,%d] process_trig: queueing local message\n",
						ORTE_NAME_ARGS(*ompi_rte_get_self()));
		}
	
    } else {  /* remote request - queue remote callback */
        if (ORTE_SUCCESS != orte_name_services.copy_process_name(cb->requestor, trackptr->requestor)) {
            return false;
        }
	cb->cb_func = NULL;
	cb->user_tag = NULL;
	cb->message = message;
	cb->remote_idtag = trackptr->remote_idtag;
	if (mca_gpr_replica_debug) {
		ompi_output(0, "[%d,%d,%d] process_trig: queueing message for [%d,%d,%d] with idtag %d using remoteid %d\n",
					ORTE_NAME_ARGS(*ompi_rte_get_self()), ORTE_NAME_ARGS(*(cb->requestor)),
					(int)cb->remote_idtag, (int)trackptr->remote_idtag);
	}
	
    }
    ompi_list_append(&mca_gpr_replica_callbacks, &cb->item);

    /* if one-shot, remove request from tracking system */
    if ((OMPI_REGISTRY_SYNCHRO_MODE_ONE_SHOT & trig->synch_mode) ||
	(OMPI_REGISTRY_NOTIFY_ONE_SHOT & trig->action)) {
	ompi_list_remove_item(&mca_gpr_replica_notify_request_tracker, &trackptr->item);
	OBJ_RELEASE(trackptr);

	/* ....and from the corresponding registry segment */
	ompi_list_remove_item(&seg->triggers, &trig->item);
	OBJ_RELEASE(trig);
    }
    if (mca_gpr_replica_debug) {
	ompi_output(0, "[%d,%d,%d] gpr replica-process_trig: complete",
		    ORTE_NAME_ARGS(*ompi_rte_get_self()));
    }

    return false;

#endif
    return false;
}


int orte_gpr_replica_purge_subscriptions(orte_process_name_t *proc)
{
#if 0
    mca_gpr_replica_segment_t *seg;
    mca_gpr_replica_notify_request_tracker_t *trackptr, *next;
    mca_gpr_replica_trigger_list_t *trig, *next_trig;
    int cmpval1, cmpval2;

    if (NULL == proc) {  /* protect against errors */
	return OMPI_ERROR;
    }

    /* locate any notification events that have proc as the recipient
     */
    for (trackptr = (mca_gpr_replica_notify_request_tracker_t*)ompi_list_get_first(&mca_gpr_replica_notify_request_tracker);
	 trackptr != (mca_gpr_replica_notify_request_tracker_t*)ompi_list_get_end(&mca_gpr_replica_notify_request_tracker);) {
	    next = (mca_gpr_replica_notify_request_tracker_t*)ompi_list_get_next(trackptr);
        if (ORTE_SUCCESS != orte_name_services.compare(&cmpval1, ORTE_NS_CMP_ALL, proc, trackptr->requestor)) {
            return OMPI_ERROR;
        }
        if (ORTE_SUCCESS != orte_name_services.compare(&cmpval2, ORTE_NS_CMP_ALL, proc, ompi_rte_get_self())) {
            return OMPI_ERROR;
        }
	if ((NULL != trackptr->requestor && 0 == cmpval1) ||
	    (NULL == trackptr->requestor && 0 == cmpval2)) {

	    /* ...find the associated subscription... */
	    if (NULL != trackptr->segptr) {
		seg = trackptr->segptr;
		for (trig = (mca_gpr_replica_trigger_list_t*)ompi_list_get_first(&seg->triggers);
		     trig != (mca_gpr_replica_trigger_list_t*)ompi_list_get_end(&seg->triggers);
		     ) {
		    next_trig = (mca_gpr_replica_trigger_list_t*)ompi_list_get_next(trig);
		    if (trackptr->local_idtag == trig->local_idtag) { /* found it */
			/* ...delete it... */
			ompi_list_remove_item(&seg->triggers, &trig->item);
		    }
		    trig = next_trig;
		}
	    }
	    /* ...and delete me too! */
	    ompi_list_remove_item(&mca_gpr_replica_notify_request_tracker, &trackptr->item);
	    OBJ_RELEASE(trackptr);
	}
	trackptr = next;
    }
#endif
    return ORTE_ERR_NOT_IMPLEMENTED;
}


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
    orte_gpr_replica_notify_tracker_t *trackptr;
    int rc;

    *local_idtag = ORTE_GPR_NOTIFY_ID_MAX;
    
    trackptr = OBJ_NEW(orte_gpr_replica_notify_tracker_t);
    if (NULL == trackptr) {
        return ORTE_ERR_OUT_OF_RESOURCE;
    }

    trackptr->cmd = cmd;
    if (ORTE_GPR_SUBSCRIBE_CMD == cmd) {
        trackptr->flag.trig_action = flag->trig_action;
    } else if (ORTE_GPR_SYNCHRO_CMD == cmd) {
        trackptr->flag.trig_synchro = flag->trig_synchro;
    } else {
        OBJ_RELEASE(trackptr);
        return ORTE_ERR_BAD_PARAM;
    }

    trackptr->segptr = seg;
    if (NULL != requestor) {
        if (ORTE_SUCCESS != (rc = orte_ns.copy_process_name(&(trackptr->requestor),
                                            requestor))) {
              return rc;
        }
    } else {
    		trackptr->requestor = NULL;
    }
    trackptr->remote_idtag = remote_idtag;
    trackptr->callback = cb_func;
    trackptr->user_tag = user_tag;

    if (0 > (rc = orte_pointer_array_add(orte_gpr_replica.triggers, trackptr))) {
        return rc;
    }
    
    trackptr->local_idtag = (orte_gpr_notify_id_t)rc;
    *local_idtag = trackptr->local_idtag;

    return ORTE_SUCCESS;
}


int
orte_gpr_replica_remove_notify_request(orte_gpr_notify_id_t local_idtag,
                                       orte_gpr_notify_id_t *remote_idtag)
{
    orte_gpr_replica_notify_tracker_t *trackptr;

    trackptr = (orte_gpr_replica_notify_tracker_t*)((orte_gpr_replica.triggers)->addr[local_idtag]);
    if (NULL == trackptr) {
        return ORTE_ERR_BAD_PARAM;
    }
    *remote_idtag = trackptr->remote_idtag;
    OBJ_RELEASE(trackptr);
    orte_pointer_array_set_item(orte_gpr_replica.triggers, local_idtag, NULL);

    return ORTE_SUCCESS;
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
						ORTE_NAME_ARGS(*ompi_rte_get_self()), seg->name, trig->trigger);
		}
	    notify_msg = mca_gpr_replica_construct_notify_message(seg, trig);
	    notify_msg->trig_action = OMPI_REGISTRY_NOTIFY_NONE;
	    notify_msg->trig_synchro = trig->synch_mode;
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
						ORTE_NAME_ARGS(*ompi_rte_get_self()), seg->name);
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

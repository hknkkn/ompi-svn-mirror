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

#include "mca/gpr/replica/transition_layer/gpr_replica_tl.h"
#include "gpr_replica_fn.h"

static int orte_gpr_replica_trig_op_add_target(orte_gpr_replica_triggers_t *trig,
                                               orte_gpr_replica_subscribed_data_t *data,
                                               orte_gpr_replica_container_t *cptr,
                                               orte_gpr_replica_itagval_t *iptr);

int
orte_gpr_replica_enter_notify_request(orte_gpr_notify_id_t *local_idtag,
                                      orte_process_name_t *requestor,
                                      orte_gpr_notify_id_t remote_idtag,
                                      int cnt, orte_gpr_subscription_t **subscriptions)
{
    orte_gpr_replica_triggers_t *trig;
    int rc, i;
    orte_gpr_replica_subscribed_data_t *data;
    
    *local_idtag = ORTE_GPR_NOTIFY_ID_MAX;
    
    trig = OBJ_NEW(orte_gpr_replica_triggers_t);
    if (NULL == trig) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }

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

    for (i=0; i < cnt; i++) {
        data = OBJ_NEW(orte_gpr_replica_subscribed_data_t);
        if (NULL == data) {
            ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
            return ORTE_ERR_OUT_OF_RESOURCE;
        }
        /* store the callback function and user_tag pointers */
        data->callback = subscriptions[i]->cbfunc;
        data->user_tag = subscriptions[i]->user_tag;
        /* add the object to the trigger's subscribed_data pointer array */
        if (0 > (rc = orte_pointer_array_add(trig->subscribed_data, data))) {
            ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
            return ORTE_ERR_OUT_OF_RESOURCE;
        }
        data->index = i;
    }
        
    if (0 > (rc = orte_pointer_array_add(orte_gpr_replica.triggers, trig))) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    trig->index = rc;
    *local_idtag = (orte_gpr_notify_id_t)rc;

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


int orte_gpr_replica_init_trigger(orte_gpr_replica_triggers_t *trig)
{
    int rc;
    
    /* find all the targets to which this trigger applies */
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_init_targets(trig))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
        
    /* if subscribe, see if initial data requested - if so, queue callback */
    
    return ORTE_SUCCESS;
}


int orte_gpr_replica_init_targets(orte_gpr_replica_triggers_t *trig)
{
    int i, j, m, rc;
    int num_keys, num_tokens;
    orte_gpr_replica_segment_t *seg;
    orte_gpr_replica_container_t **cptr;
    orte_gpr_replica_itagval_t **iptr;
    orte_gpr_replica_subscribed_data_t **data;
    
    data = (orte_gpr_replica_subscribed_data_t**)((trig->subscribed_data)->addr);
    for (m=0; m < (trig->subscribed_data)->size; m++) {
        if (NULL != data[m]) {
         
            num_tokens = orte_value_array_get_size(&(data[m]->tokentags));
            num_keys = orte_value_array_get_size(&(data[m]->keytags));
            seg = data[m]->seg;
            
            /* if no tokens were provided, then it automatically applies to all
             * containers - if no keys were provided as well, then add all containers
             * and all keyvals to the list via wildcard
             */
            if (0 == num_tokens && 0 == num_keys) {
                cptr = (orte_gpr_replica_container_t**)((seg->containers)->addr);
                for (i=0; i < (seg->containers)->size; i++) {
                    if (NULL != cptr[i]) {  /* get all containers - add all their values */
                        if (0 < cptr[i]->num_itags) { /* there is some data in container */
                            iptr = (orte_gpr_replica_itagval_t**)((cptr[i]->itagvals)->addr);
                            for (j=0; j < (cptr[i]->itagvals)->size; j++) {
                                if (NULL != iptr[j]) {
                                    if (ORTE_SUCCESS != (rc = orte_gpr_replica_trig_op_add_target(trig, data[m], cptr[i], iptr[j]))) {
                                        ORTE_ERROR_LOG(rc);
                                        return rc;
                                    }
                                }
                            }
                        }
                    }
                }
                return ORTE_SUCCESS;
            }
            
            /* if tokens provided, but no key/values provided, then this applies only to
             * the specified container(s) - find them and add to the target list
             */
            if (0 == num_keys) {
                cptr = (orte_gpr_replica_container_t**)((seg->containers)->addr);
                for (i=0; i < (seg->containers)->size; i++) {
                    if (NULL != cptr[i] && orte_gpr_replica_check_itag_list(data[m]->token_addr_mode,
                            num_tokens,
                            ORTE_VALUE_ARRAY_GET_BASE(&(data[m]->tokentags), orte_gpr_replica_itag_t),
                            cptr[i]->num_itags, cptr[i]->itags)) {  /* got this container - add all its values */
                        if (0 < cptr[i]->num_itags) { /* there is some data in container */
                            iptr = (orte_gpr_replica_itagval_t**)((cptr[i]->itagvals)->addr);
                            for (j=0; j < (cptr[i]->itagvals)->size; j++) {
                                if (NULL != iptr[j]) {
                                    if (ORTE_SUCCESS != (rc = orte_gpr_replica_trig_op_add_target(trig, data[m], cptr[i], iptr[j]))) {
                                        ORTE_ERROR_LOG(rc);
                                        return rc;
                                    }
                                }
                            }
                        }
                    }
                }
                return ORTE_SUCCESS;
            }
            
            /* if no tokens specified but key/values provided, then this applies to all itagvals
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
                                    ORTE_VALUE_ARRAY_GET_BASE(&(data[m]->keytags), orte_gpr_replica_itag_t),
                                    1, &(iptr[j]->itag))) {  /* got this value */
                                if (ORTE_SUCCESS != (rc = orte_gpr_replica_trig_op_add_target(trig, data[m], cptr[i], iptr[j]))) {
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
                if (NULL != cptr[i] && orte_gpr_replica_check_itag_list(data[m]->token_addr_mode,
                            num_tokens,
                            ORTE_VALUE_ARRAY_GET_BASE(&(data[m]->tokentags), orte_gpr_replica_itag_t),
                            cptr[i]->num_itags, cptr[i]->itags)) {  /* got this container */
                     iptr = (orte_gpr_replica_itagval_t**)((cptr[i]->itagvals)->addr);
                     for (j=0; j < (cptr[i]->itagvals)->size; j++) {
                         if (NULL != iptr[j] && orte_gpr_replica_check_itag_list(data[m]->key_addr_mode,
                                    num_keys,
                                    ORTE_VALUE_ARRAY_GET_BASE(&(data[m]->keytags), orte_gpr_replica_itag_t),
                                    1, &(iptr[j]->itag))) {  /* got this value */
                             if (ORTE_SUCCESS != (rc = orte_gpr_replica_trig_op_add_target(trig, data[m], cptr[i], iptr[j]))) {
                                 return rc;
                             }
                         }
                    }
                }
            }
        }
    }

    return ORTE_SUCCESS;     
}


static int
orte_gpr_replica_trig_op_add_target(orte_gpr_replica_triggers_t *trig,
                                    orte_gpr_replica_subscribed_data_t *data,
                                    orte_gpr_replica_container_t *cptr,
                                    orte_gpr_replica_itagval_t *iptr)
{
    orte_gpr_replica_target_t *target, **targets;
    bool found;
    int i;
    
    /* see if this container is already on the list */
    targets = (orte_gpr_replica_target_t**)((data->targets)->addr);
    found = false;
    for (i=0; i < (data->targets)->size && !found; i++) {
        if (NULL != targets[i] && cptr == targets[i]->cptr) {
            /* container found - just add this iptr to it's array */
            if (0 > orte_pointer_array_add(targets[i]->ivals, iptr)) {
                ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                return ORTE_ERR_OUT_OF_RESOURCE;
            }
            (targets[i]->num_ivals)++;
            found = true;
        }
    }
    
    if (!found) {  /* container not found - add another to the target array */
        target = OBJ_NEW(orte_gpr_replica_target_t);
        if (NULL == target) {
            ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
            return ORTE_ERR_OUT_OF_RESOURCE;
        }
        target->cptr = cptr;
        if (0 > orte_pointer_array_add(data->targets, target)) {
            ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
            return ORTE_ERR_OUT_OF_RESOURCE;
        }
        (data->num_targets)++;
        target->num_ivals = 1;
        if (0 > orte_pointer_array_add(target->ivals, iptr)) {
            ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
            return ORTE_ERR_OUT_OF_RESOURCE;
        }
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
    int rc=ORTE_SUCCESS;
    orte_gpr_replica_target_t **targets;
    orte_gpr_notify_data_t **data;
    orte_gpr_replica_subscribed_data_t **sptr;
    orte_gpr_keyval_t **kptr;
    orte_gpr_replica_itagval_t **iptr;
    int i, j, k, m, n, p;
    
    /* construct the message */
    *msg = OBJ_NEW(orte_gpr_notify_message_t);
    if (NULL == *msg) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    if (NULL == trig->requestor) {  /* local recipient */
        (*msg)->idtag = (orte_gpr_notify_id_t)(trig->index);
    } else {  /* remote recipient */
        (*msg)->idtag = trig->remote_idtag;
    }

    /* if we have data, allocate the data objects for the message */
    if (0 < trig->num_subscribed_data) {
        (*msg)->data = (orte_gpr_notify_data_t**)malloc(trig->num_subscribed_data *
                                                        sizeof(orte_gpr_notify_data_t*));
        if (NULL == (*msg)->data) {
            ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
            return ORTE_ERR_OUT_OF_RESOURCE;
        }
        (*msg)->cnt = trig->num_subscribed_data;
    } else {
        return ORTE_SUCCESS;
    }
    
    data = (*msg)->data;
    sptr = (orte_gpr_replica_subscribed_data_t**)((trig->subscribed_data)->addr);
    for (i=0, j=0; i < (trig->subscribed_data)->size; i++) {
        if (NULL != sptr[i]) {
            data[j] = OBJ_NEW(orte_gpr_notify_data_t);
            if (NULL == data[j]) {
                ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                return ORTE_ERR_OUT_OF_RESOURCE;
            }
            /* for each data object, store the callback_number, addressing mode, and name
             * of the segment this data came from
             */
            data[j]->cb_num = sptr[i]->index;
            data[j]->addr_mode = (sptr[i]->key_addr_mode << 8) || sptr[i]->token_addr_mode;
            data[j]->segment = strdup((sptr[i]->seg)->name);
            if (NULL == data[j]->segment) {
                ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                return ORTE_ERR_OUT_OF_RESOURCE;
            }
            /* allocate the storage for the values being returned */
            data[j]->cnt = sptr[i]->num_targets;
            if (0 < data[j]->cnt) {  /* ensure we have some data to return */
                data[j]->values = (orte_gpr_value_t**)malloc(data[j]->cnt * sizeof(orte_gpr_value_t*));
                if (NULL == data[j]->values) {
                    ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                    return ORTE_ERR_OUT_OF_RESOURCE;
                }
                /* cycle through its targets, setting up a new value for each unique
                 * container and copying the respective itagvals into it
                 */
                targets = (orte_gpr_replica_target_t**)((sptr[i]->targets)->addr);
                for (m=0, k=0; m < (sptr[i]->targets)->size; m++) {
                    if (NULL != targets[m]) {
                        data[j]->values[k] = OBJ_NEW(orte_gpr_value_t);
                        if (NULL == data[j]->values[k]) {
                            ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                            return ORTE_ERR_OUT_OF_RESOURCE;
                        }
                        /* record the addressing mode */
                        (data[j]->values[k])->addr_mode = data[j]->addr_mode;
                        /* record the segment these values came from */
                        (data[j]->values[k])->segment = strdup((sptr[i]->seg)->name);
                        if (NULL == ((data[j]->values[k])->segment)) {
                            ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                            return ORTE_ERR_OUT_OF_RESOURCE;
                        }
                        /* record the tokens describing the container */
                        (data[j]->values[k])->num_tokens = (targets[m]->cptr)->num_itags;
                        (data[j]->values[k])->tokens = (char **)malloc((targets[m]->cptr)->num_itags * sizeof(char*));
                        if (NULL == (data[j]->values[k])->tokens) {
                            ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                            return ORTE_ERR_OUT_OF_RESOURCE;
                        }
                        for (n=0; n < (targets[m]->cptr)->num_itags; n++) {
                            if (ORTE_SUCCESS != (rc = orte_gpr_replica_dict_reverse_lookup(
                                                        &((data[j]->values[k])->tokens[n]), sptr[i]->seg,
                                                        (targets[m]->cptr)->itags[n]))) {
                                ORTE_ERROR_LOG(rc);
                                return rc;
                            }
                        }
                        /* record the values to be returned */
                        (data[j]->values[k])->cnt = targets[m]->num_ivals;
                        if (0 < (data[j]->values[k])->cnt) {  /* there are values to return */
                            (data[j]->values[k])->keyvals = (orte_gpr_keyval_t**)malloc((data[j]->values[k])->cnt * sizeof(orte_gpr_keyval_t*));
                            if (NULL == (data[j]->values[k])->keyvals) {
                                ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                                return ORTE_ERR_OUT_OF_RESOURCE;
                            }
                            
                            kptr = (data[j]->values[k])->keyvals;
                            iptr = (orte_gpr_replica_itagval_t**)((targets[m]->ivals)->addr);
                            for (n=0, p=0; n < (targets[m]->ivals)->size; n++) {
                                if (NULL != iptr[n]) {
                                    kptr[p] = OBJ_NEW(orte_gpr_keyval_t);
                                    if (NULL == kptr[p]) {
                                        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                                        return ORTE_ERR_OUT_OF_RESOURCE;
                                    }
                                    if (ORTE_SUCCESS != (rc = orte_gpr_replica_dict_reverse_lookup(
                                                            &(kptr[p]->key), sptr[i]->seg, iptr[n]->itag))) {
                                        ORTE_ERROR_LOG(rc);
                                        return rc;
                                    }
                                    kptr[p]->type = iptr[n]->type;
                                    if (ORTE_SUCCESS != (rc = orte_gpr_replica_xfer_payload(
                                                &(kptr[p]->value), &(iptr[n]->value), iptr[n]->type))) {
                                        ORTE_ERROR_LOG(rc);
                                        return rc;
                                    }
                                    p++;
                                } /* if iptr not NULL */
                            }  /* for n */
                        }  /* if values to return */
                        k++;
                    }  /* if targets not NULL */
                }  /* for m */
            }
            j++;
        }  /* if sptr not NULL */
    }  /* for i */

    return ORTE_SUCCESS;
}


int orte_gpr_replica_check_entry_update(orte_gpr_replica_segment_t *seg,
                    orte_gpr_value_t *old_value,
                    orte_gpr_value_t *new_value,
                    int8_t *action)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}


int orte_gpr_replica_check_subscriptions(orte_gpr_replica_segment_t *seg, int8_t action_taken)
{
    orte_gpr_replica_triggers_t **trig;
    orte_gpr_replica_subscribed_data_t **sptr;
    int i, j, n, rc;

    trig = (orte_gpr_replica_triggers_t**)((orte_gpr_replica.triggers)->addr);
    for (i=0; i < (orte_gpr_replica.triggers)->size; i++) {
        if (NULL != trig[i]) {
            sptr = (orte_gpr_replica_subscribed_data_t**)((trig[i]->subscribed_data)->addr);
            n = (trig[i]->subscribed_data)->size;
            for (j=0; j < n; j++) {
                if (NULL != sptr[j] && seg == sptr[j]->seg) {
                    if (ORTE_GPR_NOTIFY_ANY & trig[i]->action &&
                        !(ORTE_GPR_TRIG_NOTIFY_START & trig[i]->action)) { /* notify exists and is active */
                        if (((ORTE_GPR_NOTIFY_ADD_ENTRY & trig[i]->action) && (ORTE_GPR_REPLICA_ENTRY_ADDED == action_taken)) ||
                            ((ORTE_GPR_NOTIFY_DEL_ENTRY & trig[i]->action) && (ORTE_GPR_REPLICA_ENTRY_DELETED == action_taken)) ||
                            ((ORTE_GPR_NOTIFY_VALUE_CHG & trig[i]->action) && (ORTE_GPR_REPLICA_ENTRY_CHANGED == action_taken)) ||
                            ((ORTE_GPR_NOTIFY_VALUE_CHG_TO & trig[i]->action) && (ORTE_GPR_REPLICA_ENTRY_CHG_TO == action_taken)) ||
                            ((ORTE_GPR_NOTIFY_VALUE_CHG_FRM & trig[i]->action) && (ORTE_GPR_REPLICA_ENTRY_CHG_FRM == action_taken))) {
                            if (ORTE_SUCCESS != (rc = orte_gpr_replica_register_callback(trig[i]))) {
                                ORTE_ERROR_LOG(rc);
                                return rc;
                            }
                        }
                    }
                
                    /* check if trigger is on this subscription - if so, check it */
                    if (ORTE_GPR_TRIG_ANY & trig[i]->action) {
                        if (ORTE_SUCCESS != (rc = orte_gpr_replica_check_trig(trig[i]))) {
                            ORTE_ERROR_LOG(rc);
                            return rc;
                        }
                    }
                } /* if sptr not NULL */
            }  /* for j */
        }  /* if trig not NULL */
    }
    return ORTE_SUCCESS;
}


int orte_gpr_replica_check_trig(orte_gpr_replica_triggers_t *trig)
{
    orte_gpr_replica_counter_t **cntr;
    bool first, fire;
    int i, rc, level, level2;
    
    if (ORTE_GPR_TRIG_CMP_LEVELS & trig->action) { /* compare the levels of the counters */
        cntr = (orte_gpr_replica_counter_t**)((trig->counters)->addr);
        first = true;
        fire = true;
        for (i=0; i < (trig->counters)->size && fire; i++) {
            if (NULL != cntr[i]) {
                if (first) {
                    if (ORTE_SUCCESS != (rc = orte_gpr_replica_get_value((void*)&level, cntr[i]->iptr))) {
                        ORTE_ERROR_LOG(rc);
                        return rc;
                    }
                    first = false;
                } else {
                   if (ORTE_SUCCESS != (rc = orte_gpr_replica_get_value((void*)&level2, cntr[i]->iptr))) {
                        ORTE_ERROR_LOG(rc);
                        return rc;
                   }
                   if (level2 != level) {
                        fire = false;
                   }
                }
            }
        }
        if (fire) { /* all levels were equal */
            if (ORTE_SUCCESS != (rc = orte_gpr_replica_register_callback(trig))) {
                ORTE_ERROR_LOG(rc);
                return rc;
            }
            goto FIRED;
        }
    }
    return ORTE_SUCCESS;
    
    /* not comparing levels - check instead to see if counters are at a level */
    cntr = (orte_gpr_replica_counter_t**)((trig->counters)->addr);
    fire = true;
    for (i=0; i < (trig->counters)->size && fire; i++) {
        if (NULL != cntr[i]) {
            if (ORTE_SUCCESS != (rc = orte_gpr_replica_get_value(&level, cntr[i]->iptr))) {
                ORTE_ERROR_LOG(rc);
                return rc;
            }
            if (cntr[i]->trigger_level != level) {
                fire = false;
            }
        }
    }
    if (fire) { /* all counters at specified trigger level */
        if (ORTE_SUCCESS != (rc = orte_gpr_replica_register_callback(trig))) {
            ORTE_ERROR_LOG(rc);
            return rc;
        }
        goto FIRED;
    }
    return ORTE_SUCCESS;

FIRED:
    /* if notify_at_start set, unset it to indicate that trigger fired */
    if (ORTE_GPR_TRIG_NOTIFY_START & trig->action) {
        trig->action = trig->action & ~ORTE_GPR_TRIG_NOTIFY_START;
    }

    return ORTE_SUCCESS;
}


int orte_gpr_replica_purge_subscriptions(orte_process_name_t *proc)
{
    orte_gpr_replica_triggers_t **trig;
    int i, rc;
    
    /* locate any notification events that have proc as the recipient
     */
    trig = (orte_gpr_replica_triggers_t**)((orte_gpr_replica.triggers)->addr);
    for (i=0; i < (orte_gpr_replica.triggers)->size; i++) {
        if (NULL != trig[i]) {
            if (NULL == proc && NULL == trig[i]->requestor) {
                if (ORTE_SUCCESS != (rc = orte_pointer_array_set_item(orte_gpr_replica.triggers,
                                                trig[i]->index, NULL))) {
                    ORTE_ERROR_LOG(rc);
                    return rc;
                }
                OBJ_RELEASE(trig);
            } else if (NULL != proc && NULL != trig[i]->requestor &&
                       0 == orte_ns.compare(ORTE_NS_CMP_ALL, proc, trig[i]->requestor)) {
                if (ORTE_SUCCESS != (rc = orte_pointer_array_set_item(orte_gpr_replica.triggers,
                                                trig[i]->index, NULL))) {
                    ORTE_ERROR_LOG(rc);
                    return rc;
                }
                OBJ_RELEASE(trig);
            }
        }
    }
    return ORTE_SUCCESS;
}

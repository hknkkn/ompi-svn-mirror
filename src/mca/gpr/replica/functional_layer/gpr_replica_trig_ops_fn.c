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
    
    /* see if a callback has already been requested for this requestor */
    for (cb = (orte_gpr_replica_callbacks_t*)ompi_list_get_first(&(orte_gpr_replica.callbacks));
         cb != (orte_gpr_replica_callbacks_t*)ompi_list_get_end(&(orte_gpr_replica.callbacks));
         cb = (orte_gpr_replica_callbacks_t*)ompi_list_get_next(cb)) {
         if (trig->requestor == cb->requestor) { /* same destination - add to existing callback */
             if (ORTE_SUCCESS != (rc = orte_gpr_replica_construct_notify_message(cb->message, trig))) {
                ORTE_ERROR_LOG(rc);
                return rc;
             }
         }
    }
    /* got a new callback, generate the request */
    cb = OBJ_NEW(orte_gpr_replica_callbacks_t);
    if (NULL == cb) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
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
    
    /* construct the message */
    cb->message = OBJ_NEW(orte_gpr_notify_message_t);
    if (NULL == cb->message) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }

    if (ORTE_SUCCESS != (rc = orte_gpr_replica_construct_notify_message(&(cb->message), trig))) {
        ORTE_ERROR_LOG(rc);
        OBJ_RELEASE(cb);
        return rc;
    }
    
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
    orte_gpr_notify_data_t **data;
    orte_gpr_replica_subscribed_data_t **sptr;
    int i, k;
    
    /* if we don't have data, just return */
    if (0 >= trig->num_subscribed_data) {
        return ORTE_SUCCESS;
    }
    
    sptr = (orte_gpr_replica_subscribed_data_t**)((trig->subscribed_data)->addr);
    for (i=0; i < (trig->subscribed_data)->size; i++) {
        if (NULL != sptr[i]) {
            if (NULL == (*msg)->data) { /* first data item on the message */
                (*msg)->data = (orte_gpr_notify_data_t**)malloc(sizeof(orte_gpr_notify_data_t*));
                if (NULL == (*msg)->data) {
                    ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                    return ORTE_ERR_OUT_OF_RESOURCE;
                }
                data = &((*msg)->data[0]); /* need to assign location */
                (*msg)->cnt = 1;
            } else {
                /* check to see if this data is going to the same callback as
                 * any prior data on the message. if so, then we add those keyvals
                 * to the existing data structure. if not, then we realloc to
                 * establish a new data structure and store the data there
                 */
                for (k=0; k < (*msg)->cnt; k++) {
                    if ((*msg)->data[k]->cb_num == sptr[i]->index) { /* going to the same place */
                        data = &((*msg)->data[k]);
                        goto MOVEON;
                    }
                }
                /* no prior matching data found, so add another data location to the message */
                (*msg)->data = realloc((*msg)->data, ((*msg)->cnt + 1)*sizeof(orte_gpr_notify_data_t*));
                if (NULL == (*msg)->data) {
                    ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                    return ORTE_ERR_OUT_OF_RESOURCE;
                }
                data = &((*msg)->data[(*msg)->cnt+1]);
                ((*msg)->cnt)++;
            }

            *data = OBJ_NEW(orte_gpr_notify_data_t);
            if (NULL == *data) {
                ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                return ORTE_ERR_OUT_OF_RESOURCE;
            }
            /* for each data object, store the callback_number, addressing mode, and name
             * of the segment this data came from
             */
            (*data)->cb_num = sptr[i]->index;
            (*data)->addr_mode = (sptr[i]->key_addr_mode << 8) || sptr[i]->token_addr_mode;
            (*data)->segment = strdup((sptr[i]->seg)->name);
            if (NULL == (*data)->segment) {
                ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                return ORTE_ERR_OUT_OF_RESOURCE;
            }
MOVEON:
            /* add the values to the data object */
            if (ORTE_SUCCESS != (rc = orte_gpr_replica_add_values(data, sptr[i]))) {
                ORTE_ERROR_LOG(rc);
                return rc;
            }
        }  /* if sptr not NULL */
    }  /* for i */

    return ORTE_SUCCESS;
}


int orte_gpr_replica_add_values(orte_gpr_notify_data_t **data,
                                orte_gpr_replica_subscribed_data_t *sptr)
{
    int i, rc, j, k, n, p, matches, num_tokens, cnt;
    orte_gpr_value_t **value;
    orte_gpr_keyval_t **kptr;
    orte_gpr_replica_itagval_t **iptr;
    orte_gpr_replica_target_t **targets;
    char *token;
    
    /* if we have no data to add, just return */
    if (0 >= sptr->num_targets) {
        return ORTE_SUCCESS;
    }

    targets = (orte_gpr_replica_target_t**)((sptr->targets)->addr);
    for (i=0; i < (sptr->targets)->size; i++) {
        if (NULL != targets[i] && 0 < targets[i]->num_ivals) {  /* don't waste time if no data */
            if (NULL == (*data)->values) { /* first value on the structure */
                (*data)->values = (orte_gpr_value_t**)malloc(sizeof(orte_gpr_value_t*));
                if (NULL == (*data)->values) {
                    ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                    return ORTE_ERR_OUT_OF_RESOURCE;
                }
                value = &((*data)->values[0]); /* need to assign location */
                (*data)->cnt = 1;
            } else {
                /* check to see if these keyvals are from the same container
                 * as some prior one. if so, then we add those keyvals
                 * to the existing value structure. if not, then we realloc to
                 * establish a new value structure and store the data there
                 */
                for (k=0; k < (*data)->cnt; k++) {
                    matches = 0;
                    num_tokens = ((*data)->values[k])->num_tokens;
                    if (num_tokens == (targets[i]->cptr)->num_itags) { /* must have same number or can't match */
                        for (j=0; j < ((*data)->values[k])->num_tokens; j++) {
                            if (ORTE_SUCCESS != (rc = orte_gpr_replica_dict_reverse_lookup(&token,
                                                    sptr->seg, (targets[i]->cptr)->itags[j]))) {
                                ORTE_ERROR_LOG(rc);
                                return rc;
                            }
                            if (0 == strcmp(((*data)->values[k])->tokens[j], token)) {
                                matches++;
                            }
                            if (num_tokens == matches) { /* from same container - just add keyvals to it */
                                value = &((*data)->values[k]);
                                goto MOVEON;
                            }
                            free(token);
                        }
                    }
                }
                /* no prior matching data found, so add another value location to the object */
                (*data)->values = (orte_gpr_value_t**)realloc((*data)->values, ((*data)->cnt + 1)*sizeof(orte_gpr_value_t*));
                if (NULL == (*data)->values) {
                    ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                    return ORTE_ERR_OUT_OF_RESOURCE;
                }
                value = &((*data)->values[(*data)->cnt+1]);
                ((*data)->cnt)++;
            }

            *value = OBJ_NEW(orte_gpr_value_t);
            if (NULL == *value) {
                ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                return ORTE_ERR_OUT_OF_RESOURCE;
            }

            /* record the addressing mode */
            (*value)->addr_mode = ((orte_gpr_addr_mode_t)sptr->key_addr_mode << 8) | (orte_gpr_addr_mode_t)sptr->token_addr_mode;
            /* record the segment these values came from */
            (*value)->segment = strdup((sptr->seg)->name);
            if (NULL == ((*value)->segment)) {
                ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                return ORTE_ERR_OUT_OF_RESOURCE;
            }
            /* record the tokens describing the container */
            (*value)->num_tokens = (targets[i]->cptr)->num_itags;
            (*value)->tokens = (char **)malloc((targets[i]->cptr)->num_itags * sizeof(char*));
            if (NULL == (*value)->tokens) {
                ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                return ORTE_ERR_OUT_OF_RESOURCE;
            }
            for (n=0; n < (targets[i]->cptr)->num_itags; n++) {
                if (ORTE_SUCCESS != (rc = orte_gpr_replica_dict_reverse_lookup(
                                            &((*value)->tokens[n]), sptr->seg,
                                            (targets[i]->cptr)->itags[n]))) {
                    ORTE_ERROR_LOG(rc);
                    return rc;
                }
            }
MOVEON:
            /* record the values to be returned */
            cnt = targets[i]->num_ivals;
            if (0 < (*value)->cnt) {  /* already have some data here, so add to the space */
                n = (*value)->cnt + cnt;
                (*value)->keyvals = (orte_gpr_keyval_t**)realloc((*value)->keyvals, n * sizeof(orte_gpr_keyval_t*));
                if (NULL == (*value)->keyvals) {
                    ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                    return ORTE_ERR_OUT_OF_RESOURCE;
                }
                (*value)->cnt = n;
            } else {
                (*value)->keyvals = (orte_gpr_keyval_t**)malloc(cnt * sizeof(orte_gpr_keyval_t*));
                if (NULL == (*value)->keyvals) {
                    ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                    return ORTE_ERR_OUT_OF_RESOURCE;
                }
                (*value)->cnt = cnt;
            }
            kptr = (*value)->keyvals;
            iptr = (orte_gpr_replica_itagval_t**)((targets[i]->ivals)->addr);
            for (n=0, p=0; n < (targets[i]->ivals)->size; n++) {
                if (NULL != iptr[n]) {
                    kptr[p] = OBJ_NEW(orte_gpr_keyval_t);
                    if (NULL == kptr[p]) {
                        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                        return ORTE_ERR_OUT_OF_RESOURCE;
                    }
                    if (ORTE_SUCCESS != (rc = orte_gpr_replica_dict_reverse_lookup(
                                            &(kptr[p]->key), sptr->seg, iptr[n]->itag))) {
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
        }  /* if targets not NULL */
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

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
#include "mca/ns/ns.h"
#include "mca/errmgr/errmgr.h"

#include "mca/gpr/replica/transition_layer/gpr_replica_tl.h"
#include "gpr_replica_fn.h"

int orte_gpr_replica_subscribe_fn(orte_gpr_notify_action_t action,
                                  orte_gpr_replica_segment_t *seg,
                                  orte_gpr_value_t *value,
                                  orte_gpr_value_t *trigval,
                                  orte_gpr_notify_id_t local_idtag)
{
    orte_gpr_replica_triggers_t *trig=NULL;
    orte_gpr_replica_container_t **cptr=NULL, *cptr2=NULL;
    orte_gpr_replica_itag_t itag, *tokentags=NULL;
    orte_gpr_replica_itagval_t *iptr=NULL;
    orte_gpr_replica_addr_mode_t tok_mode, key_mode;
    int i, j, rc, num_tokens, num_found;
    bool found;

    if (orte_gpr_replica_globals.debug) {
	   ompi_output(0, "[%d,%d,%d] gpr replica: subscribe entered: segment %s",
		    ORTE_NAME_ARGS(orte_process_info.my_name), seg->name);
    }

    trig = (orte_gpr_replica_triggers_t*)((orte_gpr_replica.triggers)->addr[local_idtag]);
    if (NULL == trig) {
        ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
        return ORTE_ERR_BAD_PARAM;
    }
    
    trig->seg = seg;
    trig->action = action;
    
    trig->token_addr_mode = 0x004f & value->addr_mode;
    if (0x00 == trig->token_addr_mode) {  /* default token address mode to AND */
        trig->token_addr_mode = ORTE_GPR_REPLICA_AND;
    }
    trig->key_addr_mode = ((0x4f00 & value->addr_mode) >> 8) & 0x004f;
    if (0x00 == trig->key_addr_mode) {  /* default key address mode to OR */
        trig->key_addr_mode = ORTE_GPR_REPLICA_OR;
    }

    if (NULL != value->tokens && 0 < value->num_tokens) {
        num_tokens = value->num_tokens; /* indicates non-NULL terminated list */
        if (ORTE_SUCCESS != (rc = orte_gpr_replica_get_itag_list(&tokentags, seg,
                                value->tokens, &num_tokens))) {
            ORTE_ERROR_LOG(rc);
            return rc;
        
        }
        if (ORTE_SUCCESS != (rc = orte_value_array_set_size(&(trig->tokentags), (size_t)num_tokens))) {
            ORTE_ERROR_LOG(rc);
            return rc;
        }
        for (i=0; i < num_tokens; i++) {
            ORTE_VALUE_ARRAY_SET_ITEM(&(trig->tokentags), orte_gpr_replica_itag_t,
                                            i, tokentags[i]);
        }
        free(tokentags);
        tokentags = NULL;
    }
    
    if (NULL != value->keyvals && 0 < value->cnt) {
        if (ORTE_SUCCESS != (rc = orte_value_array_set_size(&(trig->keytags), (size_t)(value->cnt)))) {
            ORTE_ERROR_LOG(rc);
            return rc;
        }
        for (i=0; i < value->cnt; i++) {
            if (ORTE_SUCCESS != (rc = orte_gpr_replica_create_itag(&itag,
                                                    seg, (value->keyvals[i])->key))) {
                ORTE_ERROR_LOG(rc);
                goto CLEANUP;
            }
            ORTE_VALUE_ARRAY_SET_ITEM(&(trig->keytags), orte_gpr_replica_itag_t,
                                            i, itag);
        }
    }
    
    /* if this has a trigger in it, need to setup the counters */
    if (ORTE_GPR_TRIG_ANY & action) {
        /* get the trigger's addressing modes */
        tok_mode = 0x004f & trigval->addr_mode;
        if (0x00 == tok_mode) {  /* default token address mode to AND */
            tok_mode = ORTE_GPR_REPLICA_AND;
        }
        key_mode = ((0x4f00 & trigval->addr_mode) >> 8) & 0x004f;
        if (0x00 == key_mode) {  /* default key address mode to OR */
            key_mode = ORTE_GPR_REPLICA_OR;
        }
        
        /* convert the trigger's tokens to an itaglist */
        if (NULL != trigval->tokens && 0 < trigval->num_tokens) {
            num_tokens = trigval->num_tokens; /* indicates non-NULL terminated list */
            if (ORTE_SUCCESS != (rc = orte_gpr_replica_get_itag_list(&tokentags, seg,
                                trigval->tokens, &num_tokens))) {
                ORTE_ERROR_LOG(rc);
                goto CLEANUP;
            }
        }
        
        /* find the specified container(s) */
        if (ORTE_SUCCESS != (rc = orte_gpr_replica_find_containers(&num_found, seg, tok_mode,
                                        tokentags, num_tokens))) {
            ORTE_ERROR_LOG(rc);
            goto CLEANUP;
        }
        
        if (0 == num_found) {  /* existing container not found - create one using all the tokens */
            if (ORTE_SUCCESS != (rc = orte_gpr_replica_create_container(&cptr2, seg,
                                                num_tokens, tokentags))) {
                ORTE_ERROR_LOG(rc);
                goto CLEANUP;
            }
     
            /* ok, store all the counters in the new container, adding a pointer to each
             * one in the trigger's counter array
             */
            for (i=0; i < trigval->cnt; i++) {
                if (ORTE_SUCCESS != (rc = orte_gpr_replica_add_keyval(&iptr, seg, cptr2, trigval->keyvals[i]))) {
                    ORTE_ERROR_LOG(rc);
                    goto CLEANUP;
                }
                if (0 > orte_pointer_array_add(trig->counters, (void*)iptr)) {
                    ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                    rc = ORTE_ERR_OUT_OF_RESOURCE;
                    goto CLEANUP;
                }
            }
        } else {  /* For each counter, go through the list of containers and
                     see if it already exists in container. Only allow each
                     counter to be identified once - error if either a counter is never
                     found or already existing in more than one place. */
            cptr = (orte_gpr_replica_container_t**)(orte_gpr_replica_globals.srch_cptr)->addr;
            for (i=0; i < trigval->cnt; i++) {
                found = false;
                for (j=0; j < (orte_gpr_replica_globals.srch_cptr)->size; j++) {
                    if (NULL != cptr[j]) {
                        if (ORTE_SUCCESS == orte_gpr_replica_dict_lookup(&itag, seg, trigval->keyvals[i]->key) &&
                            ORTE_SUCCESS == orte_gpr_replica_search_container(&num_found,
                                                    ORTE_GPR_REPLICA_OR,
                                                    &itag, 1, cptr[j]) &&
                            0 < num_found) {
                            /* this key already exists - make sure it's unique
                             */
                            if (1 < num_found || found) { /* not unique - error out */
                                ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
                                rc = ORTE_ERR_BAD_PARAM;
                                goto CLEANUP;
                            }
                            /* okay, add to trigger's counter array */
                            found = true;
                            iptr = (orte_gpr_replica_itagval_t*)((orte_gpr_replica_globals.srch_ival)->addr[0]);
                            if (0 > orte_pointer_array_add(trig->counters, (void*)iptr)) {
                                ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                                rc = ORTE_ERR_OUT_OF_RESOURCE;
                                goto CLEANUP;
                            }
                            (trig->num_counters)++;
                        }  /* end if found */
                    }  /* end if cptr NULL */
                }  /* end for j */
                if (!found) {  /* specified counter never found - error */
                    ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
                    rc = ORTE_ERR_BAD_PARAM;
                    goto CLEANUP;
                } /* end if found */
            }  /* end for i */
        }  /* end if/else container found */
    }  /* end if trigger */
    
    /* need to check the existing data to flag those that fit the new subscription */
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_init_trigger(seg, trig))) {
        ORTE_ERROR_LOG(rc);
    }

CLEANUP:
    if (NULL != tokentags) {
        free(tokentags);
    }
    
    if (ORTE_SUCCESS != rc) OBJ_RELEASE(trig);
    
    return rc;
}


int orte_gpr_replica_unsubscribe_fn(orte_gpr_notify_id_t sub_number)
{
    orte_gpr_replica_triggers_t *trig;

    if (orte_gpr_replica_globals.debug) {
	   ompi_output(0, "[%d,%d,%d] gpr replica: unsubscribe entered for sub number %d",
		    ORTE_NAME_ARGS(orte_process_info.my_name), sub_number);
    }

    /* release trigger on replica and remove it */
    trig = (orte_gpr_replica_triggers_t*)((orte_gpr_replica.triggers)->addr[sub_number]);
    if (NULL == trig) {
        return ORTE_ERR_BAD_PARAM;
    }
    OBJ_RELEASE(trig);
    
    orte_pointer_array_set_item(orte_gpr_replica.triggers, sub_number, NULL);
    return ORTE_SUCCESS;
}

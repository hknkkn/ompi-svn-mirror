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

int orte_gpr_replica_subscribe_fn(orte_gpr_addr_mode_t addr_mode,
                            orte_gpr_replica_segment_t *seg,
                            orte_gpr_value_t *value,
                            int trigger_level,
                            orte_gpr_notify_id_t local_idtag)
{
    orte_gpr_replica_triggers_t *trig;
    orte_gpr_replica_itag_t *itags;
    orte_gpr_keyval_t *kptr;
    orte_gpr_replica_itagval_t *iptr;
    int i, rc, num_tokens;

    if (orte_gpr_replica_globals.debug) {
	   ompi_output(0, "[%d,%d,%d] gpr replica: subscribe entered: segment %s",
		    ORTE_NAME_ARGS(orte_process_info.my_name), seg->name);
    }

    trig = (orte_gpr_replica_triggers_t*)((orte_gpr_replica.triggers)->addr[local_idtag]);
    if (NULL == trig) {
        ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
        return ORTE_ERR_BAD_PARAM;
    }
    
    trig->token_addr_mode = 0x004f & addr_mode;
    if (0x00 == trig->token_addr_mode) {  /* default token address mode to AND */
        trig->token_addr_mode = ORTE_GPR_REPLICA_AND;
    }
    trig->key_addr_mode = ((0x4f00 & addr_mode) >> 8) & 0x004f;
    if (0x00 == trig->key_addr_mode) {  /* default key address mode to OR */
        trig->key_addr_mode = ORTE_GPR_REPLICA_OR;
    }

    trig->trigger = trigger_level;
    
    if (NULL != value->tokens && 0 < value->num_tokens) {
        num_tokens = value->num_tokens; /* indicates non-NULL terminated list */
        if (ORTE_SUCCESS != (rc = orte_gpr_replica_get_itag_list(&itags, seg,
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
                                            i, itags[i]);
        }
        free(itags);
        itags = NULL;
    }
    
    if (NULL != value->keyvals && 0 < value->cnt) {
        trig->num_keys = value->cnt;
        for (i=0; i < value->cnt; i++) {
            kptr = value->keyvals[i];
            iptr = OBJ_NEW(orte_gpr_replica_itagval_t);
            if (NULL == iptr) {
                ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                rc = ORTE_ERR_OUT_OF_RESOURCE;
                goto CLEANUP;
            }
            
            if (ORTE_SUCCESS != (rc = orte_gpr_replica_create_itag(&(iptr->itag),
                                                    seg, kptr->key))) {
                ORTE_ERROR_LOG(rc);
                goto CLEANUP;
            }
            
            iptr->type = kptr->type;
            if (ORTE_SUCCESS != (rc = orte_gpr_replica_xfer_payload(&(iptr->value),
                                                       &(kptr->value), kptr->type))) {
                ORTE_ERROR_LOG(rc);
                goto CLEANUP;
            }
            
            if (0 > (iptr->index = orte_pointer_array_add(trig->itagvals, (void*)iptr))) {
                ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                rc = ORTE_ERR_OUT_OF_RESOURCE;
                goto CLEANUP;
            }
        }
    }
    
    /* need to check the existing data to flag those that fit the new subscription */
    return orte_gpr_replica_init_trigger(seg, trig);

CLEANUP:
    OBJ_RELEASE(trig);
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

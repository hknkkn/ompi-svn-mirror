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

#include "gpr_replica_fn.h"



int orte_gpr_replica_put_fn(orte_gpr_addr_mode_t addr_mode,
                            orte_gpr_replica_segment_t *seg,
                            orte_gpr_replica_itag_t *token_itags, int num_tokens,
                            size_t cnt, orte_gpr_keyval_t *keyvals,
                            int8_t *action_taken)
{
#if 0
    orte_gpr_replica_core_t *entry_ptr;
    bool overwrite;
    orte_gpr_replica_trigger_list_t *trig;
    int return_code;
    bool found;


    if (orte_gpr_replica_globals.debug) {
	    ompi_output(0, "[%d,%d,%d] gpr replica: put entered on segment %s",
		    ORTE_NAME_ARGS(*(orte_process_info.my_name)), seg->name);
    }

    /* initialize action */
    *action_taken = 0;
    
    /* ignore addressing mode - all tokens are used
     * only overwrite permission mode flag has any affect
     */
    overwrite = false;
    if (addr_mode & ORTE_GPR_OVERWRITE) {
        overwrite = true;
    }

    /* find the specified container */
    found = false;
    cptr = (orte_gpr_replica_container_t**)((seg->containers)->addr);
    for (i=0; i < (seg->containers)->size && !found; i++) {
        if (NULL != cptr && orte_gpr_replica_check_itag_list(ORTE_GPR_XAND,
                                             num_tokens, token_itags,
                                             (*cptr)->num_itags, (*cptr)->itags)) {
            found = true;
            break;
        }
        cptr++;
    }
    
    if (!found) {  /* existing container not found - create one */
        cptr = OBJ_NEW(orte_gpr_replica_container_t);
        if (NULL == cptr) {
            return ORTE_ERR_OUT_OF_RESOURCE;
        }
        if (ORTE_SUCCESS !=
                (rc = orte_gpr_replica_copy_itag_list(&(cpr->itags),
                                                      token_itags, num_tokens))) {
            return rc;
        }
        if (0 < (cptr->index = orte_pointer_array_add(seg->containers, (void*)cptr))) {
            return ORTE_ERR_OUT_OF_RESOURCE;
        }
        /* ok, store all the keyvals in the container */
        kptr = keyvals;
        for (i=0; i < cnt; i++) {
            if (ORTE_SUCCESS != (rc = orte_gpr_replica_store_keyval(seg, cptr, &kptr))) {
                return rc;
            }
            kptr++;
        }
        *action_taken = ORTE_GPR_ENTRY_ADDED;
    } else {  /* otherwise, see if entry already exists in container */
        kptr = keyvals;
        for (i=0; i < cnt; i++) {
            if (orte_gpr_replica_search_container(&iptr, cptr, kptr)) {
                /* this key already exists - overwrite, if permission given
                 * else error
                 */
                 if (overwrite) {
                    if (ORTE_SUCCESS != (rc = orte_gpr_replica_xfer_payload(iptr, kptr))) {
                        return rc;
                    }
                 } else {
                    return ORTE_ERROR;
                 }
                 *action_taken = *action_taken | ORTE_GPR_ENTRY_UPDATED;
            } else { /* new key - add to container */
                if (ORTE_SUCCESS != (rc = orte_gpr_replica_store_keyval(seg, cptr, &kptr))) {
                    return rc;
                }
                *action_taken = *action_taken | ORTE_GPR_ENTRY_ADDED;
            }
            kptr++;
        }
    }

    /* update trigger list */
    for (trig = (orte_gpr_replica_trigger_list_t*)ompi_list_get_first(&seg->triggers);
	 trig != (orte_gpr_replica_trigger_list_t*)ompi_list_get_end(&seg->triggers);
         trig = (orte_gpr_replica_trigger_list_t*)ompi_list_get_next(trig)) {
	if (orte_gpr_replica_check_key_list(trig->addr_mode, trig->num_keys, trig->keys,
				       num_keys, keys)) {
	    trig->count++;
	}
    }

 CLEANUP:
    if (orte_gpr_replica_globals.debug) {
	ompi_output(0, "[%d,%d,%d] gpr replica-put: complete", ORTE_NAME_ARGS(*(orte_process_info.my_name)));
    }

    return return_code;
#endif
    return ORTE_ERR_NOT_IMPLEMENTED;
}


int orte_gpr_replica_put_nb_fn(orte_gpr_addr_mode_t addr_mode,
                orte_gpr_replica_segment_t *seg,
                orte_gpr_replica_itag_t *token_itags, int num_tokens,
                size_t cnt, orte_gpr_keyval_t *keyvals,
                orte_gpr_notify_cb_fn_t cbfunc, void *user_tag)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}
                      

int orte_gpr_replica_get_fn(orte_gpr_addr_mode_t addr_mode,
                            orte_gpr_replica_segment_t *seg,
                            orte_gpr_replica_itag_t *tokentags, int num_tokens,
                            orte_gpr_replica_itag_t *keytags, int num_keys,
                            size_t *cnt, orte_gpr_keyval_t **keyvals)
{
#if 0
    orte_gpr_value_t *ans=NULL;
    orte_gpr_replica_core_t *reg=NULL;

    if (orte_gpr_replica_globals.debug) {
	ompi_output(0, "[%d,%d,%d] gpr replica: get entered", ORTE_NAME_ARGS(*(orte_process_info.my_name)));
    }

    /* traverse the segment's registry, looking for matching tokens per the specified mode */
    for (reg = (orte_gpr_replica_core_t*)ompi_list_get_first(&seg->registry_entries);
	 reg != (orte_gpr_replica_core_t*)ompi_list_get_end(&seg->registry_entries);
	 reg = (orte_gpr_replica_core_t*)ompi_list_get_next(reg)) {

	/* for each registry entry, check the key list */
	if (orte_gpr_replica_check_key_list(addr_mode, num_keys, keys,
				       reg->num_keys, reg->keys)) { /* found the key(s) on the list */
	    ans = OBJ_NEW(orte_gpr_value_t);
	    ans->object_size = reg->object_size;
	    ans->object = (orte_gpr_object_t*)malloc(ans->object_size);
	    memcpy(ans->object, reg->object, ans->object_size);
	    ompi_list_append(answer, &ans->item);
	}
    }
    if (orte_gpr_replica_globals.debug) {
	ompi_output(0, "[%d,%d,%d] gpr replica-get: finished search", ORTE_NAME_ARGS(*(orte_process_info.my_name)));
    }

    return;
#endif
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_gpr_replica_get_nb_fn(orte_gpr_addr_mode_t addr_mode,
                                orte_gpr_replica_segment_t *seg,
                                orte_gpr_replica_itag_t *tokentags, int num_tokens,
                                orte_gpr_replica_itag_t *keytags, int num_keys,
                                orte_gpr_notify_cb_fn_t cbfunc, void *user_tag)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}


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

#include "mca/gpr/replica/transition_layer/gpr_replica_tl.h"

#include "gpr_replica_fn.h"



int orte_gpr_replica_put_fn(orte_gpr_addr_mode_t addr_mode,
                            orte_gpr_replica_segment_t *seg,
                            orte_gpr_replica_itag_t *token_itags, int num_tokens,
                            int cnt, orte_gpr_keyval_t **keyvals,
                            int8_t *action_taken)
{
    orte_gpr_replica_itagval_t *iptr;
    orte_gpr_replica_container_t *cptr;
    orte_gpr_replica_itag_t itag;
    orte_gpr_keyval_t **kptr;
    bool overwrite;
    int rc, i;
    bool found;


    if (orte_gpr_replica_globals.debug) {
	    ompi_output(0, "[%d,%d,%d] gpr replica: put entered on segment %s",
		    ORTE_NAME_ARGS(*(orte_process_info.my_name)), seg->name);
    }

    /* initialize action */
    *action_taken = 0;
    
    /* all tokens are used
     * only overwrite permission mode flag has any affect
     */
    overwrite = false;
    if (addr_mode & ORTE_GPR_OVERWRITE) {
        overwrite = true;
    }

    /* find the specified container */
    found = false;
    cptr = (orte_gpr_replica_container_t*)((seg->containers)->addr);
    for (i=0; i < (seg->containers)->size && !found; i++) {
        if (NULL != cptr && orte_gpr_replica_check_itag_list(ORTE_GPR_XAND,
                                             num_tokens, token_itags,
                                             cptr->num_itags, cptr->itags)) {
            found = true;
            break;
        }
        cptr++;
    }
    
    if (!found) {  /* existing container not found - create one */
        if (ORTE_SUCCESS != (rc = orte_gpr_replica_create_container(&cptr, seg,
                                            num_tokens, token_itags))) {
            return rc;
        }
 
        /* ok, store all the keyvals in the container */
        kptr = keyvals;
        for (i=0; i < cnt; i++) {
            if (ORTE_SUCCESS != (rc = orte_gpr_replica_add_keyval(seg, cptr, &(kptr[i])))) {
                return rc;
            }
        }
        *action_taken = ORTE_GPR_REPLICA_ENTRY_ADDED;
    } else {  /* otherwise, see if entry already exists in container */
        kptr = keyvals;
        for (i=0; i < cnt; i++) {
            if (ORTE_SUCCESS == orte_gpr_replica_dict_lookup(&itag, seg, kptr[i]->key) &&
                orte_gpr_replica_search_container(&iptr, itag, cptr)) {
                /* this key already exists - overwrite, if permission given
                 * else error
                 */
                 if (overwrite) {
                    if (ORTE_SUCCESS != (rc = orte_gpr_replica_update_keyval(seg, iptr, (&kptr[i])))) {
                        return rc;
                    }
                 } else {
                    return ORTE_ERROR;
                 }
                 *action_taken = *action_taken | ORTE_GPR_REPLICA_ENTRY_UPDATED;
            } else { /* new key - add to container */
                if (ORTE_SUCCESS != (rc = orte_gpr_replica_add_keyval(seg, cptr, &(kptr[i])))) {
                    return rc;
                }
                *action_taken = *action_taken | ORTE_GPR_REPLICA_ENTRY_ADDED;
            }
        }
    }

    if (orte_gpr_replica_globals.debug) {
	    ompi_output(0, "[%d,%d,%d] gpr replica-put: complete", ORTE_NAME_ARGS(*(orte_process_info.my_name)));
    }

    return ORTE_SUCCESS;
}


int orte_gpr_replica_put_nb_fn(orte_gpr_addr_mode_t addr_mode,
                orte_gpr_replica_segment_t *seg,
                orte_gpr_replica_itag_t *token_itags, int num_tokens,
                int cnt, orte_gpr_keyval_t **keyvals,
                orte_gpr_notify_cb_fn_t cbfunc, void *user_tag)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}
                      

int orte_gpr_replica_get_fn(orte_gpr_addr_mode_t addr_mode,
                            orte_gpr_replica_segment_t *seg,
                            orte_gpr_replica_itag_t *tokentags, int num_tokens,
                            orte_gpr_replica_itag_t *keytags, int num_keys,
                            int *cnt, orte_gpr_value_t **values)
{
#if 0
    orte_gpr_value_t *ans=NULL;
    ompi_list_t *list;
    orte_gpr_replica_val_list_t *valptr;

    if (orte_gpr_replica_globals.debug) {
	ompi_output(0, "[%d,%d,%d] gpr replica: get entered", ORTE_NAME_ARGS(*(orte_process_info.my_name)));
    }

    /* initialize the list of findings */
    list = OBJ_NEW(ompi_list_t);
    
    /* find all containers that meet search criteria */
    none_found = true;
    cptr = (orte_gpr_replica_container_t*)((seg->containers)->addr);
    for (i=0; i < (seg->containers)->size && !found; i++) {
        if (NULL != cptr && orte_gpr_replica_check_itag_list(ORTE_GPR_XAND,
                                             num_tokens, token_itags,
                                             cptr->num_itags, cptr->itags)) {
            /* search container for matching keys - allow wildcards */
            iptr = (orte_gpr_replica_itagval_t**)((cptr->itagvals)->addr);
            for (i=0; i < (cptr->itagvals)->size; i++) {
                if (NULL != iptr[i] && orte_gpr_replica_check_itag(iptr[i]->itag,
                                             num_keys, keytags)) {
                    /* match found - copy result into list */
                    valptr = OBJ_NEW(orte_gpr_replica_val_lisst_t);
                    if (NULL == valptr) {
                        return ORTE_ERR_OUT_OF_RESOURCE;
                    }
                    (valptr->itagval).itag = iptr[i]->itag;
                    (valptr->itagval).type = iptr[i]->type;
                    if (ORTE_SUCCESS != orte_gpr_replica_xfer_payload(
                                                &(valptr->itagval).value,
                                                &(iptr[i]->value),
                                                iptr[i]->type)) {
                        return ORTE_ERROR;
                    }
                    ompi_list_append(list, &valptr->item);
                    none_found = false;
                }
            }
        }
        cptr++;
    }
    
    if (none_found) {  /* nothing found - report that */
        return ORTE_ERR_NOT_FOUND;
    }
    
    return ORTE_SUCCESS;
    
    /* traverse the segment's registry, looking for matching tokens per the specified mode */
    for (reg = (orte_gpr_replica_core_t*)ompi_list_get_first(&seg->registry_entries);
	 reg != (orte_gpr_replica_core_t*)ompi_list_get_end(&seg->registry_entries);
	 reg = (orte_gpr_replica_core_t*)ompi_list_get_next(reg)) {

	/* for each registry entry, check the key list */
	if (orte_gpr_replica_check_key_list(addr_mode, num_keys, keys,
				       reg->num_keys, reg->keys)) { /* found the key(s) on the list */
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


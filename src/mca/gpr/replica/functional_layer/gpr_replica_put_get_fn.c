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

#include "gpr_replica_fn.h"



int orte_gpr_replica_put_fn(orte_gpr_addr_mode_t addr_mode,
			   orte_gpr_replica_segment_t *seg,
                orte_gpr_replica_itag_t *token_itags, int num_tokens,
                orte_gpr_replica_itag_t *key_tags, int num_keys,
                size_t cnt, orte_gpr_replica_itagval_t **itagvals,
			   int8_t *action_taken)
{
#if 0
    orte_gpr_replica_core_t *entry_ptr;
    ompi_registry_mode_t put_mode;
    orte_gpr_replica_trigger_list_t *trig;
    int return_code;


    if (orte_gpr_replica_debug) {
	ompi_output(0, "[%d,%d,%d] gpr replica: put entered on segment %s",
		    OMPI_NAME_ARGS(*ompi_rte_get_self()), seg->name);
    }

    /* ignore addressing mode - all tokens are used
     * only overwrite permission mode flag has any affect
     */
    put_mode = addr_mode & OMPI_REGISTRY_OVERWRITE;

    /* see if specified entry already exists */
    for (entry_ptr = (orte_gpr_replica_core_t*)ompi_list_get_first(&seg->registry_entries);
	 entry_ptr != (orte_gpr_replica_core_t*)ompi_list_get_end(&seg->registry_entries);
	 entry_ptr = (orte_gpr_replica_core_t*)ompi_list_get_next(entry_ptr)) {
	if (orte_gpr_replica_check_key_list(put_mode, num_keys, keys,
				       entry_ptr->num_keys, entry_ptr->keys)) {
	    /* found existing entry - overwrite if mode set, else error */
	    if (put_mode) {  /* overwrite enabled */
		free(entry_ptr->object);
		entry_ptr->object = NULL;
		entry_ptr->object_size = size;
		entry_ptr->object = (ompi_registry_object_t)malloc(size);
		memcpy(entry_ptr->object, object, size);
		return_code = OMPI_SUCCESS;
		*action_taken = MCA_GPR_REPLICA_OBJECT_UPDATED;
		goto CLEANUP;
	    } else {
		return_code = OMPI_ERROR;
		goto CLEANUP;
	    }
	}
    }

    /* no existing entry - create new one */
    entry_ptr = OBJ_NEW(orte_gpr_replica_core_t);
    entry_ptr->keys = (orte_gpr_replica_key_t*)malloc(num_keys*sizeof(orte_gpr_replica_key_t));
    memcpy(entry_ptr->keys, keys, num_keys*sizeof(orte_gpr_replica_key_t));
    entry_ptr->num_keys = num_keys;
    entry_ptr->object_size = size;
    entry_ptr->object = (ompi_registry_object_t*)malloc(size);
    memcpy(entry_ptr->object, object, size);
    ompi_list_append(&seg->registry_entries, &entry_ptr->item);

    *action_taken = MCA_GPR_REPLICA_OBJECT_ADDED;
    return_code = OMPI_SUCCESS;

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
    if (orte_gpr_replica_debug) {
	ompi_output(0, "[%d,%d,%d] gpr replica-put: complete", OMPI_NAME_ARGS(*ompi_rte_get_self()));
    }

    return return_code;
#endif
    return ORTE_ERR_NOT_IMPLEMENTED;
}


int orte_gpr_replica_put_nb_fn(orte_gpr_addr_mode_t addr_mode,
                orte_gpr_replica_segment_t *seg,
                orte_gpr_replica_itag_t *token_itags, int num_tokens,
                orte_gpr_replica_itag_t *key_tags, int num_keys,
                size_t cnt, orte_gpr_replica_itagval_t **itagvals,
                orte_gpr_notify_cb_fn_t cbfunc, void *user_tag)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}
                      

int orte_gpr_replica_get_fn(orte_gpr_addr_mode_t addr_mode,
                            char *segment, char **tokens, char **itags,
                            orte_gpr_replica_itag_t *key_tags, int num_keys,
                            size_t *cnt, orte_gpr_replica_itagval_t **itagvals)
{
#if 0
    ompi_registry_value_t *ans=NULL;
    orte_gpr_replica_core_t *reg=NULL;

    if (orte_gpr_replica_debug) {
	ompi_output(0, "[%d,%d,%d] gpr replica: get entered", OMPI_NAME_ARGS(*ompi_rte_get_self()));
    }

    /* traverse the segment's registry, looking for matching tokens per the specified mode */
    for (reg = (orte_gpr_replica_core_t*)ompi_list_get_first(&seg->registry_entries);
	 reg != (orte_gpr_replica_core_t*)ompi_list_get_end(&seg->registry_entries);
	 reg = (orte_gpr_replica_core_t*)ompi_list_get_next(reg)) {

	/* for each registry entry, check the key list */
	if (orte_gpr_replica_check_key_list(addr_mode, num_keys, keys,
				       reg->num_keys, reg->keys)) { /* found the key(s) on the list */
	    ans = OBJ_NEW(ompi_registry_value_t);
	    ans->object_size = reg->object_size;
	    ans->object = (ompi_registry_object_t*)malloc(ans->object_size);
	    memcpy(ans->object, reg->object, ans->object_size);
	    ompi_list_append(answer, &ans->item);
	}
    }
    if (orte_gpr_replica_debug) {
	ompi_output(0, "[%d,%d,%d] gpr replica-get: finished search", OMPI_NAME_ARGS(*ompi_rte_get_self()));
    }

    return;
#endif
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_gpr_replica_get_nb_fn(orte_gpr_addr_mode_t addr_mode,
                                char *segment, char **tokens, char **itags,
                                orte_gpr_replica_itag_t *key_tags, int num_keys,
                                orte_gpr_notify_cb_fn_t cbfunc, void *user_tag)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}


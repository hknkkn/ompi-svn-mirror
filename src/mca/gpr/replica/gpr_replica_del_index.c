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

#include "include/orte_constants.h"

#include "util/output.h"
#include "util/proc_info.h"

#include "gpr_replica.h"
#include "gpr_replica_internals.h"


int orte_gpr_replica_delete_segment(char *segment)
{
    orte_gpr_replica_segment_t *seg;
    int rc;

    /* protect against errors */
    if (NULL == segment) {
	   return OMPI_ERROR;
    }

    if (orte_gpr_replica_compound_cmd_mode) {
	    return orte_gpr_base_pack_delete_segment(orte_gpr_replica_compound_cmd, segment);
    }

    OMPI_THREAD_LOCK(&orte_gpr_replica_mutex);

    /* locate the segment */
    seg = orte_gpr_replica_find_seg(false, segment);
    if (NULL == seg) {
        OMPI_THREAD_UNLOCK(&orte_gpr_replica_mutex);
	    return ORTE_ERROR;
    }

    rc = orte_gpr_replica_release_segment(seg);

    OMPI_THREAD_UNLOCK(&orte_gpr_replica_mutex);
    return rc;
}


int orte_gpr_replica_delete_entries(ompi_registry_mode_t addr_mode,
			      char *segment, char **tokens)
{
    int rc;
    orte_gpr_replica_segment_t *seg;
    orte_gpr_replica_key_t *keys;
    int num_keys;
    orte_jobid_t jobid;

    /* protect against errors */
    if (NULL == segment) {
	   return OMPI_ERROR;
    }

    if (orte_gpr_replica_compound_cmd_mode) {
	return orte_gpr_base_pack_delete_object(orte_gpr_replica_compound_cmd,
					       orte_gpr_replica_silent_mode,
					       addr_mode, segment, tokens);
    }

    OMPI_THREAD_LOCK(&orte_gpr_replica_mutex);

    seg = orte_gpr_replica_find_seg(false, segment);
    if (NULL == seg) {
        OMPI_THREAD_UNLOCK(&orte_gpr_replica_mutex);
	   return OMPI_ERROR;
    }

    keys = orte_gpr_replica_get_key_list(seg, tokens, &num_keys);

    rc = orte_gpr_replica_delete_object_nl(addr_mode, seg, keys, num_keys);

    orte_gpr_replica_check_subscriptions(seg, MCA_GPR_REPLICA_OBJECT_DELETED);

    orte_gpr_replica_check_synchros(seg);

    if (NULL != keys) {
	   free(keys);
    }

    OMPI_THREAD_UNLOCK(&orte_gpr_replica_mutex);

    orte_gpr_replica_process_callbacks();

    return rc;
}


int orte_gpr_replica_delete_object_nl(ompi_registry_mode_t addr_mode,
				     orte_gpr_replica_segment_t *seg,
				     orte_gpr_replica_key_t *keys,
				     int num_keys)
{
    orte_gpr_replica_core_t *reg, *next;
    int count;
    orte_gpr_replica_trigger_list_t *trig;

    if (orte_gpr_replica_debug) {
	ompi_output(0, "[%d,%d,%d] replica_delete_object entered: segment %s",
		    ORTE_NAME_ARGS(*ompi_rte_get_self()), seg->name);
    }

    /* traverse the segment's registry, looking for matching tokens per the specified mode */
    count = 0;
    for (reg = (orte_gpr_replica_core_t*)ompi_list_get_first(&seg->registry_entries);
	 reg != (orte_gpr_replica_core_t*)ompi_list_get_end(&seg->registry_entries);
	 ) {

	next = (orte_gpr_replica_core_t*)ompi_list_get_next(reg);

	/* for each registry entry, check the key list */
	if (orte_gpr_replica_check_key_list(addr_mode, num_keys, keys,
				       reg->num_keys, reg->keys)) { /* found the key(s) on the list */
	    count++;
	    ompi_list_remove_item(&seg->registry_entries, &reg->item);
	}
	reg = next;
    }


    /* update trigger counters */
    for (trig = (orte_gpr_replica_trigger_list_t*)ompi_list_get_first(&seg->triggers);
	 trig != (orte_gpr_replica_trigger_list_t*)ompi_list_get_end(&seg->triggers);
	 trig = (orte_gpr_replica_trigger_list_t*)ompi_list_get_next(trig)) {
	if (orte_gpr_replica_check_key_list(trig->addr_mode, trig->num_keys, trig->keys,
				       num_keys, keys)) {
	    trig->count = trig->count - count;
	}
    }

    return OMPI_SUCCESS;
}

ompi_list_t* orte_gpr_replica_index(char *segment)
{
    ompi_list_t* list;
    orte_gpr_replica_segment_t *seg;

    if (orte_gpr_replica_compound_cmd_mode) {
	   orte_gpr_base_pack_index(orte_gpr_replica_compound_cmd, segment);
	   return NULL;
    }

    OMPI_THREAD_LOCK(&orte_gpr_replica_mutex);

    if (NULL == segment) {  /* want global level index */
	   seg = NULL;
    } else {
	   /* locate the segment */
	   seg = orte_gpr_replica_find_seg(false, segment, ORTE_JOBID_MAX);
	   if (NULL == seg) {
            OMPI_THREAD_UNLOCK(&orte_gpr_replica_mutex);
	       return NULL;
	   }
    }

    list = orte_gpr_replica_index_nl(seg);

    OMPI_THREAD_UNLOCK(&orte_gpr_replica_mutex);
    return list;
}

ompi_list_t* orte_gpr_replica_index_nl(orte_gpr_replica_segment_t *seg)
{
    ompi_list_t *answer;
    orte_gpr_replica_keytable_t *ptr;
    ompi_registry_index_value_t *ans;

    if (orte_gpr_replica_debug) {
	ompi_output(0, "[%d,%d,%d] gpr replica: index entered segment: %s",
		    ORTE_NAME_ARGS(*ompi_rte_get_self()), seg->name);
    }

    answer = OBJ_NEW(ompi_list_t);

    if (NULL == seg) { /* looking for index of global registry */
	for (ptr = (orte_gpr_replica_keytable_t*)ompi_list_get_first(&orte_gpr_replica_head.segment_dict);
	     ptr != (orte_gpr_replica_keytable_t*)ompi_list_get_end(&orte_gpr_replica_head.segment_dict);
	     ptr = (orte_gpr_replica_keytable_t*)ompi_list_get_next(ptr)) {
	    ans = OBJ_NEW(ompi_registry_index_value_t);
	    ans->token = strdup(ptr->token);
	    ompi_list_append(answer, &ans->item);
	}
    } else {  /* want index of specific segment */
	for (ptr = (orte_gpr_replica_keytable_t*)ompi_list_get_first(&seg->keytable);
	     ptr != (orte_gpr_replica_keytable_t*)ompi_list_get_end(&seg->keytable);
	     ptr = (orte_gpr_replica_keytable_t*)ompi_list_get_next(ptr)) {
	    ans = OBJ_NEW(ompi_registry_index_value_t);
	    ans->token = strdup(ptr->token);
	    ompi_list_append(answer, &ans->item);
	}

    }
    return answer;
}

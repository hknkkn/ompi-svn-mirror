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
#include "mca/ns/ns_types.h"

#include "gpr_replica_api.h"

int orte_gpr_replica_put(orte_gpr_addr_mode_t mode, char *segment,
       char **tokens, size_t cnt, orte_gpr_keyval_t **keyvals)
{
#if 0
    int rc;
    int8_t action_taken;
    orte_gpr_replica_segment_t *seg;
    orte_gpr_replica_key_t *keys;
    int num_keys;

    /* protect ourselves against errors */
    if (NULL == segment || NULL == keyvals || 0 == cnt ||
	NULL == tokens || NULL == *tokens) {
	if (orte_gpr_replica_globals.debug) {
	    ompi_output(0, "[%d,%d,%d] gpr replica: error in input - put rejected",
                        ORTE_NAME_ARGS(*(orte_process_info.my_name)));
	}
	return ORTE_ERROR;
    }

    if (orte_gpr_replica_compound_cmd_mode) {
	return orte_gpr_base_pack_put(orte_gpr_replica_compound_cmd,
				     orte_gpr_replica_silent_mode,
				     addr_mode, segment,
				     tokens, object, size);
    }

    OMPI_THREAD_LOCK(&orte_gpr_replica_mutex);

    /* find the segment */
    seg = orte_gpr_replica_find_seg(true, segment,
				   ompi_name_server.get_jobid(ompi_rte_get_self()));

    if (NULL == seg) { /* couldn't find segment or create it */
        OMPI_THREAD_UNLOCK(&orte_gpr_replica_mutex);
	   return ORTE_ERROR;
    }

    /* convert tokens to array of keys */
    keys = orte_gpr_replica_get_key_list(seg, tokens, &num_keys);

    rc = orte_gpr_replica_put_nl(addr_mode, seg, keys, num_keys,
				object, size, &action_taken);

    orte_gpr_replica_check_subscriptions(seg, action_taken);

    orte_gpr_replica_check_synchros(seg);

    /* release list of keys */
    if (NULL != keys) {
	   free(keys);
    }

    OMPI_THREAD_UNLOCK(&orte_gpr_replica_mutex);

    orte_gpr_replica_process_callbacks();

    return rc;
#endif
    return ORTE_ERR_NOT_IMPLEMENTED;
}


int orte_gpr_replica_put_nb(orte_gpr_addr_mode_t addr_mode, char *segment,
                      char **tokens, size_t cnt, orte_gpr_keyval_t **keyvals,
                      orte_gpr_notify_cb_fn_t cbfunc, void *user_tag)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}


int orte_gpr_replica_get(orte_gpr_addr_mode_t addr_mode,
                                char *segment, char **tokens, char **keys,
                                size_t *cnt, orte_gpr_keyval_t **keyvals)
{
#if 0
    ompi_list_t* list;
    orte_gpr_replica_segment_t *seg;
    orte_gpr_replica_key_t *keys;
    int num_keys;

    list = OBJ_NEW(ompi_list_t);

    /* protect against errors */
    if (NULL == segment) {
	   return list;
    }

    if (orte_gpr_replica_compound_cmd_mode) {
	   orte_gpr_base_pack_get(orte_gpr_replica_compound_cmd, addr_mode, segment, tokens);
	return list;
    }

    OMPI_THREAD_LOCK(&orte_gpr_replica_mutex);

    /* find the specified segment */
    seg = orte_gpr_replica_find_seg(false, segment, MCA_NS_BASE_JOBID_MAX);
    if (NULL == seg) {  /* segment not found */
        OMPI_THREAD_UNLOCK(&orte_gpr_replica_mutex);
	   return list;
    }

    /* convert tokens to array of keys */
    keys = orte_gpr_replica_get_key_list(seg, tokens, &num_keys);

    orte_gpr_replica_get_nl(list, addr_mode, seg, keys, num_keys);

    if (NULL != keys) {
	   free(keys);
    }

    OMPI_THREAD_UNLOCK(&orte_gpr_replica_mutex);
    return list;
#endif
    return ORTE_ERR_NOT_IMPLEMENTED;
}


int orte_gpr_replica_get_nb(orte_gpr_addr_mode_t addr_mode,
                                char *segment, char **tokens, char **keys,
                                orte_gpr_notify_cb_fn_t cbfunc, void *user_tag)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

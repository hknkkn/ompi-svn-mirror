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

#include "mca/ns/ns.h"

#include "gpr_replica_fn.h"

int orte_gpr_replica_synchro_fn(orte_gpr_addr_mode_t addr_mode,
                            orte_gpr_synchro_mode_t synchro_mode,
                            orte_gpr_replica_segment_t *seg,
                            orte_gpr_replica_itag_t *token_tags, int num_tokens,
                            orte_gpr_replica_itag_t *key_tags, int num_keys,
                            int trigger,
                            orte_gpr_notify_id_t local_idtag)
{
#if 0
    mca_gpr_replica_trigger_list_t *trig;

    if (mca_gpr_replica_debug) {
	ompi_output(0, "[%d,%d,%d] gpr replica: synchro entered on segment %s trigger %d",
		    ORTE_NAME_ARGS(*ompi_rte_get_self()), seg->name, trigger);
    }

    /* construct the trigger */
    if (NULL != (trig = mca_gpr_replica_construct_trigger(synchro_mode,
							  OMPI_REGISTRY_NOTIFY_NONE,
							  addr_mode, seg, keys, num_keys,
							  trigger, id_tag, jobid))) {
	   return OMPI_SUCCESS;
    } else {
	   return OMPI_ERROR;
    }
#endif
    return ORTE_ERR_NOT_IMPLEMENTED;
}


int orte_gpr_replica_cancel_synchro_fn(orte_gpr_notify_id_t synch_number)
{
#if 0
    if (mca_gpr_replica_debug) {
	ompi_output(0, "[%d,%d,%d] gpr replica: cancel_synchro entered for synch %d",
		    ORTE_NAME_ARGS(*ompi_rte_get_self()), synch_number);
    }

    /* find trigger on replica and remove it - return requestor's id_tag */
    return mca_gpr_replica_remove_trigger(synch_number);
#endif
    return ORTE_ERR_NOT_IMPLEMENTED;
}



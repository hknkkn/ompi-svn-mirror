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

int mca_gpr_replica_synchro_fn(ompi_registry_synchro_mode_t synchro_mode,
			       ompi_registry_mode_t addr_mode,
			       mca_gpr_replica_segment_t *seg,
			       mca_gpr_replica_key_t *keys,
			       int num_keys,
			       int trigger,
			       ompi_registry_notify_id_t id_tag,
                    orte_jobid_t jobid)
{
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
}


ompi_registry_notify_id_t
mca_gpr_replica_cancel_synchro_fn(ompi_registry_notify_id_t synch_number)
{

    if (mca_gpr_replica_debug) {
	ompi_output(0, "[%d,%d,%d] gpr replica: cancel_synchro entered for synch %d",
		    ORTE_NAME_ARGS(*ompi_rte_get_self()), synch_number);
    }

    /* find trigger on replica and remove it - return requestor's id_tag */
    return mca_gpr_replica_remove_trigger(synch_number);

}



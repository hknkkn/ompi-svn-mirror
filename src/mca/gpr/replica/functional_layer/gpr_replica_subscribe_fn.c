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

int mca_gpr_replica_subscribe_fn(ompi_registry_mode_t addr_mode,
				 ompi_registry_notify_action_t action,
				 mca_gpr_replica_segment_t *seg,
				 mca_gpr_replica_key_t *keys,
				 int num_keys,
				 ompi_registry_notify_id_t id_tag,
                  orte_jobid_t jobid)
{
    mca_gpr_replica_trigger_list_t *trig;
    ompi_registry_notify_message_t *notify_msg;
    ;

    if (mca_gpr_replica_debug) {
	ompi_output(0, "[%d,%d,%d] gpr replica: subscribe entered: segment %s",
		    ORTE_NAME_ARGS(*ompi_rte_get_self()), seg->name);
    }

    /* construct the trigger */
    if (NULL != (trig = mca_gpr_replica_construct_trigger(OMPI_REGISTRY_SYNCHRO_MODE_NONE, action,
							  addr_mode, seg, keys, num_keys,
							  0, id_tag, jobid))) {

        	if ((OMPI_REGISTRY_NOTIFY_PRE_EXISTING & action) && seg->triggers_active) {  /* want list of everything there */
        	    notify_msg = mca_gpr_replica_construct_notify_message(seg, trig);
        	    notify_msg->trig_action = action;
        	    notify_msg->trig_synchro = OMPI_REGISTRY_SYNCHRO_MODE_NONE;
        	    mca_gpr_replica_process_triggers(seg, trig, notify_msg);
        	}
        	return OMPI_SUCCESS;
    } else {
	   return OMPI_ERROR;
    }
}


ompi_registry_notify_id_t
mca_gpr_replica_unsubscribe_fn(ompi_registry_notify_id_t sub_number)
{

    if (mca_gpr_replica_debug) {
	ompi_output(0, "[%d,%d,%d] gpr replica: unsubscribe entered for sub number %d",
		    ORTE_NAME_ARGS(*ompi_rte_get_self()), sub_number);
    }

    /* find trigger on replica and remove it - return requestor's id_tag */
    return mca_gpr_replica_remove_trigger(sub_number);

}

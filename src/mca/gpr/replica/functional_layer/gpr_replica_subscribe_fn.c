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

#include "gpr_replica_fn.h"

int orte_gpr_replica_subscribe_fn(orte_gpr_addr_mode_t addr_mode,
                            orte_gpr_notify_action_t action,
                            orte_gpr_replica_segment_t *seg,
                            orte_gpr_replica_itag_t *tokentags, int num_tokens,
                            orte_gpr_replica_itag_t *keytags, int num_keys,
                            orte_gpr_notify_id_t local_idtag)
{
    orte_gpr_replica_notify_tracker_t *trackptr;
    int i;

    if (orte_gpr_replica_globals.debug) {
	   ompi_output(0, "[%d,%d,%d] gpr replica: subscribe entered: segment %s",
		    ORTE_NAME_ARGS(*orte_process_info.my_name), seg->name);
    }

    trackptr = (orte_gpr_replica_notify_tracker_t*)((orte_gpr_replica.triggers)->addr[local_idtag]);
    if (NULL == trackptr) {
        return ORTE_ERR_BAD_PARAM;
    }
    
    trackptr->addr_mode = addr_mode;

    if (num_tokens != orte_value_array_set_size(&(trackptr->tokentags), num_tokens)) {
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    for (i=0; i < num_tokens; i++) {
        ORTE_VALUE_ARRAY_SET_ITEM(&(trackptr->tokentags), orte_gpr_replica_itag_t,
                                        i, tokentags[i]);
    }
    
    if (num_tokens != orte_value_array_set_size(&(trackptr->keytags), num_keys)) {
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    for (i=0; i < num_keys; i++) {
        ORTE_VALUE_ARRAY_SET_ITEM(&(trackptr->keytags), orte_gpr_replica_itag_t,
                                        i, keytags[i]);
    }

#if 0
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
#endif
    return ORTE_SUCCESS;
}


int orte_gpr_replica_unsubscribe_fn(orte_gpr_notify_id_t sub_number)
{
    if (orte_gpr_replica_globals.debug) {
	   ompi_output(0, "[%d,%d,%d] gpr replica: unsubscribe entered for sub number %d",
		    ORTE_NAME_ARGS(*orte_process_info.my_name), sub_number);
    }

    /* find trigger on replica and remove it - return requestor's id_tag */
    return orte_gpr_replica_remove_trigger(sub_number);

}

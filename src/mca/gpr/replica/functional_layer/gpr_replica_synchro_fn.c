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
                            orte_gpr_replica_segment_t *seg,
                            orte_gpr_replica_itag_t *tokentags, int num_tokens,
                            orte_gpr_replica_itag_t *keytags, int num_keys,
                            int trigger,
                            orte_gpr_notify_id_t local_idtag)
{
    orte_gpr_replica_triggers_t *trig;
    int i;

    if (orte_gpr_replica_globals.debug) {
      ompi_output(0, "[%d,%d,%d] gpr replica: synchro entered: segment %s",
         ORTE_NAME_ARGS(orte_process_info.my_name), seg->name);
    }

    trig = (orte_gpr_replica_triggers_t*)((orte_gpr_replica.triggers)->addr[local_idtag]);
    if (NULL == trig) {
        return ORTE_ERR_BAD_PARAM;
    }
    
    trig->addr_mode = addr_mode;

    if (num_tokens != orte_value_array_set_size(&(trig->tokentags), num_tokens)) {
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    for (i=0; i < num_tokens; i++) {
        ORTE_VALUE_ARRAY_SET_ITEM(&(trig->tokentags), orte_gpr_replica_itag_t,
                                        i, tokentags[i]);
    }
    
    if (num_keys != orte_value_array_set_size(&(trig->keytags), num_keys)) {
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    for (i=0; i < num_keys; i++) {
        ORTE_VALUE_ARRAY_SET_ITEM(&(trig->keytags), orte_gpr_replica_itag_t,
                                        i, keytags[i]);
    }

    trig->trigger = trigger;
    
    /* need to check the existing data to flag those that fit the new trigger */
    return orte_gpr_replica_init_trigger(seg, trig);

}


int orte_gpr_replica_cancel_synchro_fn(orte_gpr_notify_id_t synch_number)
{
    orte_gpr_replica_triggers_t *trig;

    if (orte_gpr_replica_globals.debug) {
     ompi_output(0, "[%d,%d,%d] gpr replica: cancel synchro entered for synch %d",
         ORTE_NAME_ARGS(orte_process_info.my_name), synch_number);
    }

    /* release trigger on replica and remove it */
    trig = (orte_gpr_replica_triggers_t*)((orte_gpr_replica.triggers)->addr[synch_number]);
    if (NULL == trig) {
        return ORTE_ERR_BAD_PARAM;
    }
    OBJ_RELEASE(trig);
    
    orte_pointer_array_set_item(orte_gpr_replica.triggers, synch_number, NULL);
    return ORTE_SUCCESS;
}



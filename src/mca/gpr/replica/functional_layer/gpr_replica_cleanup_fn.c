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

#include "mca/ns/ns_types.h"

#include "gpr_replica_fn.h"


int mca_gpr_replica_cleanup_job_fn(orte_jobid_t jobid)
{
    mca_gpr_replica_segment_t *seg, *next_seg;
    mca_gpr_replica_trigger_list_t *trig, *next_trig;

    /* traverse the registry */
    for (seg = (mca_gpr_replica_segment_t*)ompi_list_get_first(&mca_gpr_replica_head.registry);
	 seg != (mca_gpr_replica_segment_t*)ompi_list_get_end(&mca_gpr_replica_head.registry);) {

	next_seg = (mca_gpr_replica_segment_t*)ompi_list_get_next(seg);

	if (jobid == seg->owning_job) {  /* this is a segment associated with this jobid - remove it */

	    mca_gpr_replica_delete_segment_nl(seg);

	} else {  /* check this seg subscriptions/synchros with recipients from this jobid */
	    for (trig = (mca_gpr_replica_trigger_list_t*)ompi_list_get_first(&seg->triggers);
		 trig != (mca_gpr_replica_trigger_list_t*)ompi_list_get_end(&seg->triggers);) {

		next_trig = (mca_gpr_replica_trigger_list_t*)ompi_list_get_next(trig);

		if (trig->owning_job == jobid) {
		    mca_gpr_replica_remove_trigger(trig->local_idtag);
		}
		trig = next_trig;
	    }
	}
	seg = next_seg;
    }
}


int mca_gpr_replica_cleanup_proc_fn(bool purge, orte_process_name_t *proc)
{
    mca_gpr_replica_segment_t *seg;
    mca_gpr_replica_trigger_list_t *trig;
    char *procname;
    orte_jobid_t jobid;

	if (mca_gpr_replica_debug) {
		ompi_output(0, "[%d,%d,%d] gpr_replica_cleanup_proc: function entered for process [%d,%d,%d]",
					ORTE_NAME_ARGS(*ompi_rte_get_self()), ORTE_NAME_ARGS(*proc));
	}
	
    if (ORTE_SUCCESS != orte_name_services.get_proc_name_string(procname, proc)) {
        return;
    }
    if (ORTE_SUCCESS != orte_name_services.get_jobid(&jobid, proc)) {
        return;
    }

    /* search all segments for this process name - remove all references
     */
    for (seg = (mca_gpr_replica_segment_t*)ompi_list_get_first(&mca_gpr_replica_head.registry);
	 seg != (mca_gpr_replica_segment_t*)ompi_list_get_end(&mca_gpr_replica_head.registry);
	 seg = (mca_gpr_replica_segment_t*)ompi_list_get_next(seg)) {

        	if (jobid == seg->owning_job) {
        	    /* adjust any startup synchro synchros owned
        	     * by the associated jobid by one.
        	     */
        		if (mca_gpr_replica_debug) {
        			ompi_output(0, "[%d,%d,%d] gpr_replica_cleanup_proc: adjusting synchros for segment %s",
        						ORTE_NAME_ARGS(*ompi_rte_get_self()), seg->name);
        		}
        		
        	    for (trig = (mca_gpr_replica_trigger_list_t*)ompi_list_get_first(&seg->triggers);
        		 	trig != (mca_gpr_replica_trigger_list_t*)ompi_list_get_end(&seg->triggers);
        		 	trig = (mca_gpr_replica_trigger_list_t*)ompi_list_get_next(trig)) {
        			if (OMPI_REGISTRY_SYNCHRO_MODE_STARTUP & trig->synch_mode) {
        				if (mca_gpr_replica_debug) {
        					ompi_output(0, "\tadjusting startup synchro");
        				}
        		    		trig->count--;
        			}
                 if (mca_gpr_replica_debug) {
                    ompi_output(0, "\ttrigger level %d current count %d", trig->trigger, trig->count);
                 }
        	    }
        	    mca_gpr_replica_check_synchros(seg);
        	}

        	if (purge) {
        	    /* remove name from the dictionary and set all associated object keys to invalid */
        	    mca_gpr_replica_delete_key(seg, procname);
        	}
    }

    if (purge) {
	/* purge all subscriptions with this process as recipient */
	mca_gpr_replica_purge_subscriptions(proc);
    }

    return;
}

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

#include "include/orte_names.h"

#include "class/orte_pointer_array.h"
#include "util/output.h"
#include "util/proc_info.h"

#include "mca/ns/ns.h"

#include "mca/gpr/replica/transition_layer/gpr_replica_tl.h"

#include "gpr_replica_fn.h"


int orte_gpr_replica_cleanup_job_fn(orte_jobid_t jobid)
{
    int rc;
    char *jobidstring, *segment;
    orte_gpr_replica_segment_t *seg;
    orte_jobid_t jobid2;
    orte_gpr_replica_notify_tracker_t *trig;
    int i;
    
    if (ORTE_SUCCESS != orte_ns.convert_jobid_to_string(&jobidstring, jobid)) {
        return ORTE_ERR_BAD_PARAM;
    }
    
    asprintf(&segment, "%s-%s", ORTE_JOB_SEGMENT, jobidstring);
    
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_find_seg(&seg, false, segment))) {
        return rc;
    }
    
    /* delete the associated job segment */
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_release_segment(&seg))) {
        return rc;
    }
    
    /* traverse the registry's triggers and remove all with recipients from this jobid */
    trig = (orte_gpr_replica_notify_tracker_t*)((orte_gpr_replica.triggers)->addr);
    for (i=0; i < (orte_gpr_replica.triggers)->size; i++) {
        if ((NULL != trig) &&
            (ORTE_SUCCESS == (rc = orte_ns.get_jobid(&jobid2, trig->requestor))) &&
            (jobid == jobid2)) {
                if (ORTE_SUCCESS != (rc = orte_pointer_array_set_item(
                              orte_gpr_replica.triggers, trig->local_idtag, NULL))) {
                    return rc;
                }
        }
        trig++;
    }
    return rc;
}


int orte_gpr_replica_cleanup_proc_fn(orte_process_name_t *proc)
{
    orte_gpr_replica_segment_t *seg;
    orte_gpr_replica_itag_t itag;
    orte_gpr_notify_id_t *trigid;
    orte_gpr_replica_notify_tracker_t **tracks;
    char *procname, *segment, *jobidstring;
    orte_jobid_t jobid;
    int rc, i;

	if (orte_gpr_replica_globals.debug) {
		ompi_output(0, "[%d,%d,%d] gpr_replica_cleanup_proc: function entered for process [%d,%d,%d]",
					ORTE_NAME_ARGS(*(orte_process_info.my_name)), ORTE_NAME_ARGS(*proc));
	}
	
    if (ORTE_SUCCESS != (rc = orte_ns.get_proc_name_string(&procname, proc))) {
        return rc;
    }

    /* search all segments for this process name - remove all references
     */
    seg = (orte_gpr_replica_segment_t*)((orte_gpr_replica.segments)->addr);
    for (i=0; i < (orte_gpr_replica.segments)->size; i++) {
        if (NULL != seg) {
            if (ORTE_SUCCESS == orte_gpr_replica_dict_lookup(&itag, seg, procname)) {
                if (ORTE_SUCCESS != (rc = orte_pointer_array_set_item(seg->dict, itag, NULL))) {
                    return rc;
                }
            }
        }
        seg++;
    }
    
    /* adjust synchros on the job segment */
    if (ORTE_SUCCESS != orte_ns.get_jobid(&jobid, proc)) {
        return ORTE_ERR_BAD_PARAM;
    }
    
    if (ORTE_SUCCESS != orte_ns.convert_jobid_to_string(&jobidstring, jobid)) {
        return ORTE_ERR_BAD_PARAM;
    }
    
    asprintf(&segment, "%s-%s", ORTE_JOB_SEGMENT, jobidstring);
    
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_find_seg(&seg, false, segment))) {
        return rc;
    }
    
    trigid = (orte_gpr_notify_id_t*)seg->triggers;
    tracks = (orte_gpr_replica_notify_tracker_t**)((orte_gpr_replica.triggers)->addr);
    for (i=0; i < seg->num_trigs; i++) {
        if (NULL != tracks[*trigid] &&
            ORTE_GPR_SYNCHRO_CMD == (tracks[*trigid])->cmd) {
                (tracks[*trigid])->trigger--;
        }
        trigid++;
    }
    
    return orte_gpr_replica_check_synchros(seg);

}

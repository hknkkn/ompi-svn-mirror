/* -*- C -*-
 *
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
 * The Open MPI General Purpose Registry - Replica component
 *
 */

/*
 * includes
 */
#include "orte_config.h"

#include "mca/ns/ns.h"

#include "gpr_replica_fn.h"


int orte_gpr_replica_notify_on_fn(orte_process_name_t *proc,
				  orte_gpr_notify_id_t sub_number)
{
#if 0
    orte_gpr_replica_notify_off_t *ptr, *nextptr;
    int cmpval;

    for (ptr = (orte_gpr_replica_notify_off_t*)ompi_list_get_first(&orte_gpr_replica_notify_off_list);
	 ptr != (orte_gpr_replica_notify_off_t*)ompi_list_get_end(&orte_gpr_replica_notify_off_list);
	 ) {
	nextptr = (orte_gpr_replica_notify_off_t*)ompi_list_get_next(ptr);
    if (ORTE_SUCCESS != orte_name_services.compare(&cmpval, ORTE_NS_CMP_ALL, ptr->proc, proc)) {
        return;
    }
    if (0 == cmpval) {
	    if ((OMPI_REGISTRY_NOTIFY_ID_MAX == sub_number) ||
		(ptr->sub_number == sub_number)) {
		ompi_list_remove_item(&orte_gpr_replica_notify_off_list, &ptr->item);
		OBJ_RELEASE(ptr);
	    }
	}
	ptr = nextptr;
    }
#endif
    return ORTE_ERR_NOT_IMPLEMENTED;
}


int orte_gpr_replica_notify_off_fn(orte_process_name_t *proc,
				   orte_gpr_notify_id_t sub_number)
{
#if 0
    orte_gpr_replica_notify_off_t *ptr;
    int cmpval;

    /* check to see if this is already on the list - return if so */
    for (ptr = (orte_gpr_replica_notify_off_t*)ompi_list_get_first(&orte_gpr_replica_notify_off_list);
	 ptr != (orte_gpr_replica_notify_off_t*)ompi_list_get_end(&orte_gpr_replica_notify_off_list);
	 ptr = (orte_gpr_replica_notify_off_t*)ompi_list_get_next(ptr)) {
    if (ORTE_SUCCESS != orte_name_services.compare(&cmpval, ORTE_NS_CMP_ALL, ptr->proc, proc)) {
        return;
    }
    if (0 == cmpval) {
	    if (OMPI_REGISTRY_NOTIFY_ID_MAX == sub_number) { /* if wild card, remove all others on list */
		ompi_list_remove_item(&orte_gpr_replica_notify_off_list, &ptr->item);
		OBJ_RELEASE(ptr);
	    } else if (ptr->sub_number == sub_number) {
		return;
	    }
	}
    }

    /* either wild card or not already on list - add it */
    ptr = OBJ_NEW(orte_gpr_replica_notify_off_t);
    ptr->sub_number = sub_number;
    if (ORTE_SUCCESS != orte_name_services.copy_process_name(ptr->proc, proc)) {
        return;
    }
    ompi_list_append(&orte_gpr_replica_notify_off_list, &ptr->item);
#endif
    return ORTE_ERR_NOT_IMPLEMENTED;
}


int orte_gpr_replica_triggers_active_fn(orte_jobid_t jobid)
{
#if 0
    orte_gpr_replica_segment_t *seg;
    orte_gpr_replica_notify_off_t *ptr, *nextptr;
    orte_jobid_t procjobid;

	if (orte_gpr_replica_debug) {
		ompi_output(0, "[%d,%d,%d] setting triggers active for job %d",
					ORTE_NAME_ARGS(*ompi_rte_get_self()), (int)jobid);
	}

    /* traverse the registry */
    /* enable triggers on segments from this jobid */
    for (seg = (orte_gpr_replica_segment_t*)ompi_list_get_first(&orte_gpr_replica_head.registry);
	 seg != (orte_gpr_replica_segment_t*)ompi_list_get_end(&orte_gpr_replica_head.registry);
	 seg = (orte_gpr_replica_segment_t*)ompi_list_get_next(seg)) {

	if (seg->owning_job == jobid) {
		if (orte_gpr_replica_debug) {
			ompi_output(0, "[%d,%d,%d] setting triggers active for segment %s",
						ORTE_NAME_ARGS(*ompi_rte_get_self()), seg->name);
		}
		
	    seg->triggers_active = true;
	}
    }

    /* check the list of process names with notification turned off
     * and turn on those from this jobid
     */
    for (ptr = (orte_gpr_replica_notify_off_t*)ompi_list_get_first(&orte_gpr_replica_notify_off_list);
	 ptr != (orte_gpr_replica_notify_off_t*)ompi_list_get_end(&orte_gpr_replica_notify_off_list);
	 ) {

	nextptr = (orte_gpr_replica_notify_off_t*)ompi_list_get_next(ptr);

    if (ORTE_SUCCESS != orte_name_services.get_jobid(&procjobid, ptr->proc)) {
        return;
    }
	if (jobid == procjobid) {
	    ompi_list_remove_item(&orte_gpr_replica_notify_off_list, &ptr->item);
	    OBJ_RELEASE(ptr);
	}
	ptr = nextptr;
    }
#endif
    return ORTE_ERR_NOT_IMPLEMENTED;
}


int orte_gpr_replica_triggers_inactive_fn(orte_jobid_t jobid)
{
#if 0
    orte_gpr_replica_segment_t *seg;

    /* traverse the registry */
    /* disable triggers on segments from this jobid */
    for (seg = (orte_gpr_replica_segment_t*)ompi_list_get_first(&orte_gpr_replica_head.registry);
	 seg != (orte_gpr_replica_segment_t*)ompi_list_get_end(&orte_gpr_replica_head.registry);
	 seg = (orte_gpr_replica_segment_t*)ompi_list_get_next(seg)) {

	if (seg->owning_job == jobid) {
	    seg->triggers_active = false;
	}
    }
#endif
    return ORTE_ERR_NOT_IMPLEMENTED;
}


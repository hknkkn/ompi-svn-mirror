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

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

#include "util/proc_info.h"
#include "mca/ns/ns_types.h"

#include "gpr_replica_api.h"


int orte_gpr_replica_notify_on(orte_gpr_notify_id_t sub_number)
{
    int rc;
    
    OMPI_THREAD_LOCK(&orte_gpr_replica_mutex);
    rc = orte_gpr_replica_notify_on_fn(orte_process_info.my_name, sub_number);
    OMPI_THREAD_UNLOCK(&orte_gpr_replica_mutex);
    
    return rc;
}

int orte_gpr_replica_notify_off(orte_gpr_notify_id_t sub_number)
{
    int rc;
    
    OMPI_THREAD_LOCK(&orte_gpr_replica_mutex);
    rc = orte_gpr_replica_notify_off_fn(orte_process_info.my_name, sub_number);
    OMPI_THREAD_UNLOCK(&orte_gpr_replica_mutex);
    
    return rc;
}

int orte_gpr_replica_triggers_active(orte_jobid_t jobid)
{
    int rc;
    
    OMPI_THREAD_LOCK(&orte_gpr_replica_mutex);
    rc = orte_gpr_replica_triggers_active_fn(jobid);
    OMPI_THREAD_UNLOCK(&orte_gpr_replica_mutex);
    
    return rc;
}


int orte_gpr_replica_triggers_inactive(orte_jobid_t jobid)
{
    int rc;
    
    OMPI_THREAD_LOCK(&orte_gpr_replica_mutex);
    rc = orte_gpr_replica_triggers_inactive_fn(jobid);
    OMPI_THREAD_UNLOCK(&orte_gpr_replica_mutex);
    
    return rc;
}

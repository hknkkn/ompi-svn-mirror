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

#include "include/orte_constants.h"

#include "dps/dps_types.h"
#include "util/output.h"
#include "util/proc_info.h"

#include "mca/ns/ns_types.h"

#include "gpr_replica_api.h"
#include "mca/gpr/replica/functional_layer/gpr_replica_fn.h"

int orte_gpr_replica_define_job_segment(orte_jobid_t jobid, int num_procs)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}


int orte_gpr_replica_get_startup_msg(orte_jobid_t jobid,
                                    orte_buffer_t **msg,
                                    size_t *cnt,
                                    orte_process_name_t **procs)
{
    int rc;
    
    if (orte_gpr_replica_debug) {
	   ompi_output(0, "[%d,%d,%d] entered get_startup_msg",
		    ORTE_NAME_ARGS(*(orte_process_info.my_name)));
    }

    *cnt = 0;
    if (NULL != *procs) {
        return ORTE_ERR_BAD_PARAM;
    }
    *procs = NULL;
    
    if (NULL != *msg) {
        OBJ_RELEASE(*msg);
    }
    
    *msg = OBJ_NEW(orte_buffer_t);
    if (NULL == *msg) {
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    OMPI_THREAD_LOCK(&orte_gpr_replica_mutex);

    rc = orte_gpr_replica_get_startup_msg_fn(jobid, msg, cnt, procs);

    OMPI_THREAD_UNLOCK(&orte_gpr_replica_mutex);

    return rc;
}

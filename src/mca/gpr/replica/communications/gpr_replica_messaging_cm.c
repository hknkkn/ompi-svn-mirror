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

#include "dps/dps.h"
#include "mca/errmgr/errmgr.h"

#include "gpr_replica_comm.h"

int orte_gpr_replica_recv_get_startup_msg_cmd(orte_buffer_t *buffer,
                                              orte_buffer_t *answer)
{
    orte_gpr_cmd_flag_t command=ORTE_GPR_GET_STARTUP_MSG_CMD;
    orte_jobid_t jobid=0;
    size_t n;
    int rc;
    
    n = 1;
    if (ORTE_SUCCESS != (rc = orte_dps.unpack(buffer, &jobid, &n, ORTE_JOBID))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }

    OMPI_THREAD_LOCK(&orte_gpr_replica_globals.mutex);

    if (ORTE_SUCCESS != (rc = orte_dps.pack(answer, &command, 1, ORTE_GPR_CMD))) {
        ORTE_ERROR_LOG(rc);
        OMPI_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);
        return rc;
    }
    
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_get_startup_msg_fn(jobid, answer))) {
        ORTE_ERROR_LOG(rc);
        OMPI_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);
        return rc;
    }

    OMPI_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);

    return ORTE_SUCCESS;
}



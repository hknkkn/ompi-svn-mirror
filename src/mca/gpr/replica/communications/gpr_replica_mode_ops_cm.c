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

int orte_gpr_replica_recv_notify_on_cmd(orte_buffer_t *cmd, orte_buffer_t *answer)
{
    orte_gpr_cmd_flag_t command=ORTE_GPR_NOTIFY_ON_CMD;
    orte_process_name_t proc;
    orte_gpr_notify_id_t sub_number;
    size_t n;
    int rc, ret;

    if (ORTE_SUCCESS != (rc = orte_dps.pack(answer, &command, 1, ORTE_GPR_CMD))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    
    n = 1;
    if (ORTE_SUCCESS != (rc = orte_dps.unpack(cmd, &proc, &n, ORTE_NAME))) {
        ORTE_ERROR_LOG(rc);
        ret = rc;
        goto RETURN_ERROR;
    }

    n = 1;
    if (ORTE_SUCCESS != (rc = orte_dps.unpack(cmd, &sub_number, &n, ORTE_GPR_NOTIFY_ID))) {
        ORTE_ERROR_LOG(rc);
        ret = rc;
        goto RETURN_ERROR;
    }

    OMPI_THREAD_LOCK(&orte_gpr_replica_globals.mutex);
    ret = orte_gpr_replica_notify_on_fn(&proc, sub_number);
    OMPI_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);

    if (ORTE_SUCCESS != ret) {
        ORTE_ERROR_LOG(ret);
    }

 RETURN_ERROR:
    if (ORTE_SUCCESS != (rc = orte_dps.pack(answer, &ret, 1, ORTE_INT))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    
    return ret;
}

int orte_gpr_replica_recv_notify_off_cmd(orte_buffer_t *cmd, orte_buffer_t *answer)
{
    orte_gpr_cmd_flag_t command=ORTE_GPR_NOTIFY_OFF_CMD;
    orte_process_name_t proc;
    orte_gpr_notify_id_t sub_number;
    size_t n;
    int rc, ret;

    if (ORTE_SUCCESS != (rc = orte_dps.pack(answer, &command, 1, ORTE_GPR_CMD))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }

    n = 1;
    if (ORTE_SUCCESS != (rc = orte_dps.unpack(cmd, &proc, &n, ORTE_NAME))) {
        ORTE_ERROR_LOG(rc);
        ret = rc;
        goto RETURN_ERROR;
    }

    n = 1;
    if (ORTE_SUCCESS != (rc = orte_dps.unpack(cmd, &sub_number, &n, ORTE_GPR_NOTIFY_ID))) {
        ORTE_ERROR_LOG(rc);
        ret = rc;
        goto RETURN_ERROR;
    }

    OMPI_THREAD_LOCK(&orte_gpr_replica_globals.mutex);
    ret = orte_gpr_replica_notify_off_fn(&proc, sub_number);
    OMPI_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);

    if (ORTE_SUCCESS != ret) {
        ORTE_ERROR_LOG(ret);
    }

 RETURN_ERROR:
    if (ORTE_SUCCESS != (rc = orte_dps.pack(answer, &ret, 1, ORTE_INT))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    
    return ret;
}

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

#include "include/orte_constants.h"
#include "include/orte_types.h"
#include "dps/dps.h"

#include "mca/gpr/base/base.h"

int orte_gpr_base_unpack_cleanup_job(orte_buffer_t *cmd)
{
    orte_gpr_cmd_flag_t command;
    int32_t response;
    int rc;
    size_t n;
    
    n=1;
    if (ORTE_SUCCESS != (rc = orte_dps.unpack(cmd, &command, &n, ORTE_GPR_PACK_CMD))) {
        return rc;
    }
    
    if (ORTE_GPR_CLEANUP_JOB_CMD != command) {
        return ORTE_ERR_COMM_FAILURE;
    }

    n=1;
    if (ORTE_SUCCESS != (rc = orte_dps.unpack(cmd, &response, &n, ORTE_INT32))) {
        return rc;
    }

    return (int)response;
}

int orte_gpr_base_unpack_cleanup_proc(orte_buffer_t *cmd)
{
    orte_gpr_cmd_flag_t command;
    int32_t response;
    int rc;
    size_t n;
    
    n=1;
    if (ORTE_SUCCESS != (rc = orte_dps.unpack(cmd, &command, &n, ORTE_GPR_PACK_CMD))) {
        return rc;
    }
    
    if (ORTE_GPR_CLEANUP_PROC_CMD != command) {
        return ORTE_ERR_COMM_FAILURE;
    }

    n=1;
    if (ORTE_SUCCESS != (rc = orte_dps.unpack(cmd, &response, &n, ORTE_INT32))) {
        return rc;
    }

    return (int)response;

}

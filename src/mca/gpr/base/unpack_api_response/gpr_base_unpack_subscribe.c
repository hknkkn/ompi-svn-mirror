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
 * The Open MPI general purpose registry - unpack functions.
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


int orte_gpr_base_unpack_subscribe(orte_buffer_t *buffer, orte_gpr_notify_id_t *remote_idtag)
{
    orte_gpr_cmd_flag_t command;
    size_t n;
    int rc;

    n = 1;
    if (ORTE_SUCCESS != (rc = orte_dps.unpack(buffer, &command, &n, ORTE_GPR_PACK_CMD))) {
        return rc;
    }
    
	if (ORTE_GPR_SUBSCRIBE_CMD != command) {
	   return ORTE_ERR_COMM_FAILURE;
    }

    n = 1;
    if (ORTE_SUCCESS != (rc = orte_dps.unpack(buffer, remote_idtag, &n, ORTE_GPR_PACK_NOTIFY_ID))) {
	   return rc;
    }

    return ORTE_SUCCESS;
}


int orte_gpr_base_unpack_unsubscribe(orte_buffer_t *buffer)
{
    orte_gpr_cmd_flag_t command;
    int32_t response;
    size_t n;
    int rc;

    n = 1;
    if (ORTE_SUCCESS != (rc = orte_dps.unpack(buffer, &command, &n, ORTE_GPR_PACK_CMD))) {
        return rc;
    }
    
	if (ORTE_GPR_UNSUBSCRIBE_CMD != command) {
	   return ORTE_ERR_COMM_FAILURE;
    }

    n = 1;
    if (ORTE_SUCCESS != (rc = orte_dps.unpack(buffer, &response, &n, ORTE_INT32))) {
	   return rc;
    }

    return (int)response;
}

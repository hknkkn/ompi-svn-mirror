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
 * The Open MPI general purpose registry - base unpack functions.
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

int orte_gpr_base_unpack_put(orte_buffer_t *buffer)
{
    orte_gpr_cmd_flag_t command;
    int32_t response;
    int rc;
    size_t n;

    n=1;
    if (ORTE_SUCCESS != (rc = orte_dps.unpack(buffer, &command, &n, ORTE_GPR_PACK_CMD))) {
        return rc;
    }
    
	if (ORTE_GPR_PUT_CMD != command) {
	   return ORTE_ERR_COMM_FAILURE;
    }

    n=1;
    if (ORTE_SUCCESS != (rc = orte_dps.unpack(buffer, &response, &n, ORTE_INT32))) {
	   return rc;
    } else {
	   return (int)response;
    }

}


int orte_gpr_base_unpack_get(orte_buffer_t *buffer, int *cnt, orte_gpr_value_t **values)
{
    orte_gpr_cmd_flag_t command;
    int rc;
    orte_data_type_t type;
    size_t n;

    n=1;
    if (ORTE_SUCCESS != (rc = orte_dps.unpack(buffer, &command, &n, ORTE_GPR_PACK_CMD))) {
        return rc;
    }
    
    if (ORTE_GPR_GET_CMD != command) {
        return ORTE_ERR_COMM_FAILURE;
    }

    /* find out how many values came back */
    if (ORTE_SUCCESS != (rc = orte_dps.peek(buffer, &type, &n))) {
        return rc;
    }
    
    if (ORTE_KEYVAL != type) {
        return ORTE_ERR_COMM_FAILURE;
    }
    
    *values = (orte_gpr_value_t*)malloc(n*sizeof(orte_gpr_value_t*));
    if (NULL == *values) {
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    if (ORTE_SUCCESS != (rc = orte_dps.unpack(buffer, values, &n, ORTE_GPR_VALUE))) {
        return rc;
    }
    
    *cnt = (int)n;

    return ORTE_SUCCESS;
}

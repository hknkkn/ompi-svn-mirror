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
 */

#include "orte_config.h"

#include "include/orte_constants.h"
#include "include/orte_types.h"
#include "dps/dps.h"

#include "mca/gpr/base/base.h"


int orte_gpr_base_unpack_delete_segment(orte_buffer_t *buffer)
{
    orte_gpr_cmd_flag_t command;
    int32_t response;
    int rc;
    size_t n;

    n = 1;
    if (ORTE_SUCCESS != (rc = orte_dps.unpack(buffer, &command, &n, ORTE_GPR_PACK_CMD))) {
        return rc;
    }
    
	if (ORTE_GPR_DELETE_SEGMENT_CMD != command) {
	   return ORTE_ERR_COMM_FAILURE;
    }

    n = 1;
    if (ORTE_SUCCESS != (rc = orte_dps.unpack(buffer, &response, &n, ORTE_INT32))) {
	   return rc;
    } else {
	   return (int)response;
    }
}


int orte_gpr_base_unpack_delete_entries(orte_buffer_t *buffer)
{
    orte_gpr_cmd_flag_t command;
    int32_t response;
    int rc;
    size_t n;

    n = 1;
    if (ORTE_SUCCESS != (rc = orte_dps.unpack(buffer, &command, &n, ORTE_GPR_PACK_CMD))) {
        return rc;
    }
    
    if (ORTE_GPR_DELETE_ENTRIES_CMD != command) {
        return ORTE_ERR_COMM_FAILURE;
    }

    n = 1;
    if (ORTE_SUCCESS != (rc = orte_dps.unpack(buffer, &response, &n, ORTE_INT32))) {
        return rc;
    } else {
        return (int)response;
    }
}


int orte_gpr_base_unpack_index(orte_buffer_t *buffer, size_t *cnt, char **index)
{
    orte_gpr_cmd_flag_t command;
    size_t n;
    orte_data_type_t type;
    int rc;

    *cnt = 0;
    
    n = 1;
    if (ORTE_SUCCESS != (rc = orte_dps.unpack(buffer, &command, &n, ORTE_GPR_PACK_CMD))) {
        return rc;
    }
    
    if (ORTE_GPR_INDEX_CMD != command) {
        return ORTE_ERR_COMM_FAILURE;
    }

    if (ORTE_SUCCESS != (rc = orte_dps.peek(buffer, &type, &n))) {
        return rc;
    }
    
    if (ORTE_STRING != type) {
        return ORTE_ERR_COMM_FAILURE;
    }
    
    if (NULL != index) {
        return ORTE_ERR_BAD_PARAM;
    }
    
    if (0 < n) {
        index = (char **)malloc(n*sizeof(char*));
        if (NULL == index) {
            return ORTE_ERR_OUT_OF_RESOURCE;
        }
    }
    
    /* need to unpack the string regardless of n to "clear" the entry from the buffer */
    if (ORTE_SUCCESS != (rc = orte_dps.unpack(buffer, index, &n, ORTE_STRING))) {
        return rc;
    }
    *cnt = n;

    return ORTE_SUCCESS;
}

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
 * The Open MPI General Purpose Registry - unpack functions
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

int
orte_gpr_base_unpack_get_startup_msg(orte_buffer_t *buffer,
				    orte_buffer_t **msg, size_t *cnt,
                     orte_process_name_t **recipients)
{
    orte_gpr_cmd_flag_t command;
    orte_data_type_t type;
    size_t n;
    int rc;
    uint8_t *bytes;
    
    n=1;
    if (ORTE_SUCCESS != (rc = orte_dps.unpack(buffer, &command, &n, ORTE_GPR_PACK_CMD))) {
        return rc;
    }
    
    if (ORTE_GPR_GET_STARTUP_MSG_CMD != command) {
        	return ORTE_ERR_COMM_FAILURE;
    }

    if (ORTE_SUCCESS != (rc = orte_dps.peek(buffer, &type, &n))) {
        return rc;
    }
    
    if (ORTE_NAME != type) {
        return ORTE_ERR_COMM_FAILURE;
    }
    
    *recipients = (orte_process_name_t*)malloc(n*sizeof(orte_process_name_t*));
    if (NULL == *recipients) {
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    if (ORTE_SUCCESS != (rc = orte_dps.unpack(buffer, recipients, &n, ORTE_NAME))) {
	   return rc;
    }
    *cnt = n;
    
    if (ORTE_SUCCESS != (rc = orte_dps.peek(buffer, &type, &n))) {
        return rc;
    }
    
    if (ORTE_BYTE != type) {
        return ORTE_ERR_COMM_FAILURE;
    }
    
    bytes = (uint8_t*)malloc(n);
    if (NULL == bytes) {
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    if (ORTE_SUCCESS != (rc = orte_dps.unpack(buffer, bytes, &n, ORTE_BYTE))) {
        return rc;
    }
    
    *msg = OBJ_NEW(orte_buffer_t);
    if (ORTE_SUCCESS != (rc = orte_dps.load(*msg, bytes, n))) {
        return rc;
    }
    
    return ORTE_SUCCESS;
}

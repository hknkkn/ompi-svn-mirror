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

int orte_gpr_base_unpack_put(ompi_buffer_t buffer)
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


int orte_gpr_base_unpack_get(ompi_buffer_t buffer, ompi_list_t *returned_list)
{
    orte_gpr_cmd_flag_t command;
    int32_t object_size, num_responses;
    ompi_registry_value_t *newptr;
    ompi_registry_object_t *object;
    int i;

    if ((ORTE_SUCCESS != (rc = orte_dps.unpack(buffer, &command, 1, ORTE_GPR_PACK_CMD))
	|| (MCA_GPR_GET_CMD != command)) {
	return ORTE_ERROR;
    }

    if ((ORTE_SUCCESS != (rc = orte_dps.unpack(buffer, &num_responses, 1, ORTE_INT32)) ||
	(0 >= num_responses)) {
	return ORTE_ERROR;
    }

    for (i=0; i<num_responses; i++) {
	if (ORTE_SUCCESS != (rc = orte_dps.unpack(buffer, &object_size, 1, ORTE_GPR_PACK_OBJECT_SIZE)) {
	    return ORTE_ERROR;
	}
	object = (ompi_registry_object_t)malloc(object_size);
	if (ORTE_SUCCESS != (rc = orte_dps.unpack(buffer, object, object_size, ORTE_BYTE)) {
	    free(object);
	    return ORTE_ERROR;
	}
	newptr = OBJ_NEW(ompi_registry_value_t);
	newptr->object_size = object_size;
	newptr->object = object;
	ompi_list_append(returned_list, &newptr->item);
    }

    return ORTE_SUCCESS;
}

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

int orte_gpr_base_unpack_test_internals(orte_buffer_t *buffer, ompi_list_t *test_results)
{
    char *strings[2];
    int rc;
    int32_t num_responses;
    orte_gpr_internal_test_results_t *newptr=NULL;
    orte_gpr_cmd_flag_t command;
    size_t n, i;

    n = 1;
    if (ORTE_SUCCESS != (rc = orte_dps.unpack(buffer, &command, &n, ORTE_GPR_PACK_CMD))) {
        return rc;
    }
    
	if (ORTE_GPR_TEST_INTERNALS_CMD != command) {
	   return ORTE_ERR_COMM_FAILURE;
    }

    n = 1;
    if (ORTE_SUCCESS != (rc = orte_dps.unpack(buffer, &num_responses, &n, ORTE_INT32))) {
        return rc;
    }
    
	if (0 >= num_responses) {
	   return ORTE_ERR_NOT_AVAILABLE;
    }

    for (i=0; i<num_responses; i++) {
        n = 2;
        if (ORTE_SUCCESS != (rc = orte_dps.unpack(buffer, strings, &n, ORTE_STRING))) {
            return rc;
        }
	   newptr = OBJ_NEW(orte_gpr_internal_test_results_t);
	   newptr->test = strdup(strings[0]);
	   newptr->message = strdup(strings[1]);
	   ompi_list_append(test_results, &newptr->item);
    }

    return ORTE_SUCCESS;
}

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

#include "dps/dps.h"

#include "mca/gpr/base/base.h"

int orte_gpr_base_pack_delete_segment(orte_buffer_t *cmd, char *segment)
{
    orte_gpr_cmd_flag_t command;
    int rc;

    command = ORTE_GPR_DELETE_SEGMENT_CMD;

    if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, &command, 1, ORTE_GPR_PACK_CMD))) {
	   return rc;
    }

    if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, segment, 1, ORTE_STRING))) {
	   return rc;
    }

    return ORTE_SUCCESS;
}


int orte_gpr_base_pack_delete_object(orte_buffer_t *cmd,
                                     orte_gpr_addr_mode_t mode,
			                        char *segment, char **tokens)
{
    orte_gpr_cmd_flag_t command;
    char **tokptr;
    uint32_t num_tokens;
    int rc;

    command = ORTE_GPR_DELETE_OBJECT_CMD;

    if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, &command, 1, ORTE_GPR_PACK_CMD))) {
	   return rc;
    }

    if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, &mode, 1, ORTE_GPR_PACK_ADDR_MODE))) {
	   return rc;
    }

    if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, segment, 1, ORTE_STRING))) {
	   return rc;
    }

    /* compute number of tokens */
    tokptr = tokens;
    num_tokens = 0;
    while (NULL != *tokptr) {
	   num_tokens++;
	   tokptr++;
    }

    if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, &num_tokens, 1, ORTE_UINT32))) {
	   return rc;
    }

    if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, tokens, num_tokens, ORTE_STRING))) {
     return rc;
    }

    return ORTE_SUCCESS;
}


int orte_gpr_base_pack_index(orte_buffer_t *cmd, char *segment)
{
    orte_gpr_cmd_flag_t command;
    orte_gpr_addr_mode_t mode;
    int rc;

    command = ORTE_GPR_INDEX_CMD;

    if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, &command, 1, ORTE_GPR_PACK_CMD))) {
	   return rc;
    }

    if (NULL == segment) {  /* no segment specified - want universe dict */
	    mode = 0;
        	if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, &mode, 1, ORTE_GPR_PACK_ADDR_MODE))) {
        	    return rc;
        	}
    } else {
        	mode = 1;
        	if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, &mode, 1, ORTE_GPR_PACK_ADDR_MODE))) {
        	    return rc;
        	}
        	if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, segment, 1, ORTE_STRING))) {
        	    return rc;
        	}
    }

    return ORTE_SUCCESS;
}

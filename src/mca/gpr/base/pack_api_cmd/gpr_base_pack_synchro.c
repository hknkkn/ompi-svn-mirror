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

int orte_gpr_base_pack_synchro(orte_buffer_t *cmd,
			      orte_gpr_synchro_mode_t synchro_mode,
			      orte_gpr_addr_mode_t mode,
			      char *segment, char **tokens, char **keys, int trigger)
{
    orte_gpr_cmd_flag_t command;
    char **ptr;
    size_t n;
    int rc;

    /* need to protect against errors */
    if (NULL == segment) {
	   return ORTE_ERR_BAD_PARAM;
    }

    command = ORTE_GPR_SYNCHRO_CMD;

    if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, &command, 1, ORTE_GPR_PACK_CMD))) {
	   return rc;
    }

    if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, &synchro_mode, 1, ORTE_GPR_PACK_SYNCHRO_MODE))) {
	   return rc;
    }

    if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, &mode, 1, ORTE_GPR_PACK_ADDR_MODE))) {
	   return rc;
    }

    if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, segment, 1, ORTE_STRING))) {
	   return rc;
    }

    /* compute number of tokens */
    n = 0;
    if (NULL != tokens) {
	   ptr = tokens;
	   while (NULL != ptr[n]) {
	       n++;
	   }
    }

    if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, tokens, n, ORTE_STRING))) {
	   return rc;
    }

    /* compute number of keys */
    n = 0;
    if (NULL != keys) {
       ptr = keys;
       while (NULL != ptr[n]) {
           n++;
       }
    }

    if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, keys, n, ORTE_STRING))) {
      return rc;
    }

    if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, &trigger, 1, ORTE_INT))) {
	   return rc;
    }

    return ORTE_SUCCESS;

}


int orte_gpr_base_pack_cancel_synchro(orte_buffer_t *cmd,
				     orte_gpr_notify_id_t remote_idtag)
{
    orte_gpr_cmd_flag_t command;
    int rc;

    command = ORTE_GPR_CANCEL_SYNCHRO_CMD;

    if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, &command, 1, ORTE_GPR_PACK_CMD))) {
	   return rc;
    }

    if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, &remote_idtag, 1, ORTE_GPR_PACK_NOTIFY_ID))) {
	   return rc;
    }

    return ORTE_SUCCESS;
}

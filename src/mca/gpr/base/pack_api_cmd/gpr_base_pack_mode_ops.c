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

int orte_gpr_base_pack_notify_on(orte_buffer_t *cmd,
				orte_process_name_t *proc,
				orte_gpr_notify_id_t sub_number)
{
    orte_gpr_cmd_flag_t command;
    int rc;

    command = ORTE_GPR_NOTIFY_ON_CMD;

    if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, &command, 1, ORTE_GPR_PACK_CMD))) {
	   return rc;
    }

    if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, proc, 1, ORTE_NAME))) {
	   return rc;
    }

    if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, &sub_number, 1, ORTE_GPR_PACK_NOTIFY_ID))) {
	   return rc;
    }

    return ORTE_SUCCESS;

}

int orte_gpr_base_pack_notify_off(orte_buffer_t *cmd,
				 orte_process_name_t *proc,
				 orte_gpr_notify_id_t sub_number)
{
    orte_gpr_cmd_flag_t command;
    int rc;

    command = ORTE_GPR_NOTIFY_OFF_CMD;

    if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, &command, 1, ORTE_GPR_PACK_CMD))) {
	   return rc;
    }

    if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, proc, 1, ORTE_NAME))) {
	   return rc;
    }

    if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, &sub_number, 1, ORTE_GPR_PACK_NOTIFY_ID))) {
	   return rc;
    }

    return ORTE_SUCCESS;
}

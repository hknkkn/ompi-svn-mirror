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
 * The Open MPI General Purpose Registry - pack functions
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

int orte_gpr_base_pack_get_startup_msg(orte_buffer_t *cmd,
				      orte_jobid_t jobid)
{
    orte_gpr_cmd_flag_t command;
    int rc;

    command = ORTE_GPR_GET_STARTUP_MSG_CMD;

    if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, &command, 1, ORTE_GPR_PACK_CMD))) {
		return rc;
    }

    if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, &jobid, 1, ORTE_JOBID))) {
		return rc;
    }

    return ORTE_SUCCESS;
}

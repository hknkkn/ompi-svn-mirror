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

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>

#include "include/orte_constants.h"

#include "dps/dps.h"

#include "mca/oob/oob_types.h"
#include "mca/rml/rml.h"

#include "gpr_proxy.h"

int orte_gpr_proxy_dump(int output_id)
{
    orte_gpr_cmd_flag_t command;
    orte_buffer_t *cmd;
    orte_buffer_t *answer;
    int rc;
    size_t n;
    
    if (orte_gpr_proxy_globals.compound_cmd_mode) {
	    return orte_gpr_base_pack_dump(orte_gpr_proxy_globals.compound_cmd);
    }

    cmd = OBJ_NEW(orte_buffer_t);
    if (NULL == cmd) { /* got a problem */
	    return ORTE_ERR_OUT_OF_RESOURCE;
    }

    if (ORTE_SUCCESS != (rc = orte_gpr_base_pack_dump(cmd))) {
        OBJ_RELEASE(cmd);
	    return rc;
    }

    if (0 > orte_rml.send_buffer(orte_process_info.gpr_replica, cmd, MCA_OOB_TAG_GPR, 0)) {
	    return ORTE_ERR_COMM_FAILURE;
    }

    answer = OBJ_NEW(orte_buffer_t);
    if (NULL == answer) {
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    if (0 > orte_rml.recv_buffer(orte_process_info.gpr_replica, answer, MCA_OOB_TAG_GPR)) {
	    return ORTE_ERR_COMM_FAILURE;
    }

    n = 1;
    if (ORTE_SUCCESS != (rc = orte_dps.unpack(answer, &command, &n, ORTE_GPR_CMD))) {
        OBJ_RELEASE(answer);
        return rc;
    }
    
	if (ORTE_GPR_DUMP_CMD != command) {
        OBJ_RELEASE(answer);
	    return ORTE_ERR_COMM_FAILURE;
    }

    return orte_gpr_base_print_dump(answer, output_id);
}

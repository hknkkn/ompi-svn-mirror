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
 * The Open MPI General Purpose Registry - Replica component
 *
 */

/*
 * includes
 */
#include "orte_config.h"

#include "include/orte_constants.h"
#include "dps/dps_types.h"
#include "util/output.h"
#include "util/proc_info.h"

#include "mca/ns/ns_types.h"
#include "mca/oob/oob_types.h"
#include "mca/rml/rml.h"

#include "gpr_proxy.h"


int orte_gpr_proxy_notify_off(orte_gpr_notify_id_t sub_number)
{
    orte_buffer_t *cmd, *answer;
    int rc;
    
    if (orte_gpr_proxy_globals.debug) {
	    ompi_output(0, "[%d,%d,%d] gpr_proxy_notify_off entered for sub_number %X", 
                    ORTE_NAME_ARGS(orte_process_info.my_name), sub_number);
    }

    if (orte_gpr_proxy_globals.compound_cmd_mode) {
	    return orte_gpr_base_pack_notify_off(orte_gpr_proxy_globals.compound_cmd,
				            orte_process_info.my_name, sub_number);
    }

    cmd = OBJ_NEW(orte_buffer_t);
    if (NULL == cmd) {
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    if (ORTE_SUCCESS != (rc = orte_gpr_base_pack_notify_off(cmd, orte_process_info.my_name, sub_number))) {
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
        OBJ_RELEASE(answer);
        return ORTE_ERR_COMM_FAILURE;
    }
    
    rc = orte_gpr_base_unpack_notify_off(answer);
    OBJ_RELEASE(answer);
    
    return rc;
}

int orte_gpr_proxy_notify_on(orte_gpr_notify_id_t sub_number)
{
    orte_buffer_t *cmd, *answer;
    int rc;

    if (orte_gpr_proxy_globals.debug) {
        ompi_output(0, "[%d,%d,%d] gpr_proxy_notify_on entered for sub_number %X", 
                    ORTE_NAME_ARGS(orte_process_info.my_name), sub_number);
    }

    if (orte_gpr_proxy_globals.compound_cmd_mode) {
        return orte_gpr_base_pack_notify_on(orte_gpr_proxy_globals.compound_cmd,
                          orte_process_info.my_name, sub_number);
    }

    cmd = OBJ_NEW(orte_buffer_t);
    if (NULL == cmd) {
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    if (ORTE_SUCCESS != (rc = orte_gpr_base_pack_notify_on(cmd, orte_process_info.my_name, sub_number))) {
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
        OBJ_RELEASE(answer);
        return ORTE_ERR_COMM_FAILURE;
    }
    
    rc = orte_gpr_base_unpack_notify_on(answer);
    OBJ_RELEASE(answer);
    
    return rc;
}


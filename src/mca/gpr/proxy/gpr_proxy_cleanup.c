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
#include "dps/dps_types.h"

#include "mca/ns/ns_types.h"
#include "mca/oob/oob_types.h"
#include "mca/rml/rml.h"

#include "gpr_proxy.h"


int orte_gpr_proxy_cleanup_job(orte_jobid_t jobid)
{
    orte_buffer_t *cmd, *answer;
    int rc;

    if (orte_gpr_proxy_compound_cmd_mode) {
	   return orte_gpr_base_pack_cleanup_job(orte_gpr_proxy_compound_cmd, jobid);
    }

    cmd = OBJ_NEW(orte_buffer_t);
    if (NULL == cmd) { /* got a problem */
	   return ORTE_ERR_OUT_OF_RESOURCE;
    }

    if (ORTE_SUCCESS != (rc = orte_gpr_base_pack_cleanup_job(cmd, jobid))) {
	   return rc;
    }

    if (0 > orte_rml.send_buffer(orte_gpr_my_replica, cmd, MCA_OOB_TAG_GPR, 0)) {
        return ORTE_ERR_COMM_FAILURE;
    }

    answer = OBJ_NEW(orte_buffer_t);
    if (NULL == answer) {
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    if (0 > orte_rml.recv_buffer(orte_gpr_my_replica, answer, MCA_OOB_TAG_GPR)) {
        OBJ_RELEASE(answer);
        return ORTE_ERR_COMM_FAILURE;
    }
    
    rc = orte_gpr_base_unpack_cleanup_job(answer);
    OBJ_RELEASE(answer);
    
    return rc;

}


int orte_gpr_proxy_cleanup_proc(bool purge, orte_process_name_t *proc)
{
    orte_buffer_t *cmd, *answer;
    int rc;
    
    if (orte_gpr_proxy_compound_cmd_mode) {
	   return orte_gpr_base_pack_cleanup_proc(orte_gpr_proxy_compound_cmd, purge, proc);
    }

    cmd = OBJ_NEW(orte_buffer_t);
    if (NULL == cmd) { /* got a problem */
	    return ORTE_ERR_OUT_OF_RESOURCE;
    }

    if (ORTE_SUCCESS != (rc = orte_gpr_base_pack_cleanup_proc(cmd, purge, proc))) {
	    return rc;
    }

    if (0 > orte_rml.send_buffer(orte_gpr_my_replica, cmd, MCA_OOB_TAG_GPR, 0)) {
        return ORTE_ERR_COMM_FAILURE;
    }

    answer = OBJ_NEW(orte_buffer_t);
    if (NULL == answer) {
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    if (0 > orte_rml.recv_buffer(orte_gpr_my_replica, answer, MCA_OOB_TAG_GPR)) {
        OBJ_RELEASE(answer);
        return ORTE_ERR_COMM_FAILURE;
    }
    
    rc = orte_gpr_base_unpack_cleanup_proc(answer);
    OBJ_RELEASE(answer);
    
    return rc;

}

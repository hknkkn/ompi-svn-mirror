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
 * The Open MPI General Purpose Registry - proxy component
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

int orte_gpr_proxy_preallocate_segment(char *name, int num_slots)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}


int orte_gpr_proxy_get_startup_msg(orte_jobid_t jobid,
                                    orte_buffer_t **msg,
                                    size_t *cnt,
                                    orte_process_name_t **procs)
{
    orte_buffer_t *cmd, *answer;
    int rc;

    *msg = NULL;
    *cnt = 0;
    *procs = NULL;
    
    if (orte_gpr_proxy_compound_cmd_mode) {
	   return orte_gpr_base_pack_get_startup_msg(orte_gpr_proxy_compound_cmd, jobid);
    }

    cmd = OBJ_NEW(orte_buffer_t);
    if (NULL == cmd) { /* got a problem */
	    return ORTE_ERR_OUT_OF_RESOURCE;
    }

    if (ORTE_SUCCESS != (rc = orte_gpr_base_pack_get_startup_msg(cmd, jobid))) {
	    OBJ_RELEASE(cmd);
        return rc;
    }

	if (orte_gpr_proxy_debug) {
		ompi_output(0, "[%d,%d,%d] gpr_proxy: getting startup msg for job %d",
					ORTE_NAME_ARGS(*(orte_process_info.my_name)), (int)jobid);
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

    rc = orte_gpr_base_unpack_get_startup_msg(answer, msg, cnt, procs);
    OBJ_RELEASE(answer);

    return rc;
}

void orte_gpr_proxy_decode_startup_msg(int status, orte_process_name_t *peer,
                                       orte_buffer_t* msg, orte_rml_tag_t tag,
                                       void *cbdata)
{
    return;
}

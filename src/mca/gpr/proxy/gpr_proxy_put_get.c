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
#include "dps/dps_types.h"
#include "util/output.h"
#include "util/proc_info.h"

#include "mca/ns/ns_types.h"
#include "mca/oob/oob_types.h"
#include "mca/rml/rml.h"

#include "gpr_proxy.h"

int orte_gpr_proxy_put(orte_gpr_addr_mode_t mode, char *segment,
         char **tokens, int cnt, orte_gpr_keyval_t *keyvals)
{
    orte_buffer_t *cmd;
    orte_buffer_t *answer;
    int rc;

    if (orte_gpr_proxy_debug) {
	    ompi_output(0, "[%d,%d,%d] gpr_proxy_put: entered for segment %s 1st token %s",
                    ORTE_NAME_ARGS(*(orte_process_info.my_name)), segment, *tokens);
    }

    if (orte_gpr_proxy_compound_cmd_mode) {
	   return orte_gpr_base_pack_put(orte_gpr_proxy_compound_cmd,
				     mode, segment, tokens, cnt, keyvals);
    }

    cmd = OBJ_NEW(orte_buffer_t);
    if (NULL == cmd) { /* got a problem */
	    return ORTE_ERR_OUT_OF_RESOURCE;
    }

    if (ORTE_SUCCESS != (rc = orte_gpr_base_pack_put(cmd, mode, segment, tokens, cnt, keyvals))) {
	    OBJ_RELEASE(cmd);
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
	    return ORTE_ERR_COMM_FAILURE;
    }

    rc = orte_gpr_base_unpack_put(answer);
    OBJ_RELEASE(answer);

    return rc;
}

int orte_gpr_proxy_put_nb(orte_gpr_addr_mode_t addr_mode, char *segment,
                      char **tokens, int cnt, orte_gpr_keyval_t *keyvals,
                      orte_gpr_notify_cb_fn_t cbfunc, void *user_tag)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_gpr_proxy_get(orte_gpr_addr_mode_t mode,
                                char *segment, char **tokens, char **keys,
                                int *cnt, orte_gpr_keyval_t **keyvals)

{
    orte_buffer_t *cmd;
    orte_buffer_t *answer;
    int rc;

    *keyvals = NULL;
    *cnt = 0;
    
    /* need to protect against errors */
    if (NULL == segment) {
	    return ORTE_ERR_BAD_PARAM;
    }

    if (orte_gpr_proxy_compound_cmd_mode) {
	    return orte_gpr_base_pack_get(orte_gpr_proxy_compound_cmd, mode, segment, tokens, keys);
    }

    cmd = OBJ_NEW(orte_buffer_t);
    if (NULL == cmd) { /* got a problem */
	    return ORTE_ERR_OUT_OF_RESOURCE;
    }

    if (ORTE_SUCCESS != (rc = orte_gpr_base_pack_get(cmd, mode, segment, tokens, keys))) {
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
	    return ORTE_ERR_COMM_FAILURE;
    }

    rc = orte_gpr_base_unpack_get(answer, cnt, keyvals);
    OBJ_RELEASE(answer);

    return rc;
}

int orte_gpr_proxy_get_nb(orte_gpr_addr_mode_t addr_mode,
                                char *segment, char **tokens, char **keys,
                                orte_gpr_notify_cb_fn_t cbfunc, void *user_tag)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}
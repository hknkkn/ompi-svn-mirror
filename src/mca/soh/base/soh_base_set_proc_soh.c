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
 */

/*
 * includes
 */
#include "orte_config.h"

#include "include/orte_schema.h"

#include "mca/errmgr/errmgr.h"
#include "mca/gpr/gpr.h"
#include "mca/ns/ns.h"

#include "mca/soh/base/base.h"

int orte_soh_base_set_proc_soh(orte_process_name_t *proc,
                               orte_proc_state_t state,
                               int status)
{
    orte_gpr_value_t *value;
    orte_gpr_keyval_t **keyvals;
    int rc;
    orte_jobid_t jobid;

    value = OBJ_NEW(orte_gpr_value_t);
    if (NULL == value) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    if (ORTE_SUCCESS != (rc = orte_ns.get_jobid(&jobid, proc))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    
    if (ORTE_SUCCESS != (rc = orte_schema.get_job_segment_name(&(value->segment), jobid))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    
    if (ORTE_SUCCESS != (rc = orte_schema.get_proc_tokens(&(value->tokens),
                                                    &(value->num_tokens), proc))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    
    value->keyvals = (orte_gpr_keyval_t**)malloc(2 * sizeof(orte_gpr_keyval_t*));
    if (NULL == value->keyvals) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        OBJ_RELEASE(value);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }

    value->keyvals[0] = OBJ_NEW(orte_gpr_keyval_t);
    if (NULL == value->keyvals[0]) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        OBJ_RELEASE(value);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    (value->keyvals[0])->key = strdup(ORTE_PROC_STATE_KEY);
    (value->keyvals[0])->type = ORTE_PROC_STATE;
    (value->keyvals[0])->value.proc_state = state;
    
    value->keyvals[1] = OBJ_NEW(orte_gpr_keyval_t);
    if (NULL == value->keyvals[1]) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        OBJ_RELEASE(value);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    (value->keyvals[1])->key = strdup(ORTE_EXIT_CODE_KEY);
    (value->keyvals[0])->type = ORTE_EXIT_CODE;
    (value->keyvals[0])->value.exit_code = status;

    if (ORTE_SUCCESS != (rc = orte_gpr_replica_put(ORTE_GPR_XAND, 1, &value))) {
        ORTE_ERROR_LOG(rc);
    }
    
    OBJ_RELEASE(value);
    return rc;
}

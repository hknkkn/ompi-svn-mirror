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


#include "orte_config.h"
#include "include/orte_constants.h"
#include "util/output.h"
#include "mca/mca.h"
#include "mca/base/base.h"
#include "mca/pls/base/base.h"
#include "mca/ns/ns.h"
#include "mca/gpr/gpr.h"
#include "mca/soh/soh_types.h"


/**
 * Set the process pid in the job segment and indicate the state
 * as being launched.
 */

int orte_pls_base_set_proc_pid(orte_process_name_t* name, pid_t pid)
{
    orte_gpr_value_t* values[1];
    orte_gpr_value_t value;
    orte_gpr_keyval_t kv_pid = {{OBJ_CLASS(orte_gpr_keyval_t),0},ORTE_PROC_PID_KEY,ORTE_UINT32};
    orte_gpr_keyval_t kv_state = {{OBJ_CLASS(orte_gpr_keyval_t),0},ORTE_PROC_STATE_KEY,ORTE_PROC_STATE};
    orte_gpr_keyval_t* keyvals[2];
    int i, rc;

    if(ORTE_SUCCESS != (rc = orte_schema.get_job_segment_name(&value.segment, name->jobid)))
        return rc;

    if(ORTE_SUCCESS != (rc = orte_schema.get_proc_tokens(&value.tokens, &value.num_tokens, name)))
        return rc;

    kv_pid.value.ui32 = pid;
    kv_state.value.proc_state = ORTE_PROC_STATE_LAUNCHING;
    keyvals[0] = &kv_pid;
    keyvals[1] = &kv_state;
    
    value.keyvals = keyvals;
    value.cnt = 2;
    values[0] = &value;
    
    rc = orte_gpr.put(ORTE_GPR_OVERWRITE, 1, values);
    free(value.segment);
    for(i=0; i<value.num_tokens; i++)
        free(value.tokens[i]);
    free(value.tokens);
    return rc;
}


/**
 * Add a key-value to the node segment containing the process pid for
 * the daemons.
 */

int orte_pls_base_set_node_pid(char* node_name, orte_process_name_t* name, pid_t pid)
{
    orte_gpr_value_t* values[1];
    orte_gpr_value_t value;
    orte_gpr_keyval_t kv_pid = {{OBJ_CLASS(orte_gpr_keyval_t),0},ORTE_PROC_PID_KEY,ORTE_UINT32};
    orte_gpr_keyval_t* keyvals[1];
    char* jobid;
    int i, rc;

    if(ORTE_SUCCESS != (rc = orte_schema.get_node_tokens(&value.tokens, &value.num_tokens, name->cellid, node_name)))
        return rc;

    if(ORTE_SUCCESS != (rc = orte_ns.get_jobid_string(&jobid, name)))
        goto cleanup;

    asprintf(&kv_pid.key, "%s-%s", ORTE_PROC_PID_KEY, jobid);
    free(jobid);

    kv_pid.value.ui32 = pid;
    keyvals[0] = &kv_pid;
    
    value.keyvals = keyvals;
    value.cnt = 1;
    values[0] = &value;
    
    rc = orte_gpr.put(ORTE_GPR_OVERWRITE, 1, values);

cleanup:
    free(value.segment);
    free(kv_pid.key);
    for(i=0; i<value.num_tokens; i++)
        free(value.tokens[i]);
    free(value.tokens);
    return rc;
}



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
#include "include/orte_constants.h"
#include "include/orte_types.h"

#include "mca/gpr/gpr.h"
#include "mca/errmgr/errmgr.h"

#include "mca/rmgr/base/base.h"


int orte_rmgr_base_proc_stage_gate_init(orte_jobid_t job)
{
    int i, rc, num_counters=6;
    orte_gpr_value_t *values, value, trig;
    char* keys[] = {
        /* changes to this ordering need to be reflected in code below */
        ORTE_PROC_NUM_AT_STG1,
        ORTE_PROC_NUM_AT_STG2,
        ORTE_PROC_NUM_AT_STG3,
        ORTE_PROC_NUM_FINALIZED,
        ORTE_PROC_NUM_ABORTED,
        ORTE_PROC_NUM_TERMINATED
    };

    /* setup the counters */
    OBJ_CONSTRUCT(&value, orte_gpr_value_t);
    value.addr_mode = ORTE_GPR_TOKENS_XAND | ORTE_GPR_KEYS_OR;
    if (ORTE_SUCCESS != (rc = orte_schema.get_job_segment_name(&(value.segment), job))) {
        ORTE_ERROR_LOG(rc);
        OBJ_DESTRUCT(&value);
        return rc;
    }
    value.tokens = (char**)malloc(sizeof(char*));
    if (NULL == value.tokens) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        OBJ_DESTRUCT(&value);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    value.tokens[0] = strdup(ORTE_JOB_GLOBALS); /* put counters in the job's globals container */
    value.num_tokens = 1;
    value.cnt = num_counters;
    value.keyvals = (orte_gpr_keyval_t**)malloc(num_counters * sizeof(orte_gpr_keyval_t*));
    if (NULL == value.keyvals) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        OBJ_DESTRUCT(&value);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    for (i=0; i < num_counters; i++) {
        value.keyvals[i] = OBJ_NEW(orte_gpr_keyval_t);
        if (NULL == value.keyvals[i]) {
            ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
            OBJ_DESTRUCT(&value);
            return ORTE_ERR_OUT_OF_RESOURCE;
        }
        value.keyvals[i]->key = strdup(keys[i]);
        value.keyvals[i]->type = ORTE_INT32;
        value.keyvals[i]->value.i32 = 0;
    }
    values = &value;
    
    /* put the counters on the registry */
    if (ORTE_SUCCESS != (rc = orte_gpr.put(1, &values))) {
        ORTE_ERROR_LOG(rc);
        OBJ_DESTRUCT(&value);
        return rc;
    }
    
    /* for the trigger, we want the counter values returned to us
     * setup value so it can be used for that purpose. We'll add the key
     * each time we are ready to register a subscription - for now, just
     * get everything else ready.
     */
    for (i=0; i < num_counters; i++) {
        OBJ_RELEASE(value.keyvals[i]);
    }
    free(value.keyvals);
    value.keyvals = (orte_gpr_keyval_t**)malloc(sizeof(orte_gpr_keyval_t*));
    if (NULL == value.keyvals) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        OBJ_DESTRUCT(&value);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    value.keyvals[0] = OBJ_NEW(orte_gpr_keyval_t);
    if (NULL == value.keyvals[0]) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        OBJ_DESTRUCT(&value);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    value.cnt = 1;
    value.keyvals[0]->type = ORTE_NULL;
    
    /* setup the trigger information - initialize the common elements */
    OBJ_CONSTRUCT(&trig, orte_gpr_value_t);
    trig.addr_mode = ORTE_GPR_TOKENS_XAND;
    if (ORTE_SUCCESS != (rc = orte_schema.get_job_segment_name(&(trig.segment), job))) {
        ORTE_ERROR_LOG(rc);
        OBJ_DESTRUCT(&trig);
        return rc;
    }
    trig.tokens = (char**)malloc(sizeof(char*));
    if (NULL == trig.tokens) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        OBJ_DESTRUCT(&value);
        OBJ_DESTRUCT(&trig);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    trig.tokens[0] = strdup(ORTE_JOB_GLOBALS);
    trig.num_tokens = 1;
    trig.cnt = 2;
    trig.keyvals = (orte_gpr_keyval_t**)malloc(2*sizeof(orte_gpr_keyval_t*));
    if (NULL == trig.keyvals) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        OBJ_DESTRUCT(&value);
        OBJ_DESTRUCT(&trig);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    trig.keyvals[0] = OBJ_NEW(orte_gpr_keyval_t);
    if (NULL == trig.keyvals[0]) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        OBJ_DESTRUCT(&value);
        OBJ_DESTRUCT(&trig);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    trig.keyvals[1] = OBJ_NEW(orte_gpr_keyval_t);
    if (NULL == trig.keyvals[1]) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        OBJ_DESTRUCT(&value);
        OBJ_DESTRUCT(&trig);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }

    /* Now do the individual triggers.
     * First, setup the triggers for the three main stage gates - these all compare
     * their value to that in ORTE_JOB_SLOTS_KEY, and return the value of their
     * own counter.
     */
    trig.keyvals[0]->key = strdup(ORTE_JOB_SLOTS_KEY);
    if (NULL == trig.keyvals[0]->key) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        OBJ_DESTRUCT(&value);
        OBJ_DESTRUCT(&trig);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    trig.keyvals[0]->type = ORTE_NULL;
    trig.keyvals[1]->type = ORTE_NULL;
    
    /* do the three stage gate subscriptions */
    for (i=0; i < 3; i++) {
        value.keyvals[0]->key = strdup(keys[0]);
        if (NULL == value.keyvals[0]->key) {
            ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
            OBJ_DESTRUCT(&value);
            OBJ_DESTRUCT(&trig);
            return ORTE_ERR_OUT_OF_RESOURCE;
        }
        trig.keyvals[1]->key = strdup(keys[0]);
        if (NULL == trig.keyvals[1]->key) {
            ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
            OBJ_DESTRUCT(&value);
            OBJ_DESTRUCT(&trig);
            return ORTE_ERR_OUT_OF_RESOURCE;
        }
    
        rc = orte_gpr.subscribe(
             ORTE_GPR_TRIG_ALL_CMP,
             &value,
             &trig,
             &rc,
             orte_rmgr_base_proc_stage_gate_mgr,
             NULL);
    
         if(ORTE_SUCCESS != rc) {
             ORTE_ERROR_LOG(rc);
             OBJ_DESTRUCT(&value);
             OBJ_DESTRUCT(&trig);
             return rc;
         }
         free(value.keyvals[0]->key);
         free(trig.keyvals[1]->key);
    }
    
    /* Next, setup the trigger that watches the NUM_ABORTED counter to see if
     * any process abnormally terminates - if so, then call the stage_gate_mgr
     * so it can in turn call the error manager
     */
    value.keyvals[0]->key = strdup(ORTE_PROC_NUM_ABORTED);
    if (NULL == value.keyvals[0]) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        OBJ_DESTRUCT(&value);
        OBJ_DESTRUCT(&trig);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    OBJ_RELEASE(trig.keyvals[0]);
    OBJ_RELEASE(trig.keyvals[1]);
    free(trig.keyvals);
    trig.cnt = 1;
    trig.keyvals = (orte_gpr_keyval_t**)malloc(sizeof(orte_gpr_keyval_t**));
    if (NULL == trig.keyvals) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        OBJ_DESTRUCT(&value);
        OBJ_DESTRUCT(&trig);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    trig.keyvals[0] = OBJ_NEW(orte_gpr_keyval_t);
    if (NULL == trig.keyvals[0]) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        OBJ_DESTRUCT(&value);
        OBJ_DESTRUCT(&trig);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    trig.keyvals[0]->key = strdup(ORTE_PROC_NUM_ABORTED);
    if (NULL == trig.keyvals[0]->key) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        OBJ_DESTRUCT(&value);
        OBJ_DESTRUCT(&trig);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    trig.keyvals[0]->type = ORTE_INT32;
    trig.keyvals[0]->value.i32 = 1;  /* trigger on the first process that aborts */
    
    rc = orte_gpr.subscribe(
         ORTE_GPR_TRIG_ALL_AT,
         &value,
         &trig,
         &rc,
         orte_rmgr_base_proc_stage_gate_mgr,
         NULL);

     if (ORTE_SUCCESS != rc) {
         ORTE_ERROR_LOG(rc);
         OBJ_DESTRUCT(&value);
         OBJ_DESTRUCT(&trig);
         return rc;
     }

    OBJ_DESTRUCT(&value);
    OBJ_DESTRUCT(&trig);

    return ORTE_SUCCESS;
}


void orte_rmgr_base_proc_stage_gate_mgr(orte_gpr_notify_message_t *notify_msg,
                                        void *user_tag)
{
    OBJ_RELEASE(notify_msg);
}

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
#include "ompi_config.h"
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "include/constants.h"
#include "include/orte_names.h"
#include "mca/rmgr/base/base.h"
#include "mca/rds/base/base.h"
#include "mca/ras/base/base.h"
#include "mca/rmaps/base/base.h"
#include "mca/pls/base/base.h"
#include "mca/gpr/gpr.h"
#include "mca/ns/ns.h"
#include "mca/errmgr/errmgr.h"


/*
 *  Create the job segment and initialize the application context.
 */

int orte_rmgr_base_put_app_context(
    orte_jobid_t jobid,
    orte_app_context_t** app_context,
    size_t num_context)
{
    char *jobid_string;
    orte_gpr_value_t* value;
    size_t i;
    int rc;

    value = OBJ_NEW(orte_gpr_value_t);
    if (NULL == value) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    if(ORTE_SUCCESS != (rc = orte_ns.convert_jobid_to_string(&jobid_string, jobid))) {
        OBJ_RELEASE(value);
        return rc;
    }

    /* put context info on the job segment of the registry */
    asprintf(&(value->segment), "%s-%s", ORTE_JOB_SEGMENT, jobid_string);
    
    value->num_tokens = 1;
    value->tokens[0] = strdup("global");

    value->cnt = num_context;
    value->keyvals = (orte_gpr_keyval_t**)malloc(num_context * sizeof(orte_gpr_keyval_t*));
    if(NULL == value->keyvals) {
        OBJ_RELEASE(value);
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return OMPI_ERR_OUT_OF_RESOURCE;
    }
    memset(value->keyvals, 0, num_context * sizeof(orte_gpr_keyval_t*));

    for(i=0; i<num_context; i++) {
        value->keyvals[i] = OBJ_NEW(orte_gpr_keyval_t);
        if (NULL == value->keyvals[i]) {
            rc = ORTE_ERR_OUT_OF_RESOURCE;
            goto cleanup;
        }
        (value->keyvals[i])->key = strdup(ORTE_APP_CONTEXT_KEY);
        (value->keyvals[i])->type = ORTE_APP_CONTEXT;
        (value->keyvals[i])->value.app_context = app_context[i];
        app_context[i]->idx = i;
    }
            
    rc = orte_gpr.put(
        ORTE_GPR_OVERWRITE,
        1,
        &value);
 
cleanup:
    OBJ_RELEASE(value);
    free(jobid_string);
    return rc;
}


/*
 * Comparison function for sorting context by index.
 */

static int orte_rmgr_base_cmp_app_context(
    orte_app_context_t* app1,
    orte_app_context_t* app2)
{
    if(app1->idx < app2->idx)
        return -1;
    if(app1->idx > app2->idx)
        return 1;
    return 0;
}


/*
 *  Retreive the application context
 */

int orte_rmgr_base_get_app_context(
    orte_jobid_t jobid,
    orte_app_context_t*** app_context,
    size_t* num_context)
{
    char *jobid_string;
    char *segment;
    char *tokens[2];
    char *keys[2];
    orte_gpr_value_t** values = NULL;
    int i, num_values = 0, index = 0;
    int rc;

    if(ORTE_SUCCESS != (rc = orte_ns.convert_jobid_to_string(&jobid_string, jobid)))
        return rc;

    /* create the job segment on the registry */
    asprintf(&segment, "%s-%s", ORTE_JOB_SEGMENT, jobid_string);
    tokens[0] = "global";
    tokens[1] = NULL;

    keys[0] = ORTE_APP_CONTEXT_KEY;
    keys[1] = NULL;

    rc = orte_gpr.get(
        ORTE_GPR_OR,
        segment,
        tokens,
        keys,
        &num_values,
        &values
        );
    if(rc != ORTE_SUCCESS)
        goto cleanup;

    *num_context = 0;
    for(i=0; i<num_values; i++) {
        *num_context += values[i]->cnt;
    }

    *app_context = (orte_app_context_t**)malloc(*num_context * sizeof(orte_app_context_t*));
    for(i=0; i<num_values; i++) {
        orte_gpr_value_t* value = values[i];
        orte_gpr_keyval_t** keyvals = value->keyvals;
        int k;
        for(k=0; k < value->cnt; k++) {
            orte_gpr_keyval_t* keyval = keyvals[k];
            (*app_context)[index++] = keyval->value.app_context;
            keyval->value.app_context = NULL;
        }
    }
    qsort(app_context, *num_context, sizeof(orte_app_context_t*), 
        (int (*)(const void*,const void*))orte_rmgr_base_cmp_app_context);

cleanup:
    for(i=0; i<num_values; i++) {
        OBJ_RELEASE(values[i]);
    }
    if(NULL != values) 
        free(values);
    free(segment);
    free(jobid_string);
    return rc;
}


int orte_rmgr_base_get_proc_slots(orte_jobid_t jobid, size_t* proc_slots)
{
    return ORTE_ERROR;
}


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
 * Convenience functions for accessing the General Purpose Registry
 *
 */

/*
 * includes
 */
#include "orte_config.h"

#include "include/orte_constants.h"

#include "mca/ns/ns.h"
#include "mca/errmgr/errmgr.h"

#include "include/orte_schema.h"

int orte_schema_get_proc_tokens(char ***tokens, int32_t* num_tokens, orte_process_name_t *proc);
int orte_schema_get_node_tokens(char ***tokens, int32_t* num_tokens, orte_cellid_t cellid, char *nodename);
int orte_schema_get_cell_tokens(char ***tokens, int32_t* num_tokens, orte_cellid_t cellid);
int orte_schema_get_job_segment_name(char **name, orte_jobid_t jobid);

/*
 * globals
 */
orte_schema_t orte_schema = {
    orte_schema_get_proc_tokens,
    orte_schema_get_node_tokens,
    orte_schema_get_cell_tokens,
    orte_schema_get_job_segment_name
};

int orte_schema_open(void)
{
    /* just here to ensure library gets loaded for dynamic setup */
    return ORTE_SUCCESS;
}

int orte_schema_get_proc_tokens(char ***proc_tokens, int32_t* num_tokens, orte_process_name_t *proc)
{
    int rc;
    char** tokens;
    
    tokens = (char**)malloc(3 * sizeof(char*));
    if (NULL == tokens) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    if (ORTE_SUCCESS != (rc = orte_ns.get_proc_name_string(&tokens[0], proc))) {
        ORTE_ERROR_LOG(rc);
        goto CLEANUP;
    }
    if (ORTE_SUCCESS != (rc = orte_ns.get_vpid_string(&tokens[1], proc))) {
        ORTE_ERROR_LOG(rc);
        goto CLEANUP;
    }
    tokens[2] = NULL;
    *proc_tokens = tokens;
    if(num_tokens != NULL)
        *num_tokens = 2;
    return ORTE_SUCCESS;
    
CLEANUP:
    if (NULL != tokens) {
        if (NULL != tokens[0])
            free(tokens[0]);
        if (NULL != tokens[1])
            free(tokens[1]);
        free(tokens);
    }
    return rc;
}

int orte_schema_get_node_tokens(char ***tokens, int32_t* num_tokens, orte_cellid_t cellid, char *nodename)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_schema_get_cell_tokens(char ***tokens, int32_t* num_tokens, orte_cellid_t cellid)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_schema_get_job_segment_name(char **name, orte_jobid_t jobid)
{
    char *jobidstring;
    int rc;
    
    if (ORTE_SUCCESS != (rc = orte_ns.convert_jobid_to_string(&jobidstring, jobid))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    
    if (0 > asprintf(name, "%s-%s", ORTE_JOB_SEGMENT, jobidstring)) {
        free(jobidstring);
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }

    free(jobidstring);
    return ORTE_SUCCESS;
}
   

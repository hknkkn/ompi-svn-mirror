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
#include "mca/rds/base/base.h"
#include "mca/ras/base/base.h"
#include "mca/rmaps/base/base.h"
#include "mca/pls/base/base.h"
#include "mca/gpr/gpr.h"
#include "mca/ns/ns.h"
#include "rmgr_urm.h"


static int orte_rmgr_urm_init(
    orte_app_context_t* app_context,
    size_t num_context,
    orte_jobid_t* jobid);

static int orte_rmgr_urm_spawn(
    orte_app_context_t* app_context,
    size_t num_context,
    orte_jobid_t* jobid);


static int orte_rmgr_urm_finalize(void)
{
    return OMPI_SUCCESS;
}


orte_rmgr_base_module_t orte_rmgr_urm_module = {
    orte_rmgr_urm_init,
    orte_rds_base_query,
    orte_ras_base_allocate,
    orte_ras_base_deallocate,
    orte_rmaps_base_map,
    orte_pls_base_launch,
    orte_rmgr_urm_spawn,
    orte_rmgr_urm_finalize
};


/*
 *  Create the job segment and initialize the application context.
 */

static int orte_rmgr_urm_init(
    orte_app_context_t* app_context,
    size_t num_context,
    orte_jobid_t* jobid_return)
{
    orte_jobid_t jobid;
    char *jobid_string;
    char *segment;
    char *tokens[2];
    orte_gpr_keyval_t** keyvals;
    size_t i;
    int rc;

    /* allocate a jobid  */
    if(ORTE_SUCCESS != (rc = orte_ns.create_jobid(&jobid)))
        return rc;

    if(ORTE_SUCCESS != (rc = orte_ns.convert_jobid_to_string(&jobid_string, jobid)))
        return rc;

    /* create the job segment on the registry */
    asprintf(&segment, "%s-%s", ORTE_JOB_SEGMENT, jobid_string);
    tokens[0] = "global";
    tokens[1] = NULL;

    keyvals = (orte_gpr_keyval_t**)malloc(num_context * sizeof(orte_gpr_keyval_t*));
    if(NULL == keyvals) 
        return OMPI_ERR_OUT_OF_RESOURCE;
    memset(keyvals, 0, num_context * sizeof(orte_gpr_keyval_t*));

    for(i=0; i<num_context; i++) {
        orte_gpr_keyval_t* keyval = OBJ_NEW(orte_gpr_keyval_t);
        if(NULL == keyval) {
            rc = ORTE_ERR_OUT_OF_RESOURCE;
            goto cleanup;
        }
        keyval->key = strdup(ORTE_APP_CONTEXT_KEY);
        keyval->type = ORTE_APP_CONTEXT;
        keyval->value.app_context = app_context + i;
    }
            
    rc = orte_gpr.put(
        ORTE_GPR_OVERWRITE,
        segment,
        tokens,
        num_context,
        keyvals);
 
cleanup:
    for(i=0; i<num_context; i++) {
        keyvals[i]->value.app_context = NULL;
        OBJ_RELEASE(keyvals[i]);
    }
    free(keyvals);
    free(segment);
    free(jobid_string);
    return rc;
}


/*
 *  Shortcut for the multiple steps involved in spawning a new job.
 */

static int orte_rmgr_urm_spawn(
    orte_app_context_t* app_context,
    size_t num_context,
    orte_jobid_t* jobid)
{
    int rc;
    if(ORTE_SUCCESS != (rc = orte_rmgr_urm_init(app_context,num_context,jobid)))
        return rc;
    if(ORTE_SUCCESS != (rc = orte_ras_base_allocate(*jobid)))
        return rc;
    if(ORTE_SUCCESS != (rc = orte_rmaps_base_map(*jobid)))
        return rc;
    if(ORTE_SUCCESS != (rc = orte_pls_base_launch(*jobid)))
        return rc;
    return ORTE_SUCCESS;
}




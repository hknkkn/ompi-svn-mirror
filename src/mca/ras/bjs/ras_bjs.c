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
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "include/orte_constants.h"
#include "ras_bjs.h"


static int orte_ras_bjs_allocate(orte_jobid_t jobid)
{
    orte_app_context_t** app_context;
    size_t i, num_context;
    int rc = ORTE_SUCCESS;
    size_t num_procs = 0;
    char* nodes;

    /* query for node list in the environment */
    if(NULL == (nodes = getenv("NODELIST")))
        return ORTE_NOT_FOUND;

    /* get the application context for this job */
    if(ORTE_SUCCESS != (rc = orte_rmgr_get_app_context(jobid,&app_context,&num_context)))
        return rc;

    /* determine total number of procs */
    for(i=0; i<num_context; i++) {
        num_procs += app_context[i]->num_procs;
    }

    /* parse the node list */
    


    /* release application context */
cleanup:
    for(i=0; i<num_context; i++)
        OBJ_RELEASE(app_context[i]);
    if(NULL != app_context)
        free(app_context);
    return rc;
}

static int orte_ras_bjs_deallocate(orte_jobid_t jobid)
{
    return ORTE_SUCCESS;
}


static int orte_ras_bjs_finalize(void)
{
    return ORTE_SUCCESS;
}


orte_ras_base_module_t orte_ras_bjs_module = {
    orte_ras_bjs_allocate,
    orte_ras_bjs_deallocate,
    orte_ras_bjs_finalize
};


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
#include "mca/rds/base/base.h"
#include "mca/ras/base/base.h"
#include "mca/rmaps/base/base.h"
#include "mca/pls/base/base.h"
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


static int orte_rmgr_urm_init(
    orte_app_context_t* app_context,
    size_t num_context,
    orte_jobid_t* jobid_return)
{
    orte_jobid_t jobid;
    int rc;

    if(ORTE_SUCCESS != (rc = orte_ns.create_jobid(&jobid)))
        return rc;

    /* create the job segment on the registry */

    /* allocate a jobid  */
    return ORTE_SUCCESS;
}


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




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
#include "mca/rmgr/base/base.h"
#include "mca/pls/base/base.h"
#include "mca/gpr/gpr.h"
#include "mca/ns/ns.h"
#include "rmgr_urm.h"


static int orte_rmgr_urm_create(
    orte_app_context_t** app_context,
    size_t num_context,
    orte_jobid_t* jobid);

static int orte_rmgr_urm_map(
    orte_jobid_t jobid);

static int orte_rmgr_urm_launch(
    orte_jobid_t jobid);

static int orte_rmgr_urm_terminate_job(
    orte_jobid_t jobid);

static int orte_rmgr_urm_terminate_proc(
    const orte_process_name_t* proc_name);

static int orte_rmgr_urm_spawn(
    orte_app_context_t** app_context,
    size_t num_context,
    orte_jobid_t* jobid,
    orte_rmgr_cb_fn_t* cbfn);

orte_rmgr_base_module_t orte_rmgr_urm_module = {
    orte_rds_base_query,
    orte_rmgr_urm_create,
    orte_ras_base_allocate,
    orte_ras_base_deallocate,
    orte_rmgr_urm_map,
    orte_rmgr_urm_launch,
    orte_rmgr_urm_terminate_job,
    orte_rmgr_urm_terminate_proc,
    orte_rmgr_urm_spawn,
    NULL, /* finalize */
};


/*
 *  Create the job segment and initialize the application context.
 */

static int orte_rmgr_urm_create(
    orte_app_context_t** app_context,
    size_t num_context,
    orte_jobid_t* jobid)
{
    int rc;

    /* allocate a jobid  */
    if (ORTE_SUCCESS != (rc = orte_ns.create_jobid(jobid))) {
        return rc;
    }

    /* create and initialize job segment */
    if (ORTE_SUCCESS != 
        (rc = orte_rmgr_base_put_app_context(*jobid, app_context, 
                                             num_context))) {
        return rc;
    }

    return ORTE_SUCCESS;
}


static int orte_rmgr_urm_map(orte_jobid_t jobid)
{
    return mca_rmgr_urm_component.urm_rmaps->map(jobid);
}


static int orte_rmgr_urm_launch(orte_jobid_t jobid)
{
    return mca_rmgr_urm_component.urm_pls->launch(jobid);
}

static int orte_rmgr_urm_terminate_job(orte_jobid_t jobid)
{
    return mca_rmgr_urm_component.urm_pls->terminate_job(jobid);
}

static int orte_rmgr_urm_terminate_proc(const orte_process_name_t* proc_name)
{
    return mca_rmgr_urm_component.urm_pls->terminate_proc(proc_name);
}



/*
 *  Shortcut for the multiple steps involved in spawning a new job.
 */

static int orte_rmgr_urm_spawn(
    orte_app_context_t** app_context,
    size_t num_context,
    orte_jobid_t* jobid,
    orte_rmgr_cb_fn_t* cbfn)
{
    int rc;

    if (ORTE_SUCCESS != 
        (rc = orte_rmgr_urm_create(app_context,num_context,jobid))) {
        return rc;
    }
    if (ORTE_SUCCESS != (rc = orte_ras_base_allocate(*jobid))) {
        return rc;
    }
    if (ORTE_SUCCESS != (rc = orte_rmgr_urm_map(*jobid))) {
        return rc;
    }
    if (ORTE_SUCCESS != (rc = orte_rmgr_urm_launch(*jobid))) {
        return rc;
    }
    return ORTE_SUCCESS;
}




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
 * Shutdown a job and cleanup the registry
 *
 */

/*
 * includes
 */

#include "orte_config.h"

#include "mca/ns/ns_types.h"
#include "mca/gpr/gpr.h"

#include "runtime/runtime.h"

int orte_job_shutdown(orte_jobid_t jobid)
{
    orte_gpr.triggers_inactive(jobid);
    orte_gpr.cleanup_job(jobid);
    return ORTE_SUCCESS;
}

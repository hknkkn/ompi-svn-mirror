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

/** @file **/

#include "orte_config.h"

#include <sys/types.h>
#include <unistd.h>

#include "include/constants.h"
#include "event/event.h"
#include "util/output.h"
#include "threads/mutex.h"
#include "mca/mca.h"
#include "mca/base/base.h"
#include "mca/base/mca_base_param.h"
#include "mca/rml/base/base.h"
#include "mca/errmgr/base/base.h"
#include "mca/ns/base/base.h"
#include "mca/gpr/base/base.h"
#include "mca/rmgr/base/base.h"
#include "util/proc_info.h"

#include "runtime/runtime.h"
#include "runtime/runtime_internal.h"
#include "runtime/orte_wait.h"


/**
 * Cleanup and restart a selected set of services.
 */

int orte_restart(orte_process_name_t *name)
{
    int rc;
    bool user_threads = true;
    bool have_threads = false;

    orte_process_name_t* copy;
    if (ORTE_SUCCESS != (rc = orte_ns.copy_process_name(&copy, name))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }

    /*
     * Close selected components.
     */
    if (ORTE_SUCCESS != (rc = orte_gpr_base_close())) {
        ORTE_ERROR_LOG(rc);
	return rc;
    }
    if (ORTE_SUCCESS != (rc = orte_rml_base_close())) {
        ORTE_ERROR_LOG(rc);
	return rc;
    }
    if (OMPI_SUCCESS != (rc = orte_ns_base_close())) {
        ORTE_ERROR_LOG(rc);
	return rc;
    }
    if (ORTE_SUCCESS != (rc = orte_wait_finalize())) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }

    /*
     * setup new global state
     */
    orte_process_info.seed = false;
    orte_process_info.my_name = copy;

    /*
     * Re-start components.
     */
    if (OMPI_SUCCESS != (rc = orte_wait_init())) {
            ORTE_ERROR_LOG(rc);
            return rc;
    }
    if (OMPI_SUCCESS != (rc = orte_ns_base_open())) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    if (OMPI_SUCCESS != (rc = orte_rml_base_open())) {
        ORTE_ERROR_LOG(rc);
            return rc;
    }
    if (ORTE_SUCCESS != (rc = orte_gpr_base_open())) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }

    if (ORTE_SUCCESS != (rc = orte_ns_base_select(&user_threads, &have_threads))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
 
    if (ORTE_SUCCESS != (rc = orte_gpr_base_select(&user_threads, &have_threads))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
 
    if (ORTE_SUCCESS != (rc = orte_rml_base_select(&user_threads, &have_threads))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }

    return ORTE_SUCCESS;
}


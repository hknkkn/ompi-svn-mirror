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
#include "include/orte_schema.h"

#include "runtime/runtime.h"
#include "util/output.h"
#include "util/proc_info.h"
#include "mca/ns/ns.h"

#include "mca/rmgr/rmgr.h"

#include "mca/errmgr/base/base.h"


void orte_errmgr_base_log(int error_code, char *filename, int line)
{
    if (NULL == orte_process_info.my_name) {
        ompi_output(0, "[NO-NAME] ORTE_ERROR_LOG: %s in file %s at line %d",
                                ORTE_ERROR_NAME(error_code), filename, line);
    } else {
        ompi_output(0, "[%d,%d,%d] ORTE_ERROR_LOG: %s in file %s at line %d",
                        ORTE_NAME_ARGS(orte_process_info.my_name),
                        ORTE_ERROR_NAME(error_code), filename, line);
    }
    /* orte_errmgr_base_error_detected(error_code); */
}

void orte_errmgr_base_proc_aborted(orte_process_name_t *proc)
{
    orte_finalize();
    exit(1);
}

void orte_errmgr_base_incomplete_start(orte_jobid_t job)
{
    orte_rmgr.terminate_job(job);
}

void orte_errmgr_base_error_detected(int error_code)
{
    orte_finalize();
    exit(1);
}

int orte_errmgr_base_register_job(orte_jobid_t job)
{
    /* register subscription for process_status values
     * changing to abnormal termination codes
     */
     
     return ORTE_SUCCESS;
}

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

#include "util/output.h"
#include "util/proc_info.h"
#include "mca/ns/ns.h"

#include "mca/rmgr/rmgr.h"

#include "mca/errmgr/base/base.h"


void orte_errmgr_base_log(char *msg, char *filename, int line)
{
    if (NULL == orte_process_info.my_name) {
        ompi_output(0, "[NO-NAME] ORTE_ERROR_LOG: %s in file %s at line %d",
                                msg, filename, line);
    } else {
        ompi_output(0, "[%d,%d,%d] ORTE_ERROR_LOG: %s in file %s at line %d",
                ORTE_NAME_ARGS(orte_process_info.my_name), msg, filename, line);
    }
}

void orte_errmgr_base_proc_aborted(orte_process_name_t *proc)
{
    int rc;
    orte_jobid_t job;
    
    if (ORTE_SUCCESS != (rc = orte_ns.get_jobid(&job, proc))) {
        ORTE_ERROR_LOG(rc);
        return;
    }
    
    orte_rmgr.terminate_job(job);
}

void orte_errmgr_base_incomplete_job(orte_jobid_t job)
{
    orte_rmgr.terminate_job(job);
}

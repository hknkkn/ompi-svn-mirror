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
 * 
 *
 */

/*
 * includes
 */

#include "ompi_config.h"

#include "util/output.h"

#include "mca/rml/rml.h"
#include "mca/ns/ns_types.h"
#include "mca/gpr/gpr.h"

#include "runtime/runtime.h"


int ompi_rte_job_startup(orte_jobid_t jobid)
{
    orte_buffer_t* startup_msg;
    ompi_rte_process_status_t *proc_status;
    orte_process_name_t* procs;
    size_t i, num_procs;
    int rc;

    if (ompi_rte_debug_flag) {
        	ompi_output(0, "[%d,%d,%d] entered rte_job_startup for job %d",
        		    ORTE_NAME_ARGS(*orte_process_info.my_name), (int)jobid);
    }

    rc = orte_gpr.get_startup_msg(jobid, &startup_msg, &num_procs, &procs);
    if(rc != OMPI_SUCCESS) {
        return rc;
    }
    orte_gpr.triggers_active(jobid);

    /* check to ensure there are recipients on list - don't send if not */
    if (num_procs > 0) {
	    orte_rml.xcast(orte_process_info.my_name, procs, num_procs, startup_msg, NULL);
    }

    if (ompi_rte_debug_flag) {
         ompi_output(0, "[%d,%d,%d] rte_job_startup: completed xcast of startup message",
                 ORTE_NAME_ARGS(*orte_process_info.my_name));
    }

        /* for each recipient, set process status to "running" */

	for(i=0; i<num_procs; i++) {
		proc_status = ompi_rte_get_process_status(procs+i);
		proc_status->status_key = OMPI_PROC_RUNNING;
		proc_status->exit_code = 0;
		ompi_rte_set_process_status(proc_status, procs+i);
		free(proc_status);
	}
    free(procs);

    /* return number of processes started = number of recipients */
    return num_procs;

}


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

#include "orte_config.h"

#include "include/orte_schema.h"
#include "util/output.h"

#include "mca/rml/rml.h"
#include "mca/ns/ns_types.h"
#include "mca/gpr/gpr.h"
#include "mca/soh/soh_types.h"
#include "mca/errmgr/errmgr.h"

#include "runtime/runtime.h"


int orte_job_startup(orte_jobid_t jobid)
{
    orte_buffer_t* startup_msg;
    orte_process_name_t* procs;
    orte_gpr_value_t **values;
    orte_gpr_keyval_t *kptr=NULL, **kvals;
    char *procname, *segment;
    size_t i, num_procs;
    int rc;

    if (orte_debug_flag) {
        	ompi_output(0, "[%d,%d,%d] entered rte_job_startup for job %d",
        		    ORTE_NAME_ARGS(orte_process_info.my_name), (int)jobid);
    }

    if (ORTE_SUCCESS != (rc = orte_gpr.get_startup_msg(jobid, &startup_msg, &num_procs, &procs))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    orte_gpr.triggers_active(jobid);

    /* check to ensure there are recipients on list - don't send if not */
    if (num_procs > 0) {
	    orte_rml.xcast(orte_process_info.my_name, procs, num_procs, startup_msg, NULL);
    }

    if (orte_debug_flag) {
         ompi_output(0, "[%d,%d,%d] rte_job_startup: completed xcast of startup message",
                 ORTE_NAME_ARGS(orte_process_info.my_name));
    }

    /* for each recipient, set process status to "running" */
    values = (orte_gpr_value_t**)malloc(num_procs * sizeof(orte_gpr_value_t*));
    if (NULL == values) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    if (0 > asprintf(&segment, "%s-%s", ORTE_JOB_SEGMENT, jobid)) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        free(values);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }

/*
	for(i=0; i<num_procs; i++) {
        values[i] = OBJ_NEW(orte_gpr_value_t);
        if (NULL == values[i]) {
            ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
            rc = ORTE_ERR_OUT_OF_RESOURCE;
        }
        values[i]->segment = strdup(segment);
        values[i]->cnt = 1;
        if (ORTE_SUCCESS != (rc = orte_schema.get_proc_tokens(values[i]->tokens, procs[i]))) {
            ORTE_ERROR_LOG(rc);
            goto CLEANUP;
        }
		proc_status = orte_soh.get_proc_status(procs+i);
		proc_status->status_key = OMPI_PROC_RUNNING;
		proc_status->exit_code = 0;
		ompi_rte_set_process_status(proc_status, procs+i);
		free(proc_status);
	}
    free(procs);
*/
    /* return number of processes started = number of recipients */
    return num_procs;

}


/* -*- C -*-
 * 
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
 *
 */
/** @file 
 */

#include "orte_config.h"

#include "dps/dps.h"
#include "mca/errmgr/errmgr.h"

#include "util/output.h"
#include "util/proc_info.h"

#include "mca/rml/rml_types.h"

#include "gpr_replica_api.h"

int orte_gpr_replica_get_startup_msg(orte_jobid_t jobid,
                                    orte_buffer_t **msg,
                                    size_t *cnt,
                                    orte_process_name_t **procs)
{
    orte_buffer_t *cmd, *answer;
    int rc;
    
    if (orte_gpr_replica_globals.debug) {
      ompi_output(0, "[%d,%d,%d] entered get_startup_msg",
            ORTE_NAME_ARGS(orte_process_info.my_name));
    }

    *cnt = 0;
    if (NULL != *procs) {
        return ORTE_ERR_BAD_PARAM;
    }
    
    if (NULL != *msg) {
        OBJ_RELEASE(*msg);
    }
    
    answer = OBJ_NEW(orte_buffer_t);
    if (NULL == answer) {
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    OMPI_THREAD_LOCK(&orte_gpr_replica_globals.mutex);

    if (ORTE_SUCCESS != (rc = orte_gpr_replica_get_startup_msg_fn(jobid, answer))) {
        ORTE_ERROR_LOG(rc);
        OBJ_RELEASE(answer);
        OMPI_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);
        return rc;
    }

    /* need to parse the buffer to obtain the message */
    rc = orte_gpr_base_unpack_get_startup_msg(answer, msg, cnt, procs);
    if (ORTE_SUCCESS != rc) {
        ORTE_ERROR_LOG(rc);
    }
    OBJ_RELEASE(answer);
    
    OMPI_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);

    return rc;
}

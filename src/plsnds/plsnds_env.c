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
 */

#include "orte_config.h"

#include "include/orte_constants.h"

#include "util/proc_info.h"

#include "mca/base/mca_base_param.h"
#include "mca/ns/ns.h"

#include "plsnds_fns.h"

int orte_plsnds_env(void)
{
    int rc;
    
    int param_cellid;
    int param_jobid;
    int param_vpid_start;
    int param_num_procs;
    int param_procid;

    int cellid;
    int jobid;
    int vpid_start;
    int num_procs;
    int procid;

    param_cellid = mca_base_param_register_int("plsnds", "name", "cellid", "CELLID", -1);
    mca_base_param_lookup_int(param_cellid, &cellid);
    if (cellid < 0) return ORTE_ERR_NOT_FOUND;

    param_jobid = mca_base_param_register_int("plsnds", "name", "jobid", "JOBID", -1);
    mca_base_param_lookup_int(param_jobid, &jobid);
    if (jobid < 0) return ORTE_ERR_NOT_FOUND;

    param_procid = mca_base_param_register_int("plsnds", "name", "procid", "PROCID", -1);
    mca_base_param_lookup_int(param_procid, &procid);
    if (procid < 0) return ORTE_ERR_NOT_FOUND;

    param_vpid_start = mca_base_param_register_int("plsnds", "name", "vpid_start", "VPID_START", -1);
    mca_base_param_lookup_int(param_vpid_start, &vpid_start);
    if (vpid_start < 0) return ORTE_ERR_NOT_FOUND;

    param_num_procs = mca_base_param_register_int("plsnds", "name", "num_procs", "NUM_PROCS", -1);
    mca_base_param_lookup_int(param_num_procs, &num_procs);
    if (num_procs < 0) return ORTE_ERR_NOT_FOUND;

    if (ORTE_SUCCESS != (rc = orte_ns.create_process_name(
                            &(orte_process_info.my_name),
                            cellid,
                            jobid,
                            procid))) {
        return rc;
    }
    
    orte_process_info.vpid_start = vpid_start;
    orte_process_info.num_procs = num_procs;
    
    return ORTE_SUCCESS;
}

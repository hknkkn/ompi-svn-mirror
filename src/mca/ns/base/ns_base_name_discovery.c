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
#include "include/orte_names.h"

#include "util/sys_info.h"
#include "mca/base/mca_base_param.h"

#include "mca/ns/base/base.h"

/* storage for the results of the registrations and values returned */
/* internal function */
static int orte_ns_env_name(void);


int orte_ns_base_set_my_name(void)
{
    int rc, launchid;
    char *launcher;
    orte_jobid_t jobid;
    orte_vpid_t vpid;
    
    /* check to see if name has already been set - if so, leave it alone */
    if (NULL != orte_process_info.my_name) {
        return ORTE_SUCCESS;
    }
    
    /* first check if we are seed or singleton that couldn't
     * join an existing universe - if so, name is mandated */
    if (orte_process_info.seed || NULL == orte_process_info.ns_replica) {
        if (ORTE_SUCCESS != (rc = orte_ns_base_create_process_name(
                                &(orte_process_info.my_name), 0, 0, 0))) {
            return rc;
        }
    }
    
    /* if not seed, then check to see if the name is simply available in
     * the environment
     */
    if (ORTE_SUCCESS == orte_ns_env_name()) { /* got it! */
        return ORTE_SUCCESS;
    }
    
    /* okay, not seed/singleton and not provided in environment
     * determine which launcher was used and if that
     * launcher provided a name for us in the environment
     */
     launchid = mca_base_param_register_string("pls", "name", "launcher", "PLS_LAUNCHER", NULL);
     mca_base_param_lookup_string(launchid, &launcher);
     if (NULL != launcher) {  /* launcher identified */
     }
     
    /* if launcher didn't provide the name, then let's try to get
     * one from the name server in the universe
     */
    if (ORTE_SUCCESS != (rc = orte_ns.create_jobid(&jobid))) {
        return rc;
    }
    if (ORTE_SUCCESS != (rc = orte_ns.reserve_range(jobid, 1, &vpid))) {
        return rc;
    }
    if (ORTE_SUCCESS != (rc = orte_ns.create_process_name(&(orte_process_info.my_name),
                                                0, jobid, vpid))) {
        return rc;
    }

    return ORTE_SUCCESS;
}

static int orte_ns_env_name(void)
{
    int rc;
    
    int param_cellid;
    int param_jobid;
    int param_vpid_start;
    int param_num_procs;
    int param_procid;

    int orte_ns_smn_cellid;
    int orte_ns_smn_jobid;
    int orte_ns_smn_vpid_start;
    int orte_ns_smn_num_procs;
    int orte_ns_smn_procid;

    param_cellid = mca_base_param_register_int("ns", "name", "cellid", NULL, -1);
    mca_base_param_lookup_int(param_cellid, &orte_ns_smn_cellid);
    if (orte_ns_smn_cellid < 0) return ORTE_ERROR;

    param_jobid = mca_base_param_register_int("ns", "name", "jobid", NULL, -1);
    mca_base_param_lookup_int(param_jobid, &orte_ns_smn_jobid);
    if (orte_ns_smn_jobid < 0) return ORTE_ERROR;

    param_procid = mca_base_param_register_int("ns", "name", "procid", NULL, -1);
    mca_base_param_lookup_int(param_procid, &orte_ns_smn_procid);
    if (orte_ns_smn_procid < 0) return ORTE_ERROR;

    param_vpid_start = mca_base_param_register_int("ns", "name", "vpid_start", NULL, 0);
    mca_base_param_lookup_int(param_vpid_start, &orte_ns_smn_vpid_start);
    if (orte_ns_smn_vpid_start < 0) return ORTE_ERROR;

    param_num_procs = mca_base_param_register_int("ns", "name", "num_procs", NULL, -1);
    mca_base_param_lookup_int(param_num_procs, &orte_ns_smn_num_procs);
    if (orte_ns_smn_num_procs < 0) return ORTE_ERROR;

    if (ORTE_SUCCESS != (rc = orte_ns_base_create_process_name(
                            &(orte_process_info.my_name),
                            orte_ns_smn_cellid,
                            orte_ns_smn_jobid,
                            orte_ns_smn_procid))) {
        return rc;
    }
    
    orte_process_info.vpid_start = orte_ns_smn_vpid_start;
    orte_process_info.num_procs = orte_ns_smn_num_procs;
    
    return ORTE_SUCCESS;
}

int orte_ns_base_get_peers(orte_process_name_t **procs, 
                           size_t *num_procs)
{
    int i;
    orte_cellid_t mycellid;
    orte_jobid_t myjobid;
    
    *procs = (orte_process_name_t*)malloc(orte_process_info.num_procs *
                                            sizeof(orte_process_name_t));
    if (NULL == *procs) {
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    if (ORTE_SUCCESS != orte_ns.get_cellid(&mycellid, orte_process_info.my_name)) {
        return ORTE_ERROR;
    }
    
    if (ORTE_SUCCESS != orte_ns.get_jobid(&myjobid, orte_process_info.my_name)) {
        return ORTE_ERROR;
    }
    
    for (i=0; i < orte_process_info.num_procs; i++) {
        (*procs)[i].cellid = mycellid;
        (*procs)[i].jobid = myjobid;
        (*procs)[i].vpid = orte_process_info.vpid_start + i;
    }

    *num_procs = (size_t)orte_process_info.num_procs;
    return ORTE_SUCCESS;
}

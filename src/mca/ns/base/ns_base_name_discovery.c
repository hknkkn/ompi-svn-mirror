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

#include "plsnds/plsnds.h"

#include "util/proc_info.h"
#include "mca/base/mca_base_param.h"

#include "mca/ns/base/base.h"

int orte_ns_base_set_my_name(void)
{
    int i, rc, launchid;
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
        return orte_ns_base_create_process_name(
                                &(orte_process_info.my_name), 0, 0, 0);
    }
    
    /* if not seed, then check to see if the name is simply available in
     * the environment
     */
    if (ORTE_SUCCESS == orte_plsnds[0].discover()) { /* got it! */
        return ORTE_SUCCESS;
    }
    
    /* okay, not seed/singleton and not provided in environment
     * determine which launcher was used and if that
     * launcher provided a name for us in the environment
     */
     launchid = mca_base_param_register_string("pls", "name", "launcher", "PLS_LAUNCHER", NULL);
     mca_base_param_lookup_string(launchid, &launcher);
     if (NULL != launcher) {  /* launcher identified */
         for (i=0; i < orte_plsnds_max; i++) {
              if (0 == strcmp(launcher, orte_plsnds[i].launcher)) {
                    return orte_plsnds[i].discover;
              }
         }
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

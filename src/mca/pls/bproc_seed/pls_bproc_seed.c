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

#include "ompi_config.h"


#include "mca/ns/base/base.h"
#include "mca/pls/base/base.h"
#include "mca/rmgr/base/base.h"
#include "mca/ras/base/base.h"

#include "pls_bproc_seed.h"


orte_pls_base_module_t orte_pls_bproc_seed_module = {
    orte_pls_bproc_seed_launch,
    orte_pls_bproc_seed_finalize
};


int orte_pls_bproc_seed_launch(orte_jobid_t jobid)
{
    /* query for the application context and allocated nodes */
    orte_app_context_t** context;
    size_t num_context;
    ompi_list_t nodes;

    
    return ORTE_SUCCESS;
}


int orte_pls_bproc_seed_finalize(void)
{
    return ORTE_SUCCESS;
}



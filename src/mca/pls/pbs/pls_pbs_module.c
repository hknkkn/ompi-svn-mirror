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
 *
 * These symbols are in a file by themselves to provide nice linker
 * semantics.  Since linkers generally pull in symbols by object
 * files, keeping these symbols as the only symbols in this file
 * prevents utility programs such as "ompi_info" from having to import
 * entire components just to query their version and parameters.
 */

#include "ompi_config.h"

#include "include/orte_constants.h"
#include "mca/pls/pls.h"
#include "pls_pbs.h"


const orte_pls_base_module_1_0_0_t orte_pls_pbs_module = {
    orte_pls_pbs_launch,
    orte_pls_pbs_finalize
};


int orte_pls_pbs_launch(orte_jobid_t jobid)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}


int orte_pls_pbs_finalize(void)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

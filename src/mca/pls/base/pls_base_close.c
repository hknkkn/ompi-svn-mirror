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

#include "ompi_config.h"

#include <stdio.h>

#include "include/constants.h"
#include "mca/mca.h"
#include "mca/base/base.h"
#include "mca/pls/base/base.h"


int orte_pls_base_close(void)
{
    ompi_list_item_t* item;

    /* Finalize all selected modules */

    while (NULL != 
           (item = ompi_list_remove_first(&orte_pls_base.pls_available))) {
        orte_pls_base_available_t* available = (orte_pls_base_available_t*)item;
        if (NULL != available->module->finalize) {
            available->module->finalize();
        }
        OBJ_RELEASE(available);
    }

    /* Close all remaining available modules (may be one if this is a
       Open RTE program, or [possibly] multiple if this is ompi_info) */

    mca_base_components_close(orte_pls_base.pls_output, 
                              &orte_pls_base.pls_components, NULL);

    OBJ_DESTRUCT(&orte_pls_base.pls_available);
    return OMPI_SUCCESS;
}


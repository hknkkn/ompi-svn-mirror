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
#include "mca/svc/svc.h"
#include "mca/svc/base/base.h"


int orte_svc_base_close(void)
{
    ompi_list_item_t *item;

    /* Finalize all the svc modules */
    while((item = ompi_list_remove_first(&orte_svc_base.svc_modules)) != NULL) {
        orte_svc_base_selected_t* selected = (orte_svc_base_selected_t*)item;
        selected->module->finalize();
        OBJ_RELEASE(selected);
    }

    /* Close all remaining available components (may be one if this is a
       OMPI RTE program, or [possibly] multiple if this is ompi_info) */

    mca_base_components_close(orte_svc_base.svc_output, 
                              &orte_svc_base.svc_components, NULL);
    OBJ_DESTRUCT(&orte_svc_base.svc_modules);
    return OMPI_SUCCESS;
}


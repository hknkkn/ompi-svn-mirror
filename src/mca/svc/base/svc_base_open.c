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


/*
 * The following file was created by configure.  It contains extern
 * statements and the definition of an array of pointers to each
 * component's public mca_base_component_t struct.
 */

#include "mca/svc/base/static-components.h"


/*
 * Global variables
 */

orte_svc_base_t orte_svc_base;

/**
 * Function for finding and opening either all MCA components, 
 * or the one that was specifically requested via a MCA parameter.
 */
int orte_svc_base_open(void)
{
    /* Open up all available components */

    if (OMPI_SUCCESS != 
        mca_base_components_open("svc", 0, mca_svc_base_static_components, 
                                 &orte_svc_base.svc_components)) {
        return OMPI_ERROR;
    }

    /* Initialize the list so that in mca_mpool_base_close(), we can
       iterate over it (even if it's empty, as in the case of ompi_info) */
    OBJ_CONSTRUCT(&orte_svc_base.svc_modules, ompi_list_t);
    return OMPI_SUCCESS;
}


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


#include "orte_config.h"
#include "include/orte_constants.h"
#include "util/output.h"
#include "mca/mca.h"
#include "mca/base/base.h"
#include "mca/pls/base/base.h"


/*
 * The following file was created by configure.  It contains extern
 * statements and the definition of an array of pointers to each
 * module's public mca_base_module_t struct.
 */

#include "mca/pls/base/static-components.h"

/*
 * Global variables
 */
orte_pls_base_t orte_pls_base;

/**
 * Function for finding and opening either all MCA modules, or the one
 * that was specifically requested via a MCA parameter.
 */
int orte_pls_base_open(void)
{
    int ret;

    /* Open up all available components */
    if (ORTE_SUCCESS != 
        (ret = mca_base_components_open("pls", 0, 
                                        mca_pls_base_static_components,
                                        &orte_pls_base.pls_components))) {
        return ret;
    }
    OBJ_CONSTRUCT(&orte_pls_base.pls_available, ompi_list_item_t);

    /* setup output for debug messages */
    if (!ompi_output_init) {  /* can't open output */
        return ORTE_ERR_NOT_AVAILABLE;
    }
    orte_pls_base.pls_output = ompi_output_open(NULL);
    return ORTE_SUCCESS;
}


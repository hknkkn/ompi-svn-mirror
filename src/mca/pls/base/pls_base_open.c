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
#include "include/constants.h"
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
int mca_pls_base_output = 0;
ompi_list_t mca_pls_base_components_available;
mca_pls_base_module_t ompi_pls = {
    NULL
};  /* holds selected module's function pointers */

/**
 * Function for finding and opening either all MCA modules, or the one
 * that was specifically requested via a MCA parameter.
 */
int mca_pls_base_open(void)
{
    int ret;

    /* Open up all available components */
    if (OMPI_SUCCESS != 
        (ret = mca_base_components_open("pls", 0, mca_pls_base_static_components, 
                                      &mca_pls_base_components_available))) {
        return ret;
    }

    /* All done */
    return OMPI_SUCCESS;
}

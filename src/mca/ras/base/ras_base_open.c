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

#include "mca/mca.h"
#include "mca/base/base.h"
#include "mca/base/mca_base_param.h"
#include "util/output.h"
#include "mca/ras/base/base.h"


/*
 * The following file was created by configure.  It contains extern
 * statements and the definition of an array of pointers to each
 * component's public mca_base_component_t struct.
 */

#include "mca/ras/base/static-components.h"

/*
 * Global variables
 */
orte_ras_base_t orte_ras_base;


/**
 * Function for finding and opening either all MCA components, or the one
 * that was specifically requested via a MCA parameter.
 */
int orte_ras_base_open(void)
{
    int param, value;

    /* Debugging / verbose output */

    orte_ras_base.ras_output = ompi_output_open(NULL);
    param = mca_base_param_register_int("ras", "base", "verbose", NULL, 0);
    mca_base_param_lookup_int(param, &value);
    if (value != 0) {
        orte_ras_base.ras_output = ompi_output_open(NULL);
    } else {
        orte_ras_base.ras_output = -1;
    }

    /* Open up all available components */

    if (ORTE_SUCCESS != 
        mca_base_components_open("ras", 0, mca_ras_base_static_components, 
                                 &orte_ras_base.ras_components)) {
        return ORTE_ERROR;
    }
    OBJ_CONSTRUCT(&orte_ras_base.ras_selected, ompi_list_t);

    /* All done */

    return ORTE_SUCCESS;
}


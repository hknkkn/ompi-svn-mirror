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

#include "mca/orte_ras/base/base.h"


/*
 * The following file was created by configure.  It contains extern
 * statements and the definition of an array of pointers to each
 * component's public mca_base_component_t struct.
 */

#include "mca/ras/base/static-components.h"

/*
 * globals
 */

/*
 * Global variables
 */
int mca_orte_ras_base_output = -1;
mca_orte_ras_base_module_t orte_ras = {
    mca_orte_ras_base_allocate_not_available,
    mca_orte_ras_base_deallocate_not_available
};
bool mca_orte_ras_base_selected = false;
ompi_list_t mca_orte_ras_base_components_available;
mca_orte_ras_base_component_t mca_orte_ras_base_selected_component;



/**
 * Function for finding and opening either all MCA components, or the one
 * that was specifically requested via a MCA parameter.
 */
int mca_orte_ras_base_open(void)
{
  /* Open up all available components */

  if (ORTE_SUCCESS != 
      mca_base_components_open("ras", 0, mca_orte_ras_base_static_components, 
                               &mca_orte_ras_base_components_available)) {
    return ORTE_ERROR;
  }

  /* setup output for debug messages */
  if (!ompi_output_init) {  /* can't open output */
      return ORTE_ERR_NOT_AVAILABLE;
  }

  mca_orte_ras_base_output = ompi_output_open(NULL);

  /* All done */

  return ORTE_SUCCESS;
}

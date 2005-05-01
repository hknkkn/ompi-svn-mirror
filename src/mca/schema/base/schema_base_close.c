/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University.
 *                         All rights reserved.
 * Copyright (c) 2004-2005 The Trustees of the University of Tennessee.
 *                         All rights reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

#include "orte_config.h"
#include "include/orte_constants.h"

#include "mca/schema/base/base.h"


int orte_schema_base_close(void)
{
    /* If we have a selected component and module, then finalize it */

    if (orte_schema_base_selected) {
        orte_schema_base_selected_component.schema_finalize();
    }

    /* Close all remaining available components (may be one if this is a
       OMPI RTE program, or [possibly] multiple if this is ompi_info) */

    mca_base_components_close(orte_schema_base_output, 
                         &orte_schema_base_components_available, NULL);

    orte_schema_initialized = false;
    
  /* All done */

  return ORTE_SUCCESS;
}

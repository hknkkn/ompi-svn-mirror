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
#include "util/output.h"
#include "mca/base/base.h"
#include "mca/base/mca_base_param.h"
#include "mca/rml/base/base.h"
#include "rml_oob.h"

/**
  * component open/close/init function
  */
static int orte_rml_oob_open(void)
{
    return OMPI_SUCCESS;
}


static orte_rml_module_t* 
orte_rml_oob_init(int* priority, bool *allow_multi_user_threads, bool *have_hidden_threads)
{
    *priority = 1;
    *allow_multi_user_threads = true;
    *have_hidden_threads = false;
    return &orte_rml_oob_module;
}

static int orte_rml_oob_close(void)
{
    return OMPI_SUCCESS;
}


/**
 * component definition
 */

orte_rml_component_t orte_rml_oob_component = {
      /* First, the mca_base_component_t struct containing meta
         information about the component itself */

      {
        /* Indicate that we are a rml v1.0.0 component (which also
           implies a specific MCA version) */

        ORTE_RML_BASE_VERSION_1_0_0,

        "rml", /* MCA component name */
        1,  /* MCA component major version */
        0,  /* MCA component minor version */
        0,  /* MCA component release version */
        orte_rml_oob_open,  /* component open  */
        orte_rml_oob_close  /* component close */
      },

      /* Next the MCA v1.0.0 component meta data */
      {
        /* Whether the component is checkpointable or not */
        false
      },
      orte_rml_oob_init
};


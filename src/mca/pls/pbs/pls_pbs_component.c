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
#include "mca/pls/pbs/pls-pbs-version.h"
#include "mca/base/mca_base_param.h"


/*
 * Public string showing the pls ompi_pbs component version number
 */
const char *mca_pls_pbs_component_version_string =
  "Open MPI pbs pls MCA component version " MCA_pls_pbs_VERSION;


/*
 * Local variable
 */
int param_priority = -1;


/*
 * Local function
 */
static int pbs_open(void);


/*
 * Instantiate the public struct with all of our public information
 * and pointers to our public functions in it
 */

const orte_pls_base_component_1_0_0_t mca_pls_pbs_component = {

    /* First, the mca_component_t struct containing meta information
       about the component itself */

    {
        /* Indicate that we are a pls v1.0.0 component (which also
           implies a specific MCA version) */

        ORTE_PLS_BASE_VERSION_1_0_0,

        /* Component name and version */

        "pbs",
        MCA_pls_pbs_MAJOR_VERSION,
        MCA_pls_pbs_MINOR_VERSION,
        MCA_pls_pbs_RELEASE_VERSION,

        /* Component open and close functions */

        pbs_open,
        NULL
    },

    /* Next the MCA v1.0.0 component meta data */

    {
        /* Whether the component is checkpointable or not */

        true
    },

    /* Initialization / querying functions */

    orte_pls_pbs_init
};


static int pbs_open(void)
{
    param_priority = 
        mca_base_param_register_int("pls", "pbs", "priority", NULL, 50);

    return ORTE_SUCCESS;
}


const struct orte_pls_base_module_1_0_0_t *
orte_pls_pbs_init(bool *allow_multi_user_threads,
                  bool *have_hidden_threads, int *priority)
{
    /* Are we running under a PBS job? */

    if (NULL != getenv("PBS_ENVIRONMENT") &&
        NULL != getenv("PBS_JOBID")) {
        *allow_multi_user_threads = false;
        *have_hidden_threads = false;
        mca_base_param_lookup_int(param_priority, priority);

        return &orte_pls_pbs_module;
    }

    /* Sadly, no */

    return NULL;
}

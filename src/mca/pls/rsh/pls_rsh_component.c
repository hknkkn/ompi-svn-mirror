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
#include "util/argv.h"
#include "util/path.h"
#include "mca/pls/pls.h"
#include "pls_rsh.h"
#include "mca/pls/rsh/pls-rsh-version.h"
#include "mca/base/mca_base_param.h"


/*
 * Public string showing the pls ompi_rsh component version number
 */
const char *mca_pls_rsh_component_version_string =
  "Open MPI rsh pls MCA component version " MCA_pls_rsh_VERSION;


/*
 * Global variable
 */
char **orte_pls_rsh_agent = NULL;


/*
 * Local variables
 */
static int param_priority = -1;
static int param_agent = -1;


/*
 * Local function
 */
static int rsh_open(void);


/*
 * Instantiate the public struct with all of our public information
 * and pointers to our public functions in it
 */

const orte_pls_base_component_1_0_0_t mca_pls_rsh_component = {

    /* First, the mca_component_t struct containing meta information
       about the component itself */

    {
        /* Indicate that we are a pls v1.0.0 component (which also
           implies a specific MCA version) */

        ORTE_PLS_BASE_VERSION_1_0_0,

        /* Component name and version */

        "rsh",
        MCA_pls_rsh_MAJOR_VERSION,
        MCA_pls_rsh_MINOR_VERSION,
        MCA_pls_rsh_RELEASE_VERSION,

        /* Component open and close functions */

        rsh_open,
        NULL
    },

    /* Next the MCA v1.0.0 component meta data */

    {
        /* Whether the component is checkpointable or not */

        true
    },

    /* Initialization / querying functions */

    orte_pls_rsh_init
};


static int rsh_open(void)
{
    /* Use a low priority, but allow other components to be lower */
    
    param_priority = 
        mca_base_param_register_int("pls", "rsh", "priority", NULL, 10);
    param_agent = 
        mca_base_param_register_string("pls", "rsh", "agent", NULL, "ssh");

    return ORTE_SUCCESS;
}


const struct orte_pls_base_module_1_0_0_t *
orte_pls_rsh_init(bool *allow_multi_user_threads,
                  bool *have_hidden_threads, int *priority)
{
    char *agent;
    extern char **environ;

    /* Check to see if we can find the agent in our $PATH */

    mca_base_param_lookup_string(param_agent, &agent);
    if (NULL == agent) {
        return NULL;
    }
    orte_pls_rsh_agent = ompi_argv_split(agent, ' ');
    free(agent);

    agent = ompi_path_findv(orte_pls_rsh_agent[0], 0, environ, NULL);

    /* If we didn't find the agent in the path, then don't use this
       component */

    if (NULL == agent) {
        return NULL;
    }
    free(agent);

    *allow_multi_user_threads = false;
    *have_hidden_threads = false;
    mca_base_param_lookup_int(param_priority, priority);

    return &orte_pls_rsh_module;
}

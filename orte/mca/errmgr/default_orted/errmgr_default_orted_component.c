/*
 * Copyright (c) 2010      Cisco Systems, Inc. All rights reserved.
 *
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

#include "orte_config.h"
#include "opal/util/output.h"

#include "orte/mca/errmgr/errmgr.h"
#include "orte/mca/errmgr/base/base.h"
#include "errmgr_default_orted.h"

/*
 * Public string for version number
 */
const char *orte_errmgr_default_orted_component_version_string = 
    "ORTE ERRMGR default_orted MCA component version " ORTE_VERSION;

/*
 * Local functionality
 */
static int errmgr_default_orted_open(void);
static int errmgr_default_orted_close(void);
static int errmgr_default_orted_component_query(mca_base_module_t **module, int *priority);

/*
 * Instantiate the public struct with all of our public information
 * and pointer to our public functions in it
 */
orte_errmgr_base_component_t mca_errmgr_default_orted_component =
{
    /* Handle the general mca_component_t struct containing 
     *  meta information about the component itdefault_orted
     */
    {
        ORTE_ERRMGR_BASE_VERSION_3_0_0,
        /* Component name and version */
        "default_orted",
        ORTE_MAJOR_VERSION,
        ORTE_MINOR_VERSION,
        ORTE_RELEASE_VERSION,
        
        /* Component open and close functions */
        errmgr_default_orted_open,
        errmgr_default_orted_close,
        errmgr_default_orted_component_query
    },
    {
        /* The component is checkpoint ready */
        MCA_BASE_METADATA_PARAM_CHECKPOINT
    }
};

static int my_priority;

static int errmgr_default_orted_open(void) 
{
    mca_base_component_t *c = &mca_errmgr_default_orted_component.base_version;

    mca_base_param_reg_int(c, "priority",
                           "Priority of the default_orted errmgr component",
                           false, false, 1000,
                           &my_priority);

    return ORTE_SUCCESS;
}

static int errmgr_default_orted_close(void)
{
    return ORTE_SUCCESS;
}

static int errmgr_default_orted_component_query(mca_base_module_t **module, int *priority)
{
    if (ORTE_PROC_IS_DAEMON) {
        /* we are the default component for daemons */
        *priority = my_priority;
        *module = (mca_base_module_t *)&orte_errmgr_default_orted_module;
        return ORTE_SUCCESS;        
    }
    
    *priority = -1;
    *module = NULL;
    return ORTE_ERROR;
}


/*
 * Copyright (c) 2010      Cisco Systems, Inc.  All rights reserved. 
 * Copyright (c) 2012      Los Alamos National Security, Inc. All rights reserved.
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

#include "orte_config.h"
#include "orte/constants.h"

#include "opal/mca/base/base.h"
#include "opal/mca/base/mca_base_param.h"

#include "orte/util/proc_info.h"

#include "orte/mca/db/db.h"
#include "orte/mca/db/base/base.h"
#include "db_dbm.h"

extern orte_db_base_module_t orte_db_dbm_module;
char *orte_db_dbm_directory;

/*
 * Instantiate the public struct with all of our public information
 * and pointers to our public functions in it
 */
orte_db_base_component_t mca_db_dbm_component = {
    {
        ORTE_DB_BASE_VERSION_1_0_0,

        /* Component name and version */
        "dbm",
        ORTE_MAJOR_VERSION,
        ORTE_MINOR_VERSION,
        ORTE_RELEASE_VERSION,

        /* Component open and close functions */
        orte_db_dbm_component_open,
        orte_db_dbm_component_close,
        orte_db_dbm_component_query
    },
    {
        /* The component is checkpoint ready */
        MCA_BASE_METADATA_PARAM_CHECKPOINT
    }
};


int
orte_db_dbm_component_open(void)
{
    return ORTE_SUCCESS;
}


int orte_db_dbm_component_query(mca_base_module_t **module, int *priority)
{
    mca_base_component_t *c = &mca_db_dbm_component.base_version;

    /* retrieve the name of the file to be used */
    mca_base_param_reg_string(c, "dir",
                              "Name of directory to be used for storing and recovering db information",
                              false, false, NULL, &orte_db_dbm_directory);
    
    if (NULL != orte_db_dbm_directory) {
        *priority = 50;
        *module = (mca_base_module_t*)&orte_db_dbm_module;
        return ORTE_SUCCESS;
    }
    
    
    *priority = 0;
    *module = NULL;
    return ORTE_ERROR;
}


int
orte_db_dbm_component_close(void)
{
    return ORTE_SUCCESS;
}


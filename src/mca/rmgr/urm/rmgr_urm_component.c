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
#include "util/proc_info.h"
#include "util/output.h"
#include "mca/rds/base/base.h"
#include "mca/ras/base/base.h"
#include "mca/rmaps/base/base.h"
#include "mca/pls/base/base.h"
#include "rmgr_urm.h"

/*
 * Local functions
 */

static int orte_rmgr_urm_open(void);
static int orte_rmgr_urm_close(void);
static orte_rmgr_base_module_t* orte_rmgr_urm_init(int *priority);


orte_rmgr_urm_component_t mca_rmgr_urm_component = {
    {
      /* First, the mca_base_component_t struct containing meta
         information about the component itself */

      {
        /* Indicate that we are a iof v1.0.0 component (which also
           implies a specific MCA version) */

        ORTE_RMGR_BASE_VERSION_1_0_0,

        "urm", /* MCA component name */
        1,  /* MCA component major version */
        0,  /* MCA component minor version */
        0,  /* MCA component release version */
        orte_rmgr_urm_open,  /* component open  */
        orte_rmgr_urm_close  /* component close */
      },

      /* Next the MCA v1.0.0 component meta data */
      {
        /* Whether the component is checkpointable or not */
        false
      },

      orte_rmgr_urm_init
    }
};


/**
  * component open/close/init function
  */
static int orte_rmgr_urm_open(void)
{
    int rc;

    /**
     * Open Resource Discovery Subsystem (RDS)
     */
    if (ORTE_SUCCESS != (rc = orte_rds_base_open())) {
        return rc;
    }

    /**
     * Open Resource Allocation Subsystem (RAS)
     */
    if (ORTE_SUCCESS != (rc = orte_ras_base_open())) {
        return rc;
    }

    /**
     * Open Resource Mapping Subsystem (RMAPS)
     */
    if (ORTE_SUCCESS != (rc = orte_rmaps_base_open())) {
        return rc;
    }

    /**
     * Open Process Launch Subsystem (PLS)
     */
    if (ORTE_SUCCESS != (rc = orte_pls_base_open())) {
        return rc;
    }

    return ORTE_SUCCESS;
}


static orte_rmgr_base_module_t *orte_rmgr_urm_init(int* priority)
{
    int rc;
    orte_rmaps_base_module_t *rmaps;
    orte_pls_base_module_t *pls;

    *priority = 1;

    /**
     * Select RDS components.
     */
    if (ORTE_SUCCESS != (rc = orte_rds_base_select())) {
        return NULL;
    }

    /**
     * Select RAS components.
     */
    if (ORTE_SUCCESS != (rc = orte_ras_base_select())) {
        return NULL;
    }

    /**
     * Select RMAPS components.
     */
    if (NULL == (rmaps = orte_rmaps_base_select(NULL))) {
        return NULL;
    }

    /**
     * Select PLS components.
     */
    if (NULL == (pls = orte_pls_base_select(NULL))) {
        return NULL;
    }

    return &orte_rmgr_urm_module;
}


/**
 *  Close all subsystems.
 */
static int orte_rmgr_urm_close(void)
{
    int rc;

    /**
     * Close Process Launch Subsystem (PLS)
     */
    if (ORTE_SUCCESS != (rc = orte_pls_base_close())) {
        return rc;
    }

    /**
     * Close Resource Mapping Subsystem (RMAPS)
     */
    if (ORTE_SUCCESS != (rc = orte_rmaps_base_close())) {
        return rc;
    }

    /**
     * Close Resource Allocation Subsystem (RAS)
     */
    if (ORTE_SUCCESS != (rc = orte_ras_base_close())) {
        return rc;
    }

    /**
     * Close Resource Discovery Subsystem (RDS)
     */
    if (ORTE_SUCCESS != (rc = orte_rds_base_close())) {
        return rc;
    }

    return ORTE_SUCCESS;
}

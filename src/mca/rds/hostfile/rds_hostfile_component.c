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
#include "mca/base/base.h"
#include "mca/base/mca_base_param.h"
#include "util/proc_info.h"
#include "util/output.h"
#include "rds_hostfile.h"

/*
 * Local functions
 */

static int orte_rds_hostfile_open(void);
static int orte_rds_hostfile_close(void);
static orte_rds_base_module_t* orte_rds_hostfile_init(
    bool *allow_multi_user_threads,
    bool *have_hidden_threads);


orte_rds_hostfile_component_t mca_rds_hostfile_component = {
    {
      /* First, the mca_base_component_t struct containing meta
         information about the component itself */

      {
        /* Indicate that we are a iof v1.0.0 component (which also
           implies a specific MCA version) */

        ORTE_RDS_BASE_VERSION_1_0_0,

        "hostfile", /* MCA component name */
        1,  /* MCA component major version */
        0,  /* MCA component minor version */
        0,  /* MCA component release version */
        orte_rds_hostfile_open,  /* component open  */
        orte_rds_hostfile_close  /* component close */
      },

      /* Next the MCA v1.0.0 component meta data */
      {
        /* Whether the component is checkpointable or not */
        false
      },

      orte_rds_hostfile_init
    }
};


/**
 *  Convience functions to lookup MCA parameter values.
 */

static  int orte_rds_hostfile_param_register_int(
    const char* param_name,
    int default_value)
{
    int id = mca_base_param_register_int("rds","hostfile",param_name,NULL,default_value);
    int param_value = default_value;
    mca_base_param_lookup_int(id,&param_value);
    return param_value;
}
                                                                                                                                                                       
static char* orte_rds_hostfile_param_register_string(
    const char* param_name,
    const char* default_value)
{
    char *param_value;
    int id = mca_base_param_register_string("rds","hostfile",param_name,NULL,default_value);
    mca_base_param_lookup_string(id, &param_value);
    return param_value;
}


/**
  * component open/close/init function
  */
static int orte_rds_hostfile_open(void)
{
    mca_rds_hostfile_component.debug = orte_rds_hostfile_param_register_int("debug",1);
    return ORTE_SUCCESS;
}


static orte_rds_base_module_t* 
orte_rds_hostfile_init(bool *allow_multi_user_threads, bool *have_hidden_threads)
{
    *allow_multi_user_threads = true;
    *have_hidden_threads = false;
    return &orte_rds_hostfile_module;
}

/**
 *  Close all subsystems.
 */

static int orte_rds_hostfile_close(void)
{
    return ORTE_SUCCESS;
}



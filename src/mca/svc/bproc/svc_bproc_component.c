/* -*- C -*-
 * 
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
 */

#include "ompi_config.h"
#include <sys/bproc.h>

#include "include/orte_constants.h"
#include "include/types.h"
#include "class/ompi_list.h"
#include "mca/mca.h"
#include "mca/base/mca_base_param.h"
#include "svc_bproc.h"

/*
 * Struct of function pointers and all that to let us be initialized
 */
orte_svc_bproc_base_component_1_0_0_t orte_svc_bproc_component = {
    {
        {
        ORTE_SVC_BASE_VERSION_1_0_0,
        "bproc", /* MCA component name */
        1,  /* MCA component major version */
        0,  /* MCA component minor version */
        0,  /* MCA component release version */
        orte_svc_bproc_component_open,  /* component open */
        orte_svc_bproc_component_close /* component close */
        },
        {
        false /* checkpoint / restart */
        },
        orte_svc_bproc_component_init,    /* component init */
    }
};


/**
 *  Convience functions to lookup MCA parameters
 */
                                                                                                                     
static char* orte_svc_bproc_param_register_string(
    const char* param_name,
    const char* default_value)
{
    char *param_value;
    int id = mca_base_param_register_string("svc","bproc",param_name,NULL,default_value);
    mca_base_param_lookup_string(id, &param_value);
    return param_value;
}
                                                                                                                     
static  int orte_svc_bproc_param_register_int(
    const char* param_name,
    int default_value)
{
    int id = mca_base_param_register_int("svc","bproc",param_name,NULL,default_value);
    int param_value = default_value;
    mca_base_param_lookup_int(id,&param_value);
    return param_value;
}


/**
 * Component open/close/init
 */

int orte_svc_bproc_component_open(void)
{
    orte_svc_bproc_component.debug = orte_svc_bproc_param_register_int("debug",1);
    return ORTE_SUCCESS;
}


int orte_svc_bproc_component_close(void)
{
    return ORTE_SUCCESS;
}


orte_svc_bproc_base_module_t*
orte_svc_bproc_component_init(
		   bool* allow_threads,
		   bool* have_thread)
{
    struct bproc_version_t vers;

    /* check to see if I'm in a daemon - if not, then we can't use this launcher */
    if (!ompi_process_info.daemon) {
        return NULL;
    }
    
    /* okay, we are in a daemon - now check to see if BProc is running here */
    ret = bproc_version(&vers);
    if (ret != 0) {
      ompi_output_verbose(5, mca_pcm_base_output, 
           "bproc: bproc_version() failed");
      return NULL;
    }
    
    /* only launch from the master node */
    if (bproc_currnode() != BPROC_NODE_MASTER) {
      ompi_output_verbose(5, mca_pcm_base_output, 
            "bproc: not on BPROC_NODE_MASTER");
      return NULL;
    }
    return &orte_svc_bproc_module;
}



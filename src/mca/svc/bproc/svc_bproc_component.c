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
#include "util/proc_info.h"
#include "util/output.h"
#include "class/ompi_list.h"
#include "mca/mca.h"
#include "mca/base/mca_base_param.h"
#include "mca/rml/rml.h"
#include "mca/svc/svc.h"
#include "svc_bproc.h"

/*
 * Struct of function pointers and all that to let us be initialized
 */
orte_svc_bproc_component_t orte_svc_bproc_component = {
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


orte_svc_base_module_t*
orte_svc_bproc_component_init(
		   bool* allow_threads,
		   bool* have_thread)
{
    struct bproc_version_t vers;
    int rc;

    /* check to see if I'm in a daemon - if not, then we can't use this launcher */
    if (!orte_process_info.daemon) {
        return NULL;
    }
    
    /* okay, we are in a daemon - now check to see if BProc is running here */
    rc = bproc_version(&vers);
    if (rc != 0) {
        ompi_output(0, "bproc: bproc_version() failed with status %d", rc);
        return NULL;
    }
    
    /* only launch from the master node */
    if (bproc_currnode() != BPROC_NODE_MASTER) {
        if(orte_svc_bproc_component.debug)
            ompi_output(0, "bproc: not on BPROC_NODE_MASTER");
        return NULL;
    }
    
    /* initialize the module */
    rc = orte_rml.recv_buffer_nb(
        ORTE_RML_NAME_ANY,
        ORTE_RML_TAG_BPROC_SVC,
        0,
        orte_svc_bproc_module_recv,
        NULL);
    if(rc != ORTE_SUCCESS) {
        ompi_output(0, "orte_svc_bproc_component_init: orte_rml.rml_receive_buffer_nb failed: %d\n", rc);
        return NULL;
    }
    return &orte_svc_bproc_module;
}



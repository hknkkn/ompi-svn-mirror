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

#include "include/orte_constants.h"
#include "include/types.h"
#include "class/ompi_list.h"
#include "util/proc_info.h"
#include "mca/mca.h"
#include "mca/base/mca_base_param.h"
#include "mca/pls/base/base.h"

#include <sys/bproc.h>
#include "pls_bproc_seed.h"

/*
 * Struct of function pointers and all that to let us be initialized
 */
orte_pls_bproc_component_t orte_pls_bproc_seed_component = {
    {
        {
        ORTE_PLS_BASE_VERSION_1_0_0,

        "bproc_seed", /* MCA component name */
        1,  /* MCA component major version */
        0,  /* MCA component minor version */
        0,  /* MCA component release version */
        orte_pls_bproc_seed_component_open,  /* component open */
        orte_pls_bproc_seed_component_close /* component close */
        },
        {
        false /* checkpoint / restart */
        },
        orte_pls_bproc_seed_init    /* component init */ 
    }
};


/**
 *  Convience functions to lookup MCA parameter values.
 */
                                                                                                          
static  int orte_pls_bproc_param_register_int(
    const char* param_name,
    int default_value)
{
    int id = mca_base_param_register_int("pls","bproc",param_name,NULL,default_value);
    int param_value = default_value;
    mca_base_param_lookup_int(id,&param_value);
    return param_value;
}
                                                                                                          

int orte_pls_bproc_seed_component_open(void)
{
    /* init globals */
    OBJ_CONSTRUCT(&orte_pls_bproc_seed_component.lock, ompi_mutex_t);
    OBJ_CONSTRUCT(&orte_pls_bproc_seed_component.condition, ompi_condition_t);
    orte_pls_bproc_seed_component.num_completed = 0;

    /* init parameters */
    orte_pls_bproc_seed_component.debug = orte_pls_bproc_param_register_int("debug", 1);
    orte_pls_bproc_seed_component.image_frag_size = orte_pls_bproc_param_register_int("image_frag_size", 256*1024);
    orte_pls_bproc_seed_component.name_fd = orte_pls_bproc_param_register_int("name_fd", 3);
    orte_pls_bproc_seed_component.priority = orte_pls_bproc_param_register_int("priority", 20);

    return ORTE_SUCCESS;
}


int orte_pls_bproc_seed_component_close(void)
{
    return ORTE_SUCCESS;
}


orte_pls_base_module_t* orte_pls_bproc_seed_init(
    int *priority)
{
    int ret;
    struct bproc_version_t version;
 
    /* are we the seed */
    if(orte_process_info.seed == false)
        return NULL;

    /* okay, we are in a daemon - now check to see if BProc is running here */
    ret = bproc_version(&version);
    if (ret != 0) {
        return NULL;
    }
    
    /* only launch from the master node */
    if (bproc_currnode() != BPROC_NODE_MASTER) {
        return NULL;
    }

    /* post a non-blocking receive */


    *priority = orte_pls_bproc_seed_component.priority;
    return &orte_pls_bproc_seed_module;
}


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

#include "mca/mca.h"
#include "mca/base/base.h"
#include "mca/base/mca_base_param.h"
#include "util/output.h"
#include "mca/rmgr/base/base.h"


/*
 * The following file was created by configure.  It contains extern
 * statements and the definition of an array of pointers to each
 * component's public mca_base_component_t struct.
 */

#include "mca/rmgr/base/static-components.h"

/*
 * globals
 */

orte_rmgr_base_t orte_rmgr_base;
orte_rmgr_base_module_t orte_rmgr = {
    orte_rmgr_base_query_not_available,
    orte_rmgr_base_allocate_not_available,
    orte_rmgr_base_deallocate_not_available,
    orte_rmgr_base_map_not_available,
    orte_rmgr_base_launch_not_available,
    orte_rmgr_base_finalize_not_available
};

/*
 * Object class instantiations
 */
/* constructor - used to initialize state of app_info instance */
static void orte_rmgr_app_info_construct(orte_rmgr_app_info_t* ptr)
{
    ptr->application = NULL;
    ptr->num_procs = 0;
}

/* destructor - used to free any resources held by instance */
static void orte_rmgr_app_info_destructor(orte_rmgr_app_info_t* ptr)
{
    if (NULL != ptr->application) {
        free(ptr->application);
    }
}

/* define instance of ompi_class_t */
OBJ_CLASS_INSTANCE(
    orte_rmgr_app_info_t,              /* type name */
    ompi_object_t,                  /* parent "class" name */
    orte_rmgr_app_info_construct,      /* constructor */
    orte_rmgr_app_info_destructor);    /* destructor */


/* constructor - used to initialize state of app_context instance */
static void orte_rmgr_app_context_construct(orte_rmgr_app_context_t* ptr)
{
    ptr->argc = 0;
    ptr->argv = NULL;
    ptr->num_enviro = 0;
    ptr->enviro = NULL;
}

/* destructor - used to free any resources held by instance */
static void orte_rmgr_app_context_destructor(orte_rmgr_app_context_t* ptr)
{
    int i;
    
    if (NULL != ptr->argv) {
        for (i=0; i < ptr->argc; i++) {
            if (NULL != ptr->argv[i]) {
                free(ptr->argv[i]);
            }
        }
    }
    if (NULL != ptr->enviro) {
        for (i=0; i < ptr->num_enviro; i++) {
            if (NULL != ptr->enviro[i]) {
                free(ptr->enviro[i]);
            }
        }
    }
    
}

/* define instance of ompi_class_t */
OBJ_CLASS_INSTANCE(
    orte_rmgr_app_context_t,              /* type name */
    ompi_object_t,                  /* parent "class" name */
    orte_rmgr_app_context_construct,      /* constructor */
    orte_rmgr_app_context_destructor);    /* destructor */


/**
 * Function for finding and opening either all MCA components, or the one
 * that was specifically requested via a MCA parameter.
 */

int orte_rmgr_base_open(void)
{
    /* Open up all available components */
    if (ORTE_SUCCESS != 
        mca_base_components_open("orte_rmgr", 0, mca_rmgr_base_static_components, 
                               &orte_rmgr_base.rmgr_components)) {
        return ORTE_ERROR;
    }

    /* setup output for debug messages */
    if (!ompi_output_init) {  /* can't open output */
        return ORTE_ERR_NOT_AVAILABLE;
    }
    orte_rmgr_base.rmgr_output = ompi_output_open(NULL);
    return ORTE_SUCCESS;
}


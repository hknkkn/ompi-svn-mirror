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


#include "ompi_config.h"
#include "mca/mca.h"
#include "mca/base/base.h"
#include "mca/dps/base/base.h"


/*
 * The following file was created by configure.  It contains extern
 * statements and the definition of an array of pointers to each
 * module's public mca_base_module_t struct.
 */

#include "mca/dps/base/static-components.h"

/*
 * Global variables
 */
int mca_dps_base_output = 0;
ompi_list_t mca_dps_base_components_available;
mca_dps_base_module_t ompi_dps = {
};  /* holds selected module's function pointers */

/* Instantiate the classes
 * */
static void orte_buffer_construct (orte_buffer_t* buffer)
{
    buffer->base_ptr = buffer->data_ptr = buffer->from_ptr = NULL;
    buffer->size = buffer->len = buffer-> space = buffer-> toend = 0;
    buffer->cnt = 0;
}

static void orte_buffer_destruct (orte_buffer_t* buffer)
{
    /* paranoid check */
    if (buffer->base_ptr) free (buffer->base_ptr);

    /* just clean up */
    buffer->base_ptr = buffer->data_ptr = buffer->from_ptr = NULL;
    buffer->size = buffer->len = buffer-> space = buffer-> toend = 0;
}

OBJ_CLASS_INSTANCE(orte_buffer_t, ompi_object_t, 
     orte_buffer_construct, orte_buffer_destruct);

/**
 * Function for finding and opening either all MCA modules, or the one
 * that was specifically requested via a MCA parameter.
 */
int mca_dps_base_open(void)
{
    int ret;

    /* Open up all available components */
    if (OMPI_SUCCESS != 
        (ret = mca_base_components_open("dps", 0, mca_dps_base_static_components, 
                                      &mca_dps_base_components_available))) {
        return ret;
    }

    /* All done */
    return OMPI_SUCCESS;
}

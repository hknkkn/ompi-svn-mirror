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
#include "mca/rdas/base/base.h"
#include "mca/rdas/base/base_internal.h"


/*
 * The following file was created by configure.  It contains extern
 * statements and the definition of an array of pointers to each
 * module's public mca_base_module_t struct.
 */

#include "mca/rdas/base/static-components.h"

/*
 * Global variables
 */
int mca_rdas_base_output = 0;
ompi_list_t mca_rdas_base_components_available;
mca_rdas_base_module_t ompi_rdas = {
   mca_rdas_base_discover_resources,
   mca_rdas_base_request_resource_allocation,
   mca_rdas_base_set_resource_allocation,
   mca_rdas_base_process_allocation_options,
   mca_rdas_base_get_resource_allocation,
   mca_rdas_base_deallocate_resources,
   mca_rdas_base_pack_resource,
   mca_rdas_base_unpack_resource,
   mca_rdas_base_pack_allocated_resource,
   mca_rdas_base_unpack_allocated_resource,
   mca_rdas_base_finalize
};  /* holds selected module's function pointers */


/* mutex for the host file parsing code in the base */
ompi_mutex_t mca_rdas_base_parse_mutex;

/**
 * Function for finding and opening either all MCA modules, or the one
 * that was specifically requested via a MCA parameter.
 */
int mca_rdas_base_open(void)
{
    int ret;

    /* initialize the internal mutex */
    OBJ_CONSTRUCT(&mca_rdas_base_parse_mutex, ompi_mutex_t);

    /* Open up all available components */
    if (OMPI_SUCCESS != 
        (ret = mca_base_components_open("rdas", 0, mca_rdas_base_static_components, 
                                      &mca_rdas_base_components_available))) {
        return ret;
    }

    /* All done */
    return OMPI_SUCCESS;
}


/*
 * Object maintenance code
 */

/** constructor for \c mca_rdas_base_hostfile_data_t */
static
void
rdas_base_int_hostfile_data_construct(ompi_object_t *obj)
{
    mca_rdas_base_hostfile_data_t *data = (mca_rdas_base_hostfile_data_t*) obj;
    data->hostlist = OBJ_NEW(ompi_list_t);
}


/** destructor for \c mca_rdas_base_hostfile_data_t */
static
void
rdas_base_int_hostfile_data_destruct(ompi_object_t *obj)
{
    mca_rdas_base_hostfile_data_t *data = (mca_rdas_base_hostfile_data_t*) obj;
    mca_rdas_base_deallocate(data->hostlist);
}


/** constructor for \c mca_rdas_base_hostfile_node_t */
static
void
rdas_base_int_hostfile_node_construct(ompi_object_t *obj)
{
    mca_rdas_base_hostfile_node_t *node = (mca_rdas_base_hostfile_node_t*) obj;
    (node->hostname)[0] = '\0';
    node->count = 0;
    node->given_count = 0;
    node->info = OBJ_NEW(ompi_list_t);
}


/** destructor for \c mca_rdas_base_hostfile_node_t */
static
void
rdas_base_int_hostfile_node_destruct(ompi_object_t *obj)
{
    mca_rdas_base_hostfile_node_t *node = (mca_rdas_base_hostfile_node_t*) obj;
    ompi_list_item_t *item;

    if (NULL == node->info) return;

    while (NULL != (item = ompi_list_remove_first(node->info))) {
        OBJ_RELEASE(item);
    }

    OBJ_RELEASE(node->info);
}


/** create instance information for \c mca_rdas_base_hostfile_data_t */
OBJ_CLASS_INSTANCE(mca_rdas_base_hostfile_data_t, 
                   ompi_rte_node_allocation_data_t,
                   rdas_base_int_hostfile_data_construct,
                   rdas_base_int_hostfile_data_destruct);
/** create instance information for \c mca_rdas_base_hostfile_node_t */
OBJ_CLASS_INSTANCE(mca_rdas_base_hostfile_node_t,
                   ompi_list_item_t,
                   rdas_base_int_hostfile_node_construct,
                   rdas_base_int_hostfile_node_destruct);

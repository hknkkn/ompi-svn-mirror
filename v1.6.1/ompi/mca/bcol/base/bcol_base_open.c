/*
 * Copyright (c) 2009-2012 Oak Ridge National Laboratory.  All rights reserved.
 * Copyright (c) 2009-2012 Mellanox Technologies.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */



#include "ompi_config.h"
#include <stdio.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif  /* HAVE_UNIST_H */
#include "opal/mca/mca.h"
#include "opal/mca/base/base.h"
#include "opal/util/argv.h"

#include "orte/util/show_help.h"

#include "ompi/mca/bcol/bcol.h"
#include "ompi/mca/bcol/base/base.h"
#include "ompi/include/ompi/constants.h"
#include "ompi/mca/mpool/mpool.h"
#include "ompi/mca/coll/ml/coll_ml.h" /*frag and full message descriptors defined here*/
#include "opal/class/opal_list.h"
/*
 * The following file was created by configure.  It contains extern
 * statements and the definition of an array of pointers to each
 * component's public mca_base_component_t struct.
 */

#include "ompi/mca/bcol/base/static-components.h"

/*
**  * Global variables
**   */
int mca_bcol_base_output = 0;
opal_list_t mca_bcol_base_components_opened;

OMPI_DECLSPEC opal_list_t mca_bcol_base_components_in_use;
OMPI_DECLSPEC char *ompi_bcol_bcols_string;
OMPI_DECLSPEC int bcol_mpool_compatibility[BCOL_SIZE][BCOL_SIZE];
OMPI_DECLSPEC int bcol_mpool_index[BCOL_SIZE][BCOL_SIZE];

static void bcol_base_module_constructor(mca_bcol_base_module_t *module)
{
    int fnc;

    module->bcol_component = NULL;
    module->network_context = NULL;
    module->context_index = -1;
    module->supported_mode = 0;
    module->init_module = NULL;
    module->sbgp_partner_module = NULL;
    module->squence_number_offset = 0;
    module->n_poll_loops = 0;

    for (fnc = 0; fnc < BCOL_NUM_OF_FUNCTIONS; fnc++) {
        module->bcol_function_table[fnc] = NULL;
        module->small_message_thresholds[fnc] = BCOL_THRESHOLD_UNLIMITED;
    }

    module->set_small_msg_thresholds = NULL;

    module->header_size = 0;
    module->bcol_memory_init = NULL;

    module->next_inorder = NULL;

    mca_bcol_base_fn_table_construct(module);
}

static void bcol_base_module_destructor(mca_bcol_base_module_t *module)
{
    int fnc;

    module->bcol_component = NULL;

    module->context_index = -1;
    module->init_module = NULL;
    module->sbgp_partner_module = NULL;
    module->squence_number_offset = 0;
    module->n_poll_loops = 0;

    for (fnc = 0; fnc < BCOL_NUM_OF_FUNCTIONS; fnc++) {
        module->bcol_function_table[fnc] = NULL;
    }

    module->bcol_memory_init = NULL;
}

OBJ_CLASS_INSTANCE(mca_bcol_base_module_t,
        opal_object_t,
        bcol_base_module_constructor,
        bcol_base_module_destructor);

static void bcol_base_network_context_constructor(bcol_base_network_context_t *nc)
{
    nc->context_id = -1;
    nc->context_data = NULL;
}

static void bcol_base_network_context_destructor(bcol_base_network_context_t *nc)
{
    nc->context_id = -1;
    nc->context_data = NULL;
    nc->register_memory_fn = NULL;
    nc->deregister_memory_fn = NULL;
}

OBJ_CLASS_INSTANCE(bcol_base_network_context_t,
        opal_object_t,
        bcol_base_network_context_constructor,
        bcol_base_network_context_destructor);

/* get list of subgrouping coponents to use */
static int mca_bcol_base_set_components_to_use(opal_list_t *bcol_components_avail,
                opal_list_t *bcol_components_in_use)
{
    /* local variables */
    opal_list_item_t *b_item;
    const mca_base_component_t *b_component;

    mca_base_component_list_item_t *b_cli;
    mca_base_component_list_item_t *b_clj;

    char **bcols_requested;
    const char *b_component_name;

    size_t b_str_len;
    int i, cnt, n_bcol_types = 0;

    /* split the requst for the bcol modules */
    bcols_requested = opal_argv_split(ompi_bcol_bcols_string, ',');
    if (NULL == bcols_requested) {
        return OMPI_ERROR;
    }

    /* count arguments - set number of levels to match the input value */
    cnt = 0;
    while (bcols_requested[cnt]) {
        cnt++;
    }

    /* Initialize list */
    OBJ_CONSTRUCT(bcol_components_in_use, opal_list_t);

    /* figure out basic collective modules to use */
    /* loop over list of components requested */
    for (i = 0; i < cnt; i++) {
        /* loop over discovered components */
        for (b_item = opal_list_get_first(bcol_components_avail);
                opal_list_get_end(bcol_components_avail) != b_item;
                b_item = opal_list_get_next(b_item)) {

            b_cli = (mca_base_component_list_item_t *) b_item;
            b_component = b_cli->cli_component;

            b_component_name = b_component->mca_component_name;
            b_str_len = strlen(b_component_name);

            if ((b_str_len == strlen(bcols_requested[i])) &&
                    (0 == strncmp(b_component_name,bcols_requested[i],b_str_len))) {
                /* found selected component */
                b_clj = OBJ_NEW(mca_base_component_list_item_t);
                if (NULL == b_clj) {
                    return OPAL_ERR_OUT_OF_RESOURCE;
                }

                b_clj->cli_component = b_component;
                opal_list_append(bcol_components_in_use,
                                (opal_list_item_t *) b_clj);

                n_bcol_types++;
                break;
             } /* end check for bcol component */
         }
     }

    /* Note: Need to add error checking to make sure all requested functions
    ** were found */

    /*
    ** release resources
    ** */

    cnt = 0;
    while (bcols_requested[cnt]) {
        free(bcols_requested[cnt]);
        cnt++;
    }

    if (bcols_requested) {
        free(bcols_requested);
    }

    return OMPI_SUCCESS;
}

/**
 * Function for finding and opening either all MCA components, or the one
 * that was specifically requested via a MCA parameter.
 */
int mca_bcol_base_open(void)
{
    int value, ret;

    /*_bcol_base_components_available
     * Register some MCA parameters
     */
     /* Debugging/Verbose output */
     mca_base_param_reg_int_name("bcol",
                                 "base_verbose",
                                 "Verbosity level of BCOL framework",
                                 false, false,
                                 0, &value);

     /* get fraemwork id    */
     mca_bcol_base_output = opal_output_open(NULL);
     opal_output_set_verbosity(mca_bcol_base_output, value);

    /* Open up all available components */
    if (OMPI_SUCCESS !=
        mca_base_components_open("bcol", mca_bcol_base_output, mca_bcol_base_static_components,
                                 &mca_bcol_base_components_opened,
                                 true)) {
        return OMPI_ERROR;
    }

    /* figure out which bcol and sbgp components will actually be used */
    /* get list of sub-grouping functions to use */
    mca_base_param_reg_string_name("bcol","base_string",
            "Default set of basic collective components to use ",
            false, false, "basesmuma,basesmuma,iboffload,ptpcoll,ugni", &ompi_bcol_bcols_string);

    ret = mca_bcol_base_set_components_to_use(&mca_bcol_base_components_opened,
                                              &mca_bcol_base_components_in_use);

    /* memory registration compatibilities */
    bcol_mpool_compatibility[BCOL_SHARED_MEMORY_UMA][BCOL_SHARED_MEMORY_UMA]=1;
    bcol_mpool_compatibility[BCOL_SHARED_MEMORY_UMA][BCOL_SHARED_MEMORY_SOCKET]=1;
    bcol_mpool_compatibility[BCOL_SHARED_MEMORY_UMA][BCOL_POINT_TO_POINT]=1;
    bcol_mpool_compatibility[BCOL_SHARED_MEMORY_UMA][BCOL_IB_OFFLOAD]=1;
    bcol_mpool_compatibility[BCOL_SHARED_MEMORY_SOCKET][BCOL_SHARED_MEMORY_UMA]=1;
    bcol_mpool_compatibility[BCOL_POINT_TO_POINT]      [BCOL_SHARED_MEMORY_UMA]=1;
    bcol_mpool_compatibility[BCOL_IB_OFFLOAD]          [BCOL_SHARED_MEMORY_UMA]=1;

    return OMPI_SUCCESS;
}

/*
 * Prototype implementation of selection logic
 */
int mca_bcol_base_fn_table_construct(struct mca_bcol_base_module_t *bcol_module){

        int bcol_fn;
        /* Call all init functions */

        /* Create a function table */
        for (bcol_fn = 0; bcol_fn < BCOL_NUM_OF_FUNCTIONS; bcol_fn++){
            /* Create a list object for each bcol type list */
            OBJ_CONSTRUCT(&(bcol_module->bcol_fns_table[bcol_fn]), opal_list_t);
        }

    return OMPI_SUCCESS;
}

int mca_bcol_base_fn_table_destroy(struct mca_bcol_base_module_t *bcol_module){

    int bcol_fn;

    for (bcol_fn = 0; bcol_fn < BCOL_NUM_OF_FUNCTIONS; bcol_fn++){
        opal_list_t *tmp_list;

        tmp_list = &bcol_module->bcol_fns_table[bcol_fn];

        /* gvm FIX: Go through the list and destroy each item */
        /* Destroy the function table object for each bcol type list */
        OBJ_DESTRUCT(&(bcol_module->bcol_fns_table[bcol_fn]));
    }

    return OMPI_SUCCESS;
}

int mca_bcol_base_set_attributes(struct mca_bcol_base_module_t *bcol_module,
                mca_bcol_base_coll_fn_comm_attributes_t *arg_comm_attribs,
                mca_bcol_base_coll_fn_invoke_attributes_t *arg_inv_attribs,
                mca_bcol_base_module_collective_fn_primitives_t bcol_fn,
                mca_bcol_base_module_collective_fn_primitives_t progress_fn
                )
{
    mca_bcol_base_coll_fn_comm_attributes_t *comm_attribs = NULL;
    mca_bcol_base_coll_fn_invoke_attributes_t *inv_attribs = NULL;
    struct mca_bcol_base_coll_fn_desc_t *fn_filtered = NULL;
    int coll_type;

    comm_attribs = malloc(sizeof(mca_bcol_base_coll_fn_comm_attributes_t));
    inv_attribs = malloc(sizeof(mca_bcol_base_coll_fn_invoke_attributes_t));

    if (!((comm_attribs) && (inv_attribs))) {
        return OMPI_ERR_OUT_OF_RESOURCE;
    }

    coll_type = comm_attribs->bcoll_type = arg_comm_attribs->bcoll_type;
    comm_attribs->comm_size_min = arg_comm_attribs->comm_size_min;
    comm_attribs->comm_size_max = arg_comm_attribs->comm_size_max;
    comm_attribs->data_src = arg_comm_attribs->data_src;
    comm_attribs->waiting_semantics = arg_comm_attribs->waiting_semantics;

    inv_attribs->bcol_msg_min = arg_inv_attribs->bcol_msg_min;
    inv_attribs->bcol_msg_max = arg_inv_attribs->bcol_msg_max ;
    inv_attribs->datatype_bitmap = arg_inv_attribs->datatype_bitmap ;
    inv_attribs->op_types_bitmap = arg_inv_attribs->op_types_bitmap;

    fn_filtered = OBJ_NEW(mca_bcol_base_coll_fn_desc_t);

    fn_filtered->coll_fn = bcol_fn;
    fn_filtered->progress_fn = progress_fn;

    fn_filtered->comm_attr = comm_attribs;
    fn_filtered->inv_attr = inv_attribs;


    opal_list_append(&(bcol_module->bcol_fns_table[coll_type]),(opal_list_item_t*)fn_filtered);

    return OMPI_SUCCESS;
}

int mca_bcol_base_bcol_fns_table_init(struct mca_bcol_base_module_t *bcol_module){

    int ret, bcol_init_fn;

    for (bcol_init_fn =0; bcol_init_fn < BCOL_NUM_OF_FUNCTIONS; bcol_init_fn++) {
        if (NULL != bcol_module->bcol_function_init_table[bcol_init_fn]) {
            ret = (bcol_module->bcol_function_init_table[bcol_init_fn]) (bcol_module);
            if (OMPI_SUCCESS != ret) {
                return OMPI_ERROR;
            }
        }
    }

    return OMPI_SUCCESS;
}

OBJ_CLASS_INSTANCE(mca_bcol_base_coll_fn_desc_t,
                   opal_list_item_t,
                   NULL,
                   NULL);

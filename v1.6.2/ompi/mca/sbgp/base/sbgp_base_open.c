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
#include "orte/util/show_help.h"

#include "orte/util/show_help.h"

#include "ompi/mca/sbgp/sbgp.h"
#include "ompi/mca/sbgp/base/base.h"
#include "ompi/include/ompi/constants.h"
#include "opal/util/argv.h"

/*
 * The following file was created by configure.  It contains extern
 * statements and the definition of an array of pointers to each
 * component's public mca_base_component_t struct.
 */

#include "ompi/mca/sbgp/base/static-components.h"

/*
**  * Global variables
**   */
int mca_sbgp_base_output = 0;
opal_list_t mca_sbgp_base_components_opened;
opal_list_t mca_sbgp_base_components_in_use;
int mca_sbgp_base_components_in_use_inited=0;
OMPI_DECLSPEC char *ompi_sbgp_subgroups_string;

static void mca_sbgp_base_destruct (mca_sbgp_base_module_t *module)
{
   /* free the list of ranks */
   if(module->group_list ) {
       free(module->group_list);
       module->group_list=NULL;
   }
}

OBJ_CLASS_INSTANCE(mca_sbgp_base_module_t,
        opal_object_t,
        NULL,
        mca_sbgp_base_destruct);

OBJ_CLASS_INSTANCE(sbgp_base_component_keyval_t,
        mca_base_component_list_item_t,
        NULL,
        NULL);

#define RELEASE_LIST_OF_STRINGS(str_arr)        \
    do {                                        \
        int arr_size  = 0;                      \
        if (NULL != str_arr) {                  \
            while(NULL != str_arr[arr_size]) {  \
                free(str_arr[arr_size]);        \
                ++arr_size;                     \
            }                                   \
            free(str_arr);                      \
        }                                       \
    } while (0)

/* get list of subgrouping coponents to use */
static int ompi_sbgp_set_components_to_use(opal_list_t *sbgp_components_avail,
        opal_list_t *sbgp_components_in_use)
{
    /* local variables */
    opal_list_item_t *item;
    const mca_base_component_t *component;
    mca_base_component_list_item_t *cli;
    sbgp_base_component_keyval_t *clj;
    char **subgoups_requested = NULL, **sbgp_string = NULL;
    char *sbgp_component, *sbgp_key;
    const char *component_name;
    size_t str_len;
    int i, sbgp_size = 0,
        sbgp_string_size = 0,
        rc = OMPI_SUCCESS;

    /* split the list of requested subgroups */
    subgoups_requested = opal_argv_split(ompi_sbgp_subgroups_string, ',');
    if(NULL == subgoups_requested) {
        return OMPI_ERROR;
    }
    /* debug print */
    /*
    fprintf(stderr,"FFF ompi_sbgp_subgroups_string %s \n",ompi_sbgp_subgroups_string);
    fflush(stderr);
    */
    /* end debug */

    /* count arguments - set number of levels to match the input value */
    while(subgoups_requested[sbgp_size]){
        ++sbgp_size;

    }
    /*
    fprintf(stderr,"DDD subgroup size %d \n",sbgp_size);
    fflush(stderr);
    */

    /* Initialize list */
    OBJ_CONSTRUCT(sbgp_components_in_use, opal_list_t);

    /* loop over list of components requested */
    for(i = 0; i < sbgp_size; i++) {
        sbgp_string_size = 0;
        sbgp_component = NULL;
        sbgp_key = NULL;

        /* get key-value */
        sbgp_string = opal_argv_split(subgoups_requested[i], ':');
        while (sbgp_string[sbgp_string_size]) {
            ++sbgp_string_size;
        }

        switch (sbgp_string_size) {
            case 2:
                sbgp_key = sbgp_string[1];
            case 1:
                sbgp_component = sbgp_string[0];
                break;
            default:
                opal_output(mca_sbgp_base_output,
                        "Requested SBGP configuration is illegal %s",
                        subgoups_requested[i]);
                RELEASE_LIST_OF_STRINGS(sbgp_string);
                rc = OMPI_ERROR;
                goto error;
        }
        /* loop over discovered components */
        for (item = opal_list_get_first(sbgp_components_avail) ;
                opal_list_get_end(sbgp_components_avail) != item;
                item = opal_list_get_next(item)
                ) {

            cli = (mca_base_component_list_item_t *) item;
            component = cli->cli_component;
            component_name = component->mca_component_name;
        /* debug print */
        /*
        fprintf(stderr,"component name %s sbgp_component %s \n",component_name,sbgp_component);
        fflush(stderr);
        */
        /* end debug */

            str_len = strlen(component_name);

            /* key_value[0] has the component name, and key_value[1], if
            ** it is not NULL, has the key_value associated with this
            ** instance of the compoenent
            */

            if((str_len == strlen(sbgp_component)) &&
                    (0 == strncmp(component_name,sbgp_component,str_len))) {
                 /* found selected component */
                 clj = OBJ_NEW(sbgp_base_component_keyval_t);
                 if (NULL == clj) {
                     rc = OPAL_ERR_OUT_OF_RESOURCE;
                     RELEASE_LIST_OF_STRINGS(sbgp_string);
                     goto error;
                 }
                 /* fprintf(stderr,"sbgp selecting %s %s\n", sbgp_component, component_name); */

                 clj->component.cli_component = component;
                 if (NULL != sbgp_key) {
                     clj->key_value = strdup(sbgp_key);
                 } else {
                     clj->key_value = NULL;
                 }
                 opal_list_append(sbgp_components_in_use, (opal_list_item_t *)clj);
                 break;
             }
         }
         RELEASE_LIST_OF_STRINGS(sbgp_string);
     }

    /* Note: Need to add error checking to make sure all requested functions
    ** were found */

    /*
    ** release resources
    ** */
    /* subgoups_requested */
error:
    /* RELEASE_LIST_OF_STRINGS(subgoups_requested); */

    return rc;
}

/**
 * Function for finding and opening either all MCA components, or the one
 * that was specifically requested via a MCA parameter.
 */
int mca_sbgp_base_open(void)
{
    int value, ret = OMPI_SUCCESS;

    /*_sbgp_base_components_available
     * Register some MCA parameters
     */
     /* Debugging/Verbose output */
     mca_base_param_reg_int_name("sbgp",
                                 "base_verbose",
                                 "Verbosity level of SBGP framework",
                                 false, false,
                                 0, &value);

     /* get fraemwork id    */
     mca_sbgp_base_output = opal_output_open(NULL);
     opal_output_set_verbosity(mca_sbgp_base_output, value);

    /* Open up all available components */
    ret = mca_base_components_open("sbgp", mca_sbgp_base_output, mca_sbgp_base_static_components,
                             &mca_sbgp_base_components_opened,
                             true);
    if (OMPI_SUCCESS != ret) {
        return OMPI_ERROR;
    }

    /* get list of sub-grouping functions to use */
    mca_base_param_reg_string_name("sbgp","base_subgroups_string",
            "Default set of subgroup operations to apply ",
            false,false,"basesmsocket,basesmuma,ibnet,p2p",&ompi_sbgp_subgroups_string);

    ret = ompi_sbgp_set_components_to_use(&mca_sbgp_base_components_opened,
            &mca_sbgp_base_components_in_use);

    return ret;
}


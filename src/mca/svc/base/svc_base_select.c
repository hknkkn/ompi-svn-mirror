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

#include "runtime/runtime.h"
#include "util/output.h"
#include "mca/mca.h"
#include "mca/base/base.h"
#include "mca/svc/svc.h"
#include "mca/svc/base/base.h"


OBJ_CLASS_INSTANCE(orte_svc_base_selected_t, ompi_list_item_t, NULL, NULL);


/**
 * Function for weeding out svc modules that don't want to run.
 *
 * Call the init function on all available components to find out if they
 * want to run.  Select all components that don't fail.  Failing modules
 * will be closed and unloaded.  The selected modules will be returned
 * to the caller in a ompi_list_t.
 */
int orte_svc_base_select(bool *allow_threads, bool* have_threads)
{
    ompi_list_item_t *item;

    /* Call the init functions for all available components */
    for(item =  ompi_list_get_first(&orte_svc_base.svc_components);
        item != ompi_list_get_end(&orte_svc_base.svc_components);
        item =  ompi_list_get_next(item)) {

        mca_base_component_list_item_t* cli = (mca_base_component_list_item_t *) item;
        orte_svc_base_component_t* component = (orte_svc_base_component_t *) cli->cli_component;

        ompi_output_verbose(10, orte_svc_base.svc_output, 
                        "select: initializing %s module %s",
                        component->svc_version.mca_type_name,
                        component->svc_version.mca_component_name);
        if (NULL == component->svc_init) {
            ompi_output_verbose(10, orte_svc_base.svc_output,
                                "select: no init function; ignoring module");
        } else {
            bool allow;
            bool have;
            orte_svc_base_module_t* module = component->svc_init(&allow, &have);

            /* If the module didn't initialize, unload it */
            if (NULL == module) {
                ompi_output_verbose(10, orte_svc_base.svc_output, "select: init returned failure");
                mca_base_component_repository_release((mca_base_component_t *) component);
                ompi_output_verbose(10, orte_svc_base.svc_output,
                            "select: component %s unloaded",
                            component->svc_version.mca_component_name);
            } 

            /* Otherwise, it initialized properly.  Save it. */
            else {
                orte_svc_base_selected_t *sm = OBJ_NEW(orte_svc_base_selected_t);
                sm->component = component;
                sm->module = module;
                *allow_threads &= allow;
                *have_threads |= have;
                ompi_list_append(&orte_svc_base.svc_modules, (ompi_list_item_t*) sm);
            }
        }
    }
    return OMPI_SUCCESS;
}



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
#include "class/ompi_list.h"
#include "util/output.h"
#include "util/show_help.h"
#include "mca/mca.h"
#include "mca/base/base.h"
#include "mca/pls/base/base.h"


OBJ_CLASS_INSTANCE(orte_pls_base_available_t,
                   ompi_list_item_t, NULL, NULL);


/*
 * Function for selecting all available modules.
 */
int orte_pls_base_select(
    bool *allow_multi_user_threads,
    bool *have_hidden_threads)
{
    ompi_list_item_t *item;
    mca_base_component_list_item_t *cli;
    const orte_pls_base_component_t *component;
    const orte_pls_base_module_t *module;
    bool multi, hidden;
    int priority;

    /* Iterate through all the available components */

    for (item = ompi_list_get_first(&orte_pls_base.pls_components);
         item != ompi_list_get_end(&orte_pls_base.pls_components);
         item = ompi_list_get_next(item)) {
        cli = (mca_base_component_list_item_t *) item;
        component = (orte_pls_base_component_t *) cli->cli_component;

        /* Call the component's init function and see if it wants to be
           available */

        module = component->pls_init(&multi, &hidden, &priority);

        /* If we got a non-NULL module back, then the component wants to
           be available. */

        if (NULL != module) {
            orte_pls_base_available_t* available = 
                OBJ_NEW(orte_pls_base_available_t);
            available->module = module;
            available->component = component;
            available->allow_multi_user_threads = multi;
            available->have_hidden_threads = hidden;
            if (priority < 0) {
                priority = 0;
            } else if (priority > 100) {
                priority = 100;
            }
            available->priority = priority;
            ompi_list_append(&orte_pls_base.pls_available, &available->super);
        }
    }

    return (ompi_list_get_size(&orte_pls_base.pls_available) > 0) ? 
        ORTE_SUCCESS : ORTE_ERROR;
}


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

#include "mca/rmaps/base/base.h"


/**
 * Function for selecting one component from all those that are
 * available.
 */
int orte_rmaps_base_select(bool *allow_multi_user_threads, 
                       bool *have_hidden_threads)
{
    ompi_list_item_t *item;
    mca_base_component_list_item_t *cli;
    orte_rmaps_base_component_t *component;
    orte_rmaps_base_module_t *module;
    bool multi, hidden;
    int priority;

    /* Iterate through all the available components */

    for (item = ompi_list_get_first(&orte_rmaps_base.rmaps_components);
         item != ompi_list_get_end(&orte_rmaps_base.rmaps_components);
         item = ompi_list_get_next(item)) {
        cli = (mca_base_component_list_item_t *) item;
        component = (orte_rmaps_base_component_t *) cli->cli_component;

        /* Call the component's init function and see if it wants to be
         * selected 
         */
        module = component->rmaps_init(&multi, &hidden, &priority);

        /* If we got a non-NULL module back, then the component wants to
         * be selected.  So save its multi/hidden values and save the
         * module with the highest priority 
         */
        if (NULL != module) {
        } 
    }
    return ORTE_SUCCESS;
}


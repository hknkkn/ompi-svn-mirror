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

#include "mca/ras/base/base.h"


/**
 * Function for selecting one component from all those that are
 * available.
 */
int orte_mca_ras_base_select(bool *allow_multi_user_threads, 
                       bool *have_hidden_threads)
{
  ompi_list_item_t *item;
  mca_base_component_list_item_t *cli;
  orte_mca_ras_base_component_t *component, *best_component = NULL;
  orte_mca_ras_base_module_t *module, *best_module = NULL;
  bool multi, hidden;
  int priority, best_priority = -1;

  /* Iterate through all the available components */

  for (item = ompi_list_get_first(&orte_mca_ras_base_components_available);
       item != ompi_list_get_end(&orte_mca_ras_base_components_available);
       item = ompi_list_get_next(item)) {
    cli = (mca_base_component_list_item_t *) item;
    component = (orte_mca_ras_base_component_t *) cli->cli_component;

    /* Call the component's init function and see if it wants to be
       selected */

    module = component->ras_init(&multi, &hidden, &priority);

    /* If we got a non-NULL module back, then the component wants to
       be selected.  So save its multi/hidden values and save the
       module with the highest priority */

    if (NULL != module) {
      /* If this is the best one, save it */

      if (priority > best_priority) {

        /* If there was a previous best one, finalize */

        if (NULL != best_component) {
          best_component->ns_finalize();
        }

        /* Save the new best one */

        best_module = module;
        best_component = component;
        *allow_multi_user_threads = multi;
        *have_hidden_threads = hidden;

	/* update the best priority */
	best_priority = priority;
      } 

      /* If it's not the best one, finalize it */

      else {
        component->ras_finalize();
      }
    }
  }

  /* If we didn't find one to select, barf */

  if (NULL == best_component) {
    return ORTE_ERROR;
  }

  /* We have happiness -- save the component and module for later
     usage */

  orte_ras = *best_module;
  orte_mca_ras_base_selected_component = *best_component;
  orte_mca_ras_base_selected = true;

  /* all done */

  return ORTE_SUCCESS;
}

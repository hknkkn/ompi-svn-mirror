/*
 * $HEADER$
 */

#include "ompi_config.h"

#include "runtime/runtime.h"
#include "mca/mca.h"
#include "mca/base/base.h"
#include "mca/ptl/ptl.h"
#include "mca/ptl/base/base.h"


/**
 * Function for weeding out ptl components that don't want to run.
 *
 * Call the init function on all available components to find out if
 * they want to run.  Select all components that don't fail.  Failing
 * components will be closed and unloaded.  The selected modules will
 * be returned to the caller in a ompi_list_t.
 */
int mca_ptl_base_select(bool *allow_multi_user_threads, 
                        bool *have_hidden_threads)
{
  int i, num_ptls;
  bool user_threads, hidden_threads;
  ompi_list_item_t *item;
  mca_base_component_list_item_t *cli;
  mca_ptl_base_component_t *component;
  mca_ptl_base_module_t **modules;
  mca_ptl_base_selected_module_t *sm;

  /* Traverse the list of available modules; call their init
     functions. */

  for (item = ompi_list_get_first(&mca_ptl_base_components_available);
       ompi_list_get_end(&mca_ptl_base_components_available) != item;
       item = ompi_list_get_next(item)) {
    cli = (mca_base_component_list_item_t *) item;
    component = (mca_ptl_base_component_t *) cli->cli_component;

    ompi_output_verbose(10, mca_ptl_base_output, 
                       "select: initializing %s component %s",
                       component->ptlm_version.mca_type_name,
                       component->ptlm_version.mca_component_name);
    if (NULL == component->ptlm_init) {
      ompi_output_verbose(10, mca_ptl_base_output,
                         "select: no init function; ignoring component");
    } else {
      modules = component->ptlm_init(&num_ptls, &user_threads,
                                  &hidden_threads);

      /* If the component didn't initialize, unload it */

      if (NULL == modules) {
        ompi_output_verbose(10, mca_ptl_base_output,
                           "select: init returned failure");

        mca_base_component_repository_release((mca_base_component_t *) component);
        ompi_output_verbose(10, mca_ptl_base_output,
                            "select: module %s unloaded",
                            component->ptlm_version.mca_component_name);
      } 

      /* Otherwise, it initialized properly.  Save it. */

      else {
        *allow_multi_user_threads &= user_threads;
        *have_hidden_threads |= hidden_threads;

        ompi_output_verbose(10, mca_ptl_base_output,
                           "select: init returned success");

        for (i = 0; i < num_ptls; ++i) {
          sm = malloc(sizeof(mca_ptl_base_selected_module_t));
          if (NULL == sm) {
            return OMPI_ERR_OUT_OF_RESOURCE;
          }
          OBJ_CONSTRUCT(sm, ompi_list_item_t);
          sm->pbsm_component = component;
          sm->pbsm_module = modules[i];
          ompi_list_append(&mca_ptl_base_components_initialized,
                          (ompi_list_item_t*) sm);
        }
        free(modules);
      }
    }
  }

  /* Finished querying all components.  Check for the bozo case. */

  if (0 == ompi_list_get_size(&mca_ptl_base_components_initialized)) {
    /* JMS Replace with show_help */
    ompi_abort(1, "No ptl components available.  This shouldn't happen.");
  }

  /* All done */

  return OMPI_SUCCESS;
}

/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

#include "ompi_config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "mpi.h"
#include "ompi/communicator/communicator.h"
#include "opal/util/argv.h"
#include "opal/util/show_help.h"
#include "opal/class/opal_list.h"
#include "opal/class/opal_object.h"
#include "opal/mca/mca.h"
#include "opal/mca/base/base.h"
#include "ompi/mca/coll/coll.h"
#include "ompi/mca/coll/base/base.h"


/*
 * Local variables
 */


/*
 * Local types
 */
struct avail_coll_t {
    opal_list_item_t super;

    int ac_priority;
    mca_coll_base_module_1_1_0_t *ac_module;
};
typedef struct avail_coll_t avail_coll_t;


/*
 * Local functions
 */
static opal_list_t *check_components(opal_list_t *components, 
                                     ompi_communicator_t *comm, 
                                     char **names, int num_names);
static int check_one_component(ompi_communicator_t *comm, 
                               const mca_base_component_t *component,
                               mca_coll_base_module_1_1_0_t **module);

static int query(const mca_base_component_t *component, 
                 ompi_communicator_t *comm, int *priority,
                 mca_coll_base_module_1_1_0_t **module);

static int query_1_1_0(const mca_coll_base_component_1_1_0_t *coll_component, 
                       ompi_communicator_t *comm, int *priority,
                       mca_coll_base_module_1_1_0_t **module);

/*
 * Stuff for the OBJ interface
 */
static OBJ_CLASS_INSTANCE(avail_coll_t, opal_list_item_t, NULL, NULL);


#define COPY(module, comm, func)                                        \
    do {                                                                \
        if (NULL != module->coll_ ## func) {                            \
            if (NULL != comm->c_coll.coll_ ## func ## _module) {        \
                OBJ_RELEASE(comm->c_coll.coll_ ## func ## _module);     \
            }                                                           \
            comm->c_coll.coll_ ## func = module->coll_ ## func;         \
            comm->c_coll.coll_ ## func ## _module = module;             \
            OBJ_RETAIN(module);                                         \
        }                                                               \
    } while (0)

/*
 * This function is called at the initialization time of every
 * communicator.  It is used to select which coll component will be
 * active for a given communicator.
 *
 * This selection logic is not for the weak.
 */
int mca_coll_base_comm_select(ompi_communicator_t *comm)
{
    int ret, num_names;
    char name[MPI_MAX_OBJECT_NAME + 32];
    char *names, **name_array;
    opal_list_t *selectable;
    opal_list_item_t *item;

  /* Announce */
  snprintf(name, sizeof(name), "%s (cid %d)", comm->c_name, 
           comm->c_contextid);
  name[sizeof(name) - 1] = '\0';
  opal_output_verbose(10, mca_coll_base_output,
                      "coll:base:comm_select: new communicator: %s", 
                      name);
  
  /* Initialize all the relevant pointers, since they're used as
     sentinel values */
  memset(&comm->c_coll, 0, sizeof(mca_coll_base_comm_coll_t));
  
  /* See if a set of component was requested by the MCA parameter.
     Don't check for error. */
  names = NULL;
  mca_base_param_lookup_string(mca_coll_base_param, &names);

  if (NULL != names && 0 < strlen(names)) {
    /* mca param based */
    name_array = opal_argv_split(names, ',');
    num_names = opal_argv_count(name_array);

    opal_output_verbose(10, mca_coll_base_output, 
                       "coll:base:comm_select: Checking specific modules: %s",
                       names);
    selectable = check_components(&mca_coll_base_components_available, 
                                  comm, name_array, num_names);
    opal_argv_free(name_array);
  } else {
    /* no specific components given -- try all */
    opal_output_verbose(10, mca_coll_base_output, 
                       "coll:base:comm_select: Checking all available modules");
    selectable = check_components(&mca_coll_base_components_available, 
                                  comm, NULL, 0);
  }

  /* Upon return from the above, the modules list will contain the
     list of modules that returned (priority >= 0).  If we have no
     collective modules available, then use the basic component */
  if (NULL == selectable) {
      /* There's no modules available */
      opal_show_help("help-mca-coll-base",
                     "comm-select:none-available", true);
      return OMPI_ERROR;
  }

  /* FIX ME - Do some kind of collective operation to find a module
     that everyone has available */

  /* do the selection loop */
  for (item = opal_list_get_first(selectable) ;
       item != opal_list_get_end(selectable) ;
       item = opal_list_get_next(item)) {
      avail_coll_t *avail = (avail_coll_t*) item;

      /* initialize the module */
      ret = avail->ac_module->coll_module_enable(avail->ac_module, comm);
      if (OMPI_SUCCESS != ret) {
          mca_coll_base_comm_unselect(comm);
          continue;
      }

      /* copy over any of the pointers */
      COPY(avail->ac_module, comm, allgather); 
      COPY(avail->ac_module, comm, allgatherv); 
      COPY(avail->ac_module, comm, allreduce); 
      COPY(avail->ac_module, comm, alltoall); 
      COPY(avail->ac_module, comm, alltoallv); 
      COPY(avail->ac_module, comm, alltoallw); 
      COPY(avail->ac_module, comm, barrier); 
      COPY(avail->ac_module, comm, bcast); 
      COPY(avail->ac_module, comm, exscan); 
      COPY(avail->ac_module, comm, gather); 
      COPY(avail->ac_module, comm, gatherv); 
      COPY(avail->ac_module, comm, reduce); 
      COPY(avail->ac_module, comm, reduce_scatter); 
      COPY(avail->ac_module, comm, scan); 
      COPY(avail->ac_module, comm, scatter); 
      COPY(avail->ac_module, comm, scatterv); 

      /* release the original module reference */
      OBJ_RELEASE(avail->ac_module);
  }

  /* check to make sure no NULLs */
  if ((NULL == comm->c_coll.coll_allgather) ||
      (NULL == comm->c_coll.coll_allgatherv) ||
      (NULL == comm->c_coll.coll_allreduce) ||
      (NULL == comm->c_coll.coll_alltoall) ||
      (NULL == comm->c_coll.coll_alltoallv) ||
      (NULL == comm->c_coll.coll_alltoallw) ||
      (NULL == comm->c_coll.coll_barrier) ||
      (NULL == comm->c_coll.coll_bcast) ||
      ((OMPI_COMM_IS_INTRA(comm)) && (NULL == comm->c_coll.coll_exscan)) ||
      (NULL == comm->c_coll.coll_gather) ||
      (NULL == comm->c_coll.coll_gatherv) ||
      (NULL == comm->c_coll.coll_reduce) ||
      (NULL == comm->c_coll.coll_reduce_scatter) ||
      ((OMPI_COMM_IS_INTRA(comm)) && (NULL == comm->c_coll.coll_scan)) ||
      (NULL == comm->c_coll.coll_scatter) ||
      (NULL == comm->c_coll.coll_scatterv)) {
      mca_coll_base_comm_unselect(comm);
      return OMPI_ERR_NOT_FOUND;
  }

  return OMPI_SUCCESS;
}


/*
 * For each module in the list, if it is in the list of names (or the
 * list of names is NULL), then check and see if it wants to run, and
 * do the resulting priority comparison.  Make a list of modules to be
 * only those who returned that they want to run, and put them in
 * priority order.
 */
static opal_list_t *check_components(opal_list_t *components, 
                                     ompi_communicator_t *comm, 
                                     char **names, int num_names)
{
  int i, priority;
  const mca_base_component_t *component;
  opal_list_item_t *item, *item2;
  mca_coll_base_module_1_1_0_t *module;
  bool want_to_check;
  opal_list_t *selectable;
  avail_coll_t *avail, *avail2;
  
  /* Make a list of the components that query successfully */

  selectable = OBJ_NEW(opal_list_t);

  /* Scan through the list of components.  This nested loop is O(N^2),
     but we should never have too many components and/or names, so this
     *hopefully* shouldn't matter... */
  
  for (item = opal_list_get_first(components); 
       item != opal_list_get_end(components); 
       item = opal_list_get_next(item)) {
    component = ((mca_base_component_priority_list_item_t *) 
                 item)->super.cli_component;

    /* If we have a list of names, scan through it */

    if (0 == num_names) {
      want_to_check = true;
    } else {
      want_to_check = false;
      for (i = 0; i < num_names; ++i) {
        if (0 == strcmp(names[i], component->mca_component_name)) {
          want_to_check = true;
        }
      }
    }

    /* If we determined that we want to check this component, then do
       so */

    if (want_to_check) {
      priority = check_one_component(comm, component, &module);
      if (priority >= 0) {

        /* We have a component that indicated that it wants to run by
           giving us a module */

        avail = OBJ_NEW(avail_coll_t);
        avail->ac_priority = priority;
        avail->ac_module = module;

        /* Put this item on the list in priority order (lowest
           priority first).  Should it go first? */

        if (opal_list_is_empty(selectable)) {
            opal_list_prepend(selectable, (opal_list_item_t *) avail);
        } else {
            item2 = opal_list_get_first(selectable); 
            avail2 = (avail_coll_t *) item2;
            if (avail->ac_priority < avail2->ac_priority) {
                opal_list_prepend(selectable, (opal_list_item_t *) avail);
            } else {
                for (i = 1; item2 != opal_list_get_end(selectable); 
                     item2 = opal_list_get_next(item2), ++i) {
                    avail2 = (avail_coll_t *) item2;
                    if (avail->ac_priority < avail2->ac_priority) {
                        opal_list_insert(selectable, 
                                         (opal_list_item_t *) avail, i);
                        break;
                    }
                }
                
                /* If we didn't find a place to put it in the list, then
                   append it (because it has the lowest priority found so
                   far) */

                if (opal_list_get_end(selectable) == item2) {
                    opal_list_append(selectable, (opal_list_item_t *) avail);
                }
            }
        }
      }
    }
  }

  /* If we didn't find any available components, return an error */

  if (0 == opal_list_get_size(selectable)) {
    OBJ_RELEASE(selectable);
    return NULL;
  }

  /* All done */

  return selectable;
}


/*
 * Check a single component
 */
static int check_one_component(ompi_communicator_t *comm, 
                               const mca_base_component_t *component,
                               mca_coll_base_module_1_1_0_t **module)
{
  int err;
  int priority = -1;

  err = query(component, comm, &priority, module);

  if (OMPI_SUCCESS == err) {
    priority = (priority < 100) ? priority : 100;
    opal_output_verbose(10, mca_coll_base_output, 
                        "coll:base:comm_select: component available: %s, priority: %d", 
                        component->mca_component_name, priority);

  } else {
    priority = -1;
    opal_output_verbose(10, mca_coll_base_output, 
                        "coll:base:comm_select: component not available: %s",
                        component->mca_component_name);
  }

  return priority;
}


/**************************************************************************
 * Query functions
 **************************************************************************/

/*
 * Take any version of a coll module, query it, and return the right
 * module struct
 */
static int query(const mca_base_component_t *component, 
                 ompi_communicator_t *comm, 
                 int *priority, mca_coll_base_module_1_1_0_t **module)
{
  /* coll v1.0.0 */

  *module = NULL;
  if (1 == component->mca_major_version &&
      0 == component->mca_minor_version &&
      0 == component->mca_release_version) {
    const mca_coll_base_component_1_1_0_t *coll100 = 
      (mca_coll_base_component_1_1_0_t *) component;

    return query_1_1_0(coll100, comm, priority, module);
  } 

  /* Unknown coll API version -- return error */

  return OMPI_ERROR;
}


static int query_1_1_0(const mca_coll_base_component_1_1_0_t *component,
                       ompi_communicator_t *comm, int *priority,
                       mca_coll_base_module_1_1_0_t **module)
{
    mca_coll_base_module_1_1_0_t *ret;

  /* There's currently no need for conversion */

  ret = component->collm_comm_query(comm, priority);
  if (NULL != ret) {
    *module = ret;
    return OMPI_SUCCESS;
  }

  return OMPI_ERROR;
}

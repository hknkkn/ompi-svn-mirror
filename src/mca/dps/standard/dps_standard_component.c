/* -*- C -*-
 *
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
/** @file:
 *
 * The Open MPI Name Server
 *
 * The Open MPI Name Server provides unique name ranges for processes
 * within the universe. Each universe will have one name server
 * running within the seed daemon.  This is done to prevent the
 * inadvertent duplication of names.
 */

/*
 * includes
 */
#include "ompi_config.h"

#include "include/orte_constants.h"
#include "util/output.h"
#include "mca/mca.h"
#include "mca/base/mca_base_param.h"
#include "mca/oob/base/base.h"
#include "mca/ns/base/base.h"
#include "dps_standard.h"


/*
 * Struct of function pointers that need to be initialized
 */
OMPI_COMP_EXPORT mca_dps_base_component_t mca_dps_standard_component = {
  {
    MCA_DPS_BASE_VERSION_1_0_0,

    "standard", /* MCA module name */
    1,  /* MCA module major version */
    0,  /* MCA module minor version */
    0,  /* MCA module release version */
    mca_dps_standard_open,  /* module open */
    mca_dps_standard_close /* module close */
  },
  {
    false /* checkpoint / restart */
  },
  mca_dps_standard_init,    /* module init */
  mca_dps_standard_finalize /* module shutdown */
};

/*
 * setup the function pointers for the module
 */
static mca_dps_base_module_t mca_dps_standard = {
    mca_dps_standard_pack_value,
    mca_dps_standard_unpack_value,
    mca_dps_standard_pack_object,
    mca_dps_standard_unpack_object,
    mca_dps_standard_pack_buffer,
    mca_dps_standard_unpack_buffer,
    mca_dps_standard_init_buffer
};

/*
 * Whether or not we allowed this component to be selected
 */
static bool initialized = false;


/*
 * globals needed within replica component
 */
int mca_dps_standard_debug;

/*
 * don't really need this function - could just put NULL in the above structure
 * Just holding the place in case we decide there is something we need to do
 */
int mca_dps_standard_open(void)
{
    int id;

    id = mca_base_param_register_int("dps", "standard", "debug", NULL, 0);
    mca_base_param_lookup_int(id, &mca_dps_standard_debug);

    return OMPI_SUCCESS;
}

/*
 * ditto for this one
 */
int mca_dps_standard_close(void)
{
    return OMPI_SUCCESS;
}

mca_dps_standard_module_t* mca_dps_standard_init(bool *allow_multi_user_threads, bool *have_hidden_threads, int *priority)
{
      /* Return a module (choose an arbitrary, positive priority
       *  -- it's only relevant compared to other dps components).
       */

      *priority = 50;

      /* We allow multi user threads but don't have any hidden threads */

      *allow_multi_user_threads = true;
      *have_hidden_threads = false;

     /* Return the module */

      initialized = true;

      return &mca_dps_standard;
}

/*
 * finalize routine
 */
int mca_dps_standard_finalize(void)
{
    if (mca_dps_standard_debug) {
	ompi_output(0, "finalizing dps");
    }

    initialized = false;

    /* All done */
    return OMPI_SUCCESS;
}

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
 *
 */

#include "ompi_config.h"

#include "include/orte_constants.h"
#include "include/types.h"
#include "class/ompi_list.h"
#include "mca/mca.h"
#include "mca/base/mca_base_param.h"

#include "mca/pls/base/base.h"

#include <sys/bproc.h>

#include "pls_bproc.h"

/*
 * Struct of function pointers and all that to let us be initialized
 */
mca_pls_base_component_1_0_0_t mca_pls_bproc_component = {
  {
    MCA_PLS_BASE_VERSION_1_0_0,

    "bproc", /* MCA component name */
    1,  /* MCA component major version */
    0,  /* MCA component minor version */
    0,  /* MCA component release version */
    mca_pls_bproc_component_open,  /* component open */
    mca_pls_bproc_component_close /* component close */
  },
  {
    false /* checkpoint / restart */
  },
  mca_pls_bproc_init,    /* component init */
  NULL                 /* unique name */
};

/*
 * Module variables handles
 */
static int param_priority;

int
mca_pls_bproc_component_open(void)
{
  param_priority =
    mca_base_param_register_int("pls", "bproc", "priority", NULL, 5);

  return ORTE_SUCCESS;
}


int
mca_pls_bproc_component_close(void)
{
    return ORTE_SUCCESS;
}


mca_pls_base_module_t*
mca_pls_bproc_init(int *priority, 
		   bool have_threads,
		   int constraints)
{
    int ret;
    mca_ls_bproc_module_t *me;
    struct bproc_version_t vers;

    /* check and initialize the priority */
    mca_base_param_lookup_int(param_priority, priority);

    /* check to see if I'm in a daemon - if not, then we can't use this launcher */
    if (!ompi_process_info.daemon) {
        return NULL;
    }
    
    /* okay, we are in a daemon - now check to see if BProc is running here */
    ret = bproc_version(&vers);
    if (ret != 0) {
      ompi_output_verbose(5, mca_pcm_base_output, 
           "bproc: bproc_version() failed");
      return NULL;
    }
    
    /* only launch from the master node */
    if (bproc_currnode() != BPROC_NODE_MASTER) {
      ompi_output_verbose(5, mca_pcm_base_output, 
            "bproc: not on BPROC_NODE_MASTER");
      return NULL;
    }

    /* ok, now let's try to fire up */
    me = malloc(sizeof(mca_pcm_bproc_module_t));
    if (NULL == me) return NULL;

    /* init constraints and job list */
    me->constraints = constraints;
    me->jobs = NULL;

    /*
     * fill in the function pointers
     */
    me->super.pls_spawn_procs = mca_pls_bproc_spawn_procs;
    me->super.pls_finalize = mca_pls_bproc_finalize;

    return (mca_pls_base_module_t*) me;
}


int
mca_pls_bproc_finalize(struct mca_pls_base_module_1_0_0_t* me_super)
{
    mca_pls_bproc_module_t *me = (mca_pls_bproc_module_t*) me_super;

    if (NULL == me) return ORTE_ERR_BAD_PARAM;

    free(me);

    return ORTE_SUCCESS;
}


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


#include "mca/ns/base/base.h"
#include "mca/pls/base/base.h"

#include "pls_bproc.h"

extern char **environ;

int
mca_pls_bproc_spawn_procs(struct mca_pls_base_module_1_0_0_t* me_super,
                          mca_ns_base_jobid_t jobid)
{
    mca_pcm_bproc_module_t *me = (mca_pcm_bproc_module_t*) me_super;

    return ORTE_SUCCESS;
}


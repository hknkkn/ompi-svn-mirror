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
#include <sys/bproc.h>

#include "include/orte_constants.h"
#include "include/types.h"
#include "class/ompi_list.h"
#include "mca/mca.h"
#include "mca/base/mca_base_param.h"
#include "svc_bproc.h"

/*
 * Struct of function pointers and all that to let us be initialized
 */
orte_svc_bproc_base_module_t orte_svc_bproc_module = {
    orte_svc_bproc_module_finalize
};

/**
 * Cleanup resources held by module.
 */

int orte_svc_bproc_module_finalize(void)
{
    return ORTE_SUCCESS;
}


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
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/bproc.h>

#include "include/orte_constants.h"
#include "include/orte_types.h"
#include "util/argv.h"
#include "util/output.h"
#include "mca/ras/base/base.h"
#include "mca/ras/base/ras_base_node.h"
#include "ras_proxy.h"



/**
 *  Discover available (pre-allocated) nodes. Allocate the
 *  requested number of nodes/process slots to the job.
 *  
 */

static int orte_ras_proxy_allocate(orte_jobid_t jobid)
{
    return ORTE_SUCCESS;
}



static int orte_ras_proxy_deallocate(orte_jobid_t jobid)
{
    return ORTE_SUCCESS;
}


static int orte_ras_proxy_finalize(void)
{
    return ORTE_SUCCESS;
}


orte_ras_base_module_t orte_ras_proxy_module = {
    orte_ras_proxy_allocate,
    orte_ras_proxy_deallocate,
    orte_ras_proxy_finalize
};


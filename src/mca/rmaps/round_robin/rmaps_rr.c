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

#include "include/orte_constants.h"
#include "include/orte_types.h"
#include "util/output.h"
#include "mca/rmaps/base/base.h"
#include "rmaps_rr.h"




static int orte_rmaps_round_robin_map(orte_jobid_t jobid)
{
    return ORTE_SUCCESS;
}


static int orte_rmaps_round_robin_finalize(void)
{
    return ORTE_SUCCESS;
}


orte_rmaps_base_module_t orte_rmaps_round_robin_module = {
    orte_rmaps_round_robin_map,
    orte_rmaps_round_robin_finalize
};


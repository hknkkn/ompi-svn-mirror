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

#include "util/output.h"
#include "mca/mca.h"
#include "mca/base/base.h"
#include "mca/base/mca_base_param.h"
#include "mca/rmaps/base/rmaps_base_map.h"


int orte_rmaps_base_get_map(orte_jobid_t jobid, ompi_list_t* map)
{
    return ORTE_SUCCESS;
}

int orte_rmaps_base_set_map(orte_jobid_t jobid, ompi_list_t* map)
{
    return ORTE_SUCCESS;
}



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
/** @file:
 *
 */

#include "orte_config.h"

#include "include/orte_constants.h"

#include "mca/ns/ns_types.h"

#include "mca/orte_rmaps/base/base.h"


/*
 * "not available" functions
 */
int
mca_orte_rmaps_base_query_not_available(orte_jobid_t job)
{
    return ORTE_ERR_UNREACH;
}

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

#include "mca/mca.h"
#include "mca/ras/base/base.h"

/**
 * globals
 */


/*
 * "not available" functions
 */
int
mca_orte_ras_base_allocate_not_available(void)
{
    return ORTE_ERR_UNREACH;
}

int
mca_orte_ras_base_deallocate_not_available(void)
{
    return ORTE_ERR_UNREACH;
}

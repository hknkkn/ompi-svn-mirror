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
 * The Open MPI General Purpose Registry - Replica component
 *
 */

/*
 * includes
 */
#include "orte_config.h"

#include "util/proc_info.h"
#include "mca/ns/ns_types.h"

#include "gpr_replica_api.h"


int orte_gpr_replica_increment_value(orte_gpr_value_t *value)
{
    return ORTE_SUCCESS;
}

int orte_gpr_replica_decrement_value(orte_gpr_value_t *value)
{
    return ORTE_SUCCESS;
}

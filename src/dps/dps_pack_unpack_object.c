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
 
/*
 * DPS Buffer Operations
 */
 
/** @file:
 *
 */

#include "ompi_config.h"

#include "dps_internal.h"


int orte_dps_pack_object(orte_buffer_t *buffer, void *src,
                         char **descriptions,
                         orte_pack_type_t *types)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_dps_unpack_object(orte_buffer_t *buffer, void **dest,
                           char **descriptions,
                           orte_pack_type_t *types)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}


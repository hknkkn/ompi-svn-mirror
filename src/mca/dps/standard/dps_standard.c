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
#include "ompi_config.h"

#include "dps_standard.h"

/**
 * globals
 */

/*
 * functions
 */

int mca_dps_standard_pack_value(void *dest, void *src,
                                orte_pack_type_t type,
                                int num_values)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int mca_dps_standard_unpack_value(void *dest, void *src,
                                  orte_pack_type_t type,
                                  int num_values)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int mca_dps_standard_pack_object(void *dest, void *src,
                                 orte_pack_type_t *types)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int mca_dps_standard_unpack_object(void *dest, void *src,
                                   orte_pack_type_t *types)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int mca_dps_standard_pack_buffer(orte_buffer_t *buffer, void *src,
                                 orte_pack_type_t type, int num_values)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int mca_dps_standard_unpack_buffer(orte_buffer_t *buffer, void *src,
                                   orte_pack_type_t, int num_values)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int mca_dps_standard_init_buffer(orte_buffer_t **buffer, int size)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}
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
#ifndef ORTE_DPS_INTERNAL_H_
#define ORTE_DPS_INTERNAL_H_

#include "orte_config.h"

#include "include/orte_constants.h"

#include "dps.h"

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

/*
 * globals needed within dps
 */
extern bool orte_dps_debug;

/*
 * Implementations of API functions
 */
int orte_dps_buffer_init(orte_buffer_t **buffer, char *label);

int orte_dps_pack_value(orte_buffer_t *buffer, void *src,
                        char *description,
                        orte_pack_type_t type);

int orte_dps_unpack_value(orte_buffer_t *buffer, void *dest,
                          char *description,
                          orte_pack_type_t *type);

int orte_dps_pack_object(orte_buffer_t *buffer, void *src,
                         char **descriptions,
                         orte_pack_type_t *types);

int orte_dps_unpack_object(orte_buffer_t *buffer, void **dest,
                           char **descriptions,
                           orte_pack_type_t *types);

int orte_dps_buffer_free(orte_buffer_t **buffer);


/*
 * Totally internal functions
 */
size_t orte_dps_memory_required(bool packed, void *src, orte_pack_type_t type);

int orte_dps_buffer_extend (orte_buffer_t *bptr, size_t mem_req);


#if defined(c_plusplus) || defined(__cplusplus)
}
#endif
#endif

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
/**
 * @file
 *
 * Resource Discovery & Allocation Subsystem (RDAS)
 *
 * The RDAS is responsible for discovering the resources available to the universe, and
 * for allocating them to the requesting job.
 *
 */

#ifndef ORTE_DPS_H_
#define ORTE_DPS_H_

#include "orte_config.h"

#include "dps_types.h"

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif
/*
 * DPS initialization function
 * In dynamic libraries, declared objects and functions don't get loaded
 * until called. We need to ensure that the orte_dps function structure
 * gets loaded, so we provide an "open" call that is executed as part of
 * the program startup. It simply checks for debug parameters - good enough
 * to ensure that the DPS gets loaded!
 */
OMPI_DECLSPEC int orte_dps_open(void);

/*
 * DPS finalize function
 */
OMPI_DECLSPEC int orte_dps_close(void);


/*
 * DPS interface functions
 */

typedef int (*orte_dps_init_buffer_fn_t)(orte_buffer_t **buffer, char *label);

typedef int (*orte_dps_pack_value_fn_t)(orte_buffer_t *buffer, void *src,
                                        char *description,
                                        orte_pack_type_t type);

typedef int (*orte_dps_unpack_value_fn_t)(orte_buffer_t *buffer, void *dest,
                                          char *description,
                                          orte_pack_type_t *type);

typedef int (*orte_dps_pack_object_fn_t)(orte_buffer_t *buffer, void *src,
                                         char **descriptions,
                                         orte_pack_type_t *types);

typedef int (*orte_dps_unpack_object_fn_t)(orte_buffer_t *buffer, void **dest,
                                           char **descriptions,
                                           orte_pack_type_t *types);

typedef int (*orte_dps_free_buffer_fn_t)(orte_buffer_t **buffer);


/**
 * Base structure for the DPS
 *
 * Base module structure for the DPS - presents the required function
 * pointers to the calling interface. 
 */
struct orte_dps_t {
    orte_dps_init_buffer_fn_t buffer_init;
    orte_dps_pack_value_fn_t pack_value;
    orte_dps_unpack_value_fn_t unpack_value;
    orte_dps_pack_object_fn_t pack_object;
    orte_dps_unpack_object_fn_t unpack_object;
    orte_dps_free_buffer_fn_t buffer_free;
};
typedef struct orte_dps_t orte_dps_t;

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif

OMPI_DECLSPEC extern orte_dps_t orte_dps;  /* holds dps function pointers */

#endif /* ORTE_DPS_H */

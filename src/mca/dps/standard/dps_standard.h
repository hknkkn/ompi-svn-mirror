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
#ifndef NS_REPLICA_H
#define NS_REPLICA_H

#include "ompi_config.h"
#include "include/types.h"
#include "include/orte_constants.h"

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

/*
 * globals needed within component
 */
extern int mca_dps_standard_debug;

/*
 * Module open / close
 */
int mca_dps_standard_open(void);
int mca_dps_standard_close(void);


/*
 * Startup / Shutdown
 */
mca_dps_base_module_t* mca_dps_standard_init(bool *allow_multi_user_threads, bool *have_hidden_threads, int *priority);
int mca_dps_standard_finalize(void);

/*
 * Implementations
 */
int mca_dps_standard_pack_value(void *dest, void *src,
                                orte_pack_type_t type,
                                int num_values);

int mca_dps_standard_unpack_value(void *dest, void *src,
                                  orte_pack_type_t type,
                                  int num_values);

int mca_dps_standard_pack_object(void *dest, void *src,
                                 orte_pack_type_t *types);

int mca_dps_standard_unpack_object(void *dest, void *src,
                                   orte_pack_type_t *types);

int mca_dps_standard_pack_buffer(orte_buffer_t *buffer, void *src,
                                 orte_pack_type_t type, int num_values);

int mca_dps_standard_unpack_buffer(orte_buffer_t *buffer, void *src,
                                   orte_pack_type_t, int num_values);

int mca_dps_standard_init_buffer(orte_buffer_t **buffer, int size);


#if defined(c_plusplus) || defined(__cplusplus)
}
#endif
#endif

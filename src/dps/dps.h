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

#include "ompi_config.h"

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif
/**
 * Supported datatypes for conversion operations.
 * NOTE, these have (or should) a one to one match to ompi_pack_type_t
 *
 */

typedef uint8_t orte_pack_type_t;

#define ORTE_BYTE               (orte_pack_type_t)   1     /**< a byte of data */
#define ORTE_INT8               (orte_pack_type_t)   2     /**< an 8-bit integer */
#define ORTE_INT16              (orte_pack_type_t)   3     /**< a 16 bit integer */
#define ORTE_INT32              (orte_pack_type_t)   4     /**< a 32 bit integer */
#define ORTE_STRING             (orte_pack_type_t)   5     /**< a NULL terminated string */
#define ORTE_NAME               (orte_pack_type_t)   6     /**< an ompi_process_name_t */
#define ORTE_JOBID              (orte_pack_type_t)   7     /**< a jobid */
#define ORTE_CELLID             (orte_pack_type_t)   8     /**< a cellid */
#define ORTE_NODE_STATE         (orte_pack_type_t)   9     /**< node status flag */
#define ORTE_PROCESS_STATUS     (orte_pack_type_t)  10     /**< process status key */
#define ORTE_EXIT_CODE          (orte_pack_type_t)  11     /**< process exit code */
#define ORTE_PACKED             (orte_pack_type_t)  12     /**< already packed data. */

typedef struct orte_buffer_t {
     /* first member must be the objects parent */
    ompi_object_t parent;
    
     /* now for the real elements of the type */

    void*   base_ptr;  /* start of my memory */
    void*   data_ptr;  /* location of where next data will go */
    void*   from_ptr;  /* location of where to get the next data from */

    /* counters */

    size_t    size;      /* total size of this buffer */
    size_t    len;       /* total amount already packed */
    size_t    space;     /* how much space we have left */
                         /* yep, size=len+space */

    size_t    toend;     /* how many bytes till the end when unpacking :) */
                         /* yep, toend is the opposite of len */


    size_t    cnt;     /* temp cnt of buffer usage (debugging) */
} orte_buffer_t;

/* formalise the declaration */
OMPI_DECLSPEC OBJ_CLASS_DECLARATION (orte_buffer_t);


/*
 * DPS interface functions
 */

typedef int (*orte_dps_base_pack_value_fn_t)(void *dest, void *src,
                                            orte_pack_type_t type,
                                            int num_values);

typedef int (*orte_dps_base_unpack_value_fn_t)(void *dest, void *src,
                                              orte_pack_type_t type,
                                              int num_values);

typedef int (*orte_dps_base_pack_object_fn_t)(void *dest, void *src,
                                             orte_pack_type_t *types);

typedef int (*orte_dps_base_unpack_object_fn_t)(void *dest, void *src,
                                             orte_pack_type_t *types);

typedef int (*orte_dps_base_pack_buffer_fn_t)(orte_buffer_t *buffer, void *src,
                                             orte_pack_type_t type, int num_values);

typedef int (*orte_dps_base_unpack_buffer_fn_t)(orte_buffer_t *buffer, void *src,
                                               orte_pack_type_t, int num_values);

typedef int (*orte_dps_base_init_buffer_fn_t)(orte_buffer_t **buffer, int size);


/**
 * Base structure for the DPS
 *
 * Base module structure for the DPS - presents the required function
 * pointers to the calling interface. 
 */
struct orte_dps_t {
    orte_dps_base_pack_value_fn_t pack_value;
    orte_dps_base_unpack_value_fn_t unpack_value;
    orte_dps_base_pack_object_fn_t pack_object;
    orte_dps_base_unpack_object_fn_t unpack_object;
    orte_dps_base_pack_buffer_fn_t pack_buffer;
    orte_dps_base_unpack_buffer_fn_t unpack_buffer;
    orte_dps_base_init_buffer_fn_t init_buffer;
};
typedef struct orte_dps_t orte_dps_t;

OMPI_DECLSPEC extern int mca_dps_base_output;
OMPI_DECLSPEC extern orte_dps_t orte_dps;  /* holds dps function pointers */

#endif /* ORTE_DPS_H */

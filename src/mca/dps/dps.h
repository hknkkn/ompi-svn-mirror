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

#ifndef MCA_DPS_H
#define MCA_DPS_H

#include "ompi_config.h"
#include "mca/mca.h"

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
 * MCA component management functions
 */

/**
 * DPS initialization function
 *
 * Called by the MCA framework to initialize the component.  Will
 * be called exactly once in the lifetime of the process.
 *
 * @param have_threads (IN) Whether the current running process is
 *                       multi-threaded or not.  true means there
 *                       may be concurrent access into the
 *                       underlying components *and* that the
 *                       components may launch new threads.
 * @param priority (OUT) Relative priority or ranking use by MCA to
 *                       select a module.
 *
 */
typedef struct mca_dps_base_module_1_0_0_t* 
(*mca_rdas_base_component_init_fn_t)(bool have_threads,
                                    int *priority);


/** 
 * DPS module version and interface functions
 *
 * \note the first two entries have type names that are a bit
 *  misleading.  The plan is to rename the mca_base_module_*
 * types in the future.
 */
struct mca_dps_base_component_1_0_0_t {
  /** component version */
  mca_base_component_t dps_version;
  /** component data */
  mca_base_component_data_1_0_0_t dps_data;
  /** Function called when component is initialized  */
  mca_dps_base_component_init_fn_t dps_init;
};
/** shorten mca_dps_base_component_1_0_0_t declaration */
typedef struct mca_dps_base_component_1_0_0_t mca_dps_base_component_1_0_0_t;
/** shorten mca_dps_base_component_t declaration */
typedef mca_dps_base_component_1_0_0_t mca_dps_base_component_t;


/*
 * DPS interface functions
 */

typedef int (*mca_dps_base_pack_value_fn_t)(void *dest, void *src,
                                            orte_pack_type_t type,
                                            int num_values);

typedef int (*mca_dps_base_unpack_value_fn_t)(void *dest, void *src,
                                              orte_pack_type_t type,
                                              int num_values);

typedef int (*mca_dps_base_pack_object_fn_t)(void *dest, void *src,
                                             orte_pack_type_t *types);

typedef int (*mca_dps_base_unpack_object_fn_t)(void *dest, void *src,
                                             orte_pack_type_t *types);

typedef int (*mca_dps_base_pack_buffer_fn_t)(orte_buffer_t *buffer, void *src,
                                             orte_pack_type_t type, int num_values);

typedef int (*mca_dps_base_unpack_buffer_fn_t)(orte_buffer_t *buffer, void *src,
                                               orte_pack_type_t, int num_values);

typedef int (*mca_dps_base_init_buffer_fn_t)(orte_buffer_t **buffer, int size);


/**
 * Base module structure for the DPS
 *
 * Base module structure for the DPS - presents the required function
 * pointers to the calling interface. 
 */
struct mca_dps_base_module_1_0_0_t {
    mca_dps_base_pack_value_fn_t pack_value;
    mca_dps_base_unpack_value_fn_t unpack_value;
    mca_dps_base_pack_object_fn_t pack_object;
    mca_dps_base_unpack_object_fn_t unpack_object;
    mca_dps_base_pack_buffer_fn_t pack_buffer;
    mca_dps_base_unpack_buffer_fn_t unpack_buffer;
    mca_dps_base_init_buffer_fn_t init_buffer;
};
/** shorten mca_dps_base_module_1_0_0_t declaration */
typedef struct mca_dps_base_module_1_0_0_t mca_dps_base_module_1_0_0_t;
/** shorten mca_dps_base_module_t declaration */
typedef struct mca_dps_base_module_1_0_0_t mca_dps_base_module_t;


/**
 * Macro for use in modules that are of type dps v1.0.0
 */
#define MCA_DPS_BASE_VERSION_1_0_0 \
  /* dps v1.0 is chained to MCA v1.0 */ \
  MCA_BASE_VERSION_1_0_0, \
  /* dps v1.0 */ \
  "dps", 1, 0, 0

#endif /* MCA_DPS_H */

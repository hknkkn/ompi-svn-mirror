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

#ifndef MCA_PLS_H
#define MCA_PLS_H

#include "ompi_config.h"
#include "mca/mca.h"
#include "class/ompi_list.h"

/*
 * Define mapper algorithm options
 */
 typedef uint8_t mca_pls_base_mapper_t;
 
 #define ORTE_PROCESS_MAPPER_ROUND_ROBIN    (mca_pls_base_mapper_t)  1
 #define ORTE_PROCESS_MAPPER_SINGLE_VALUE   (mca_pls_base_mapper_t)  2
 
 
/*
 * MCA component management functions
 */

/**
 * PLS initialization function
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
typedef struct mca_pls_base_module_1_0_0_t* 
(*mca_pls_base_component_init_fn_t)(bool have_threads,
                                    int *priority);

typedef int (*mca_pls_base_component_finalize_fn_t)(void);


/** 
 * PLS module version and interface functions
 *
 * \note the first two entries have type names that are a bit
 *  misleading.  The plan is to rename the mca_base_module_*
 * types in the future.
 */
struct mca_pls_base_component_1_0_0_t {
  /** component version */
  mca_base_component_t pls_version;
  /** component data */
  mca_base_component_data_1_0_0_t pls_data;
  /** Function called when component is initialized  */
  mca_pls_base_component_init_fn_t pls_init;
  /** Function called when component is finalized */
  mca_pls_base_component_finalize_fn_t pls_finalize;
};
/** shorten mca_pls_base_component_1_0_0_t declaration */
typedef struct mca_pls_base_component_1_0_0_t mca_pls_base_component_1_0_0_t;
/** shorten mca_pls_base_component_t declaration */
typedef mca_pls_base_component_1_0_0_t mca_pls_base_component_t;


/*
 * PLS interface functions
 */

/*
 * dummy function for compiling
 */
typedef int (*mca_pls_dummy_fn_t)(void);

/**
 * Base module structure for the PLS
 *
 * Base module structure for the PLS - presents the required function
 * pointers to the calling interface. 
 */
struct mca_pls_base_module_1_0_0_t {
   mca_pls_dummy_fn_t dummy;
};
/** shorten mca_pls_base_module_1_0_0_t declaration */
typedef struct mca_pls_base_module_1_0_0_t mca_pls_base_module_1_0_0_t;
/** shorten mca_pls_base_module_t declaration */
typedef struct mca_pls_base_module_1_0_0_t mca_pls_base_module_t;


/**
 * Macro for use in modules that are of type pml v1.0.0
 */
#define MCA_PLS_BASE_VERSION_1_0_0 \
  /* pls v1.0 is chained to MCA v1.0 */ \
  MCA_BASE_VERSION_1_0_0, \
  /* pls v1.0 */ \
  "pls", 1, 0, 0

#endif /* MCA_PLS_H */

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
 * The Open RTE Process Launch Subsystem
 *
 */

#ifndef MCA_PLS_H
#define MCA_PLS_H

#include "ompi_config.h"
#include "mca/mca.h"
#include "mca/ns/ns_types.h"
#include "class/ompi_list.h"

/*
 * PLS interface functions
 */

/**
 * Launch the indicated jobid 
 */

typedef int (*orte_pls_base_module_launch_fn_t)(orte_jobid_t);

/**
 * Cleanup all resources held by the module
 */
typedef int (*orte_pls_base_module_finalize_fn_t)(void);

/**
 * Base module structure for the PLS
 *
 * Base module structure for the PLS - presents the required function
 * pointers to the calling interface. 
 */

struct orte_pls_base_module_1_0_0_t {
   orte_pls_base_module_launch_fn_t launch;
   orte_pls_base_module_finalize_fn_t finalize;
};

/** shorten orte_pls_base_module_1_0_0_t declaration */
typedef struct orte_pls_base_module_1_0_0_t orte_pls_base_module_1_0_0_t;
/** shorten orte_pls_base_module_t declaration */
typedef struct orte_pls_base_module_1_0_0_t orte_pls_base_module_t;

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
typedef struct orte_pls_base_module_1_0_0_t* 
(*orte_pls_base_component_init_fn_t)(
    bool *allow_multi_user_threads,
    bool *have_hidden_threads);

/** 
 * PLS component 
 */
struct orte_pls_base_component_1_0_0_t {
    /** component version */
    mca_base_component_t pls_version;
    /** component data */
    mca_base_component_data_1_0_0_t pls_data;
    /** Function called when component is initialized  */
    orte_pls_base_component_init_fn_t pls_init;
};
/** shorten orte_pls_base_component_1_0_0_t declaration */
typedef struct orte_pls_base_component_1_0_0_t orte_pls_base_component_1_0_0_t;
/** shorten orte_pls_base_component_t declaration */
typedef orte_pls_base_component_1_0_0_t orte_pls_base_component_t;


/**
 * Macro for use in modules that are of type pml v1.0.0
 */
#define ORTE_PLS_BASE_VERSION_1_0_0 \
  /* pls v1.0 is chained to MCA v1.0 */ \
  MCA_BASE_VERSION_1_0_0, \
  /* pls v1.0 */ \
  "pls", 1, 0, 0

#endif /* MCA_PLS_H */

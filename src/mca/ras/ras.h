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
 * The Open RTE Resource Allocation Subsystem (RAS)
 */

#ifndef MCA_RAS_H
#define MCA_RAS_H

/*
 * includes
 */

#include "orte_config.h"
#include "include/orte_constants.h"

#include "mca/mca.h"

#include "ras_types.h"


/*
 * Component functions - all MUST be provided!
 */

/**
 * Allocate resources to a job.
 * 
 * @code
 * new_cellid = ompi_name_server.create_cellid()
 * @endcode
 */
typedef int (*orte_ras_base_module_allocate_fn_t)(orte_jobid_t jobid);

/**
 * Deallocate resources from a job
 *
 * @code
 * return_value = ompi_name_server.assign_cellid_to_process(ompi_process_name_t* name);
 * @endcode
 */
typedef int (*orte_ras_base_module_deallocate_fn_t)(orte_jobid_t jobid);


/*
 * Ver 1.0.0
 */
struct mca_orte_ras_base_module_1_0_0_t {
    mca_orte_ras_base_module_allocate_fn_t allocate;
    mca_orte_ras_base_module_deallocate_fn_t deallocate;
};

typedef struct mca_orte_ras_base_module_1_0_0_t mca_orte_ras_base_module_1_0_0_t;
typedef mca_orte_ras_base_module_1_0_0_t mca_orte_ras_base_module_t;

/*
 * RAS Component
 */

typedef mca_orte_ras_base_module_t* (*mca_orte_ras_base_component_init_fn_t)(
    bool *allow_multi_user_threads,
    bool *have_hidden_threads,
    int *priority);

typedef int (*mca_orte_ras_base_component_finalize_fn_t)(void);
 
/*
 * the standard component data structure
 */

struct mca_orte_ras_base_component_1_0_0_t {
    mca_base_component_t ras_version;
    mca_base_component_data_1_0_0_t ras_data;

    mca_orte_ras_base_component_init_fn_t ras_init;
    mca_orte_ras_base_component_finalize_fn_t ras_finalize;
};
typedef struct mca_orte_ras_base_component_1_0_0_t mca_orte_ras_base_component_1_0_0_t;
typedef mca_orte_ras_base_component_1_0_0_t mca_orte_ras_base_component_t;



/*
 * Macro for use in components that are of type ns v1.0.0
 */
#define ORTE_MCA_RAS_BASE_VERSION_1_0_0 \
  /* ras v1.0 is chained to MCA v1.0 */ \
  MCA_BASE_VERSION_1_0_0, \
  /* ns v1.0 */ \
  "ras", 1, 0, 0

/* Global structure for accessing RAS functions
 */
OMPI_DECLSPEC extern mca_orte_ras_base_module_t orte_ras;  /* holds selected module's function pointers */

#endif

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
 * The Open RTE Resource MAPping Subsystem (RMAPS)
 */

#ifndef ORTE_MCA_RMAPS_H
#define ORTE_MCA_RMAPS_H

/*
 * includes
 */

#include "orte_config.h"
#include "include/orte_constants.h"

#include "mca/mca.h"
#include "mca/ns/ns_types.h"

#include "rmaps_types.h"


/*
 * Component functions - all MUST be provided!
 */

/**
 * Query/update a resource
 *
 * @code
 * return_value = ompi_name_server.assign_cellid_to_process(ompi_process_name_t* name);
 * @endcode
 */
typedef int (*orte_mca_rmaps_base_module_map_fn_t)(orte_jobid_t job);


/*
 * Ver 1.0.0
 */
struct orte_mca_rmaps_base_module_1_0_0_t {
    orte_mca_rmaps_base_module_map_fn_t map;
};

typedef struct orte_mca_rmaps_base_module_1_0_0_t orte_mca_rmaps_base_module_1_0_0_t;
typedef orte_mca_rmaps_base_module_1_0_0_t orte_mca_rmaps_base_module_t;

/*
 * RMAPS Component
 */

typedef orte_mca_rmaps_base_module_t* (*orte_mca_rmaps_base_component_init_fn_t)(
    bool *allow_multi_user_threads,
    bool *have_hidden_threads,
    int *priority);

typedef int (*orte_mca_rmaps_base_component_finalize_fn_t)(void);
 
/*
 * the standard component data structure
 */

struct orte_mca_rmaps_base_component_1_0_0_t {
    mca_base_component_t rmaps_version;
    mca_base_component_data_1_0_0_t rmaps_data;

    orte_mca_rmaps_base_component_init_fn_t rmaps_init;
    orte_mca_rmaps_base_component_finalize_fn_t rmaps_finalize;
};
typedef struct orte_mca_rmaps_base_component_1_0_0_t orte_mca_rmaps_base_component_1_0_0_t;
typedef orte_mca_rmaps_base_component_1_0_0_t orte_mca_rmaps_base_component_t;



/*
 * Macro for use in components that are of type ns v1.0.0
 */
#define MCA_ORTE_RMAPS_BASE_VERSION_1_0_0 \
  /* ras v1.0 is chained to MCA v1.0 */ \
  MCA_BASE_VERSION_1_0_0, \
  /* ras v1.0 */ \
  "orte_rmaps", 1, 0, 0

/* Global structure for accessing RMAPS functions
 */
OMPI_DECLSPEC extern orte_mca_rmaps_base_module_t orte_rmaps;  /* holds selected module's function pointers */

#endif

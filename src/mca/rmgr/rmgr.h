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

#ifndef MCA_ORTE_RAS_H
#define MCA_ORTE_RAS_H

/*
 * includes
 */

#include "orte_config.h"
#include "include/orte_constants.h"

#include "mca/mca.h"
#include "mca/ns/ns_types.h"

#include "rmgr_types.h"


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
typedef int (*orte_rmgr_base_module_query_fn_t)(void);
                                                                                                                                          
/**
 * Allocate resources to a job.
 * 
 * @code
 * new_cellid = ompi_name_server.create_cellid()
 * @endcode
 */
typedef int (*orte_rmgr_base_module_allocate_fn_t)(orte_jobid_t jobid);

/**
 * Deallocate resources from a job
 *
 * @code
 * return_value = ompi_name_server.assign_cellid_to_process(ompi_process_name_t* name);
 * @endcode
 */
typedef int (*orte_rmgr_base_module_deallocate_fn_t)(orte_jobid_t jobid);

/**
 * Map resources to a job
 *
 * @code
 * return_value = ompi_name_server.assign_cellid_to_process(ompi_process_name_t* name);
 * @endcode
 */
typedef int (*orte_rmgr_base_module_map_fn_t)(orte_jobid_t job);

/**
 * Cleanup resources held by rmgr.
 */

typedef int (*orte_rmgr_base_module_finalize_fn_t)(void);

/*
 * Ver 1.0.0
 */
struct orte_rmgr_base_module_1_0_0_t {
    orte_rmgr_base_module_query_fn_t query;
    orte_rmgr_base_module_allocate_fn_t allocate;
    orte_rmgr_base_module_deallocate_fn_t deallocate;
    orte_rmgr_base_module_map_fn_t map;
    orte_rmgr_base_module_finalize_fn_t rmgr_finalize;
};

typedef struct orte_rmgr_base_module_1_0_0_t orte_rmgr_base_module_1_0_0_t;
typedef orte_rmgr_base_module_1_0_0_t orte_rmgr_base_module_t;

/*
 * RAS Component
 */

typedef orte_rmgr_base_module_t* (*orte_rmgr_base_component_init_fn_t)(
    bool *allow_multi_user_threads,
    bool *have_hidden_threads,
    int *priority);

 
/*
 * the standard component data structure
 */

struct orte_rmgr_base_component_1_0_0_t {
    mca_base_component_t rmgr_version;
    mca_base_component_data_1_0_0_t rmgr_data;
    orte_rmgr_base_component_init_fn_t rmgr_init;
};
typedef struct orte_rmgr_base_component_1_0_0_t orte_rmgr_base_component_1_0_0_t;
typedef orte_rmgr_base_component_1_0_0_t orte_rmgr_base_component_t;



/*
 * Macro for use in components that are of type ns v1.0.0
 */
#define MCA_ORTE_RAS_BASE_VERSION_1_0_0 \
  /* rmgr v1.0 is chained to MCA v1.0 */ \
  MCA_BASE_VERSION_1_0_0, \
  /* rmgr v1.0 */ \
  "orte_rmgr", 1, 0, 0

/* Global structure for accessing RAS functions
 */
OMPI_DECLSPEC extern orte_rmgr_base_module_t orte_rmgr;  /* holds selected module's function pointers */

#endif

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
 * The Open RTE Resource Discovery Subsystem (RDS)
 */

#ifndef MCA_ORTE_RDS_H
#define MCA_ORTE_RDS_H

/*
 * includes
 */

#include "orte_config.h"
#include "include/orte_constants.h"

#include "mca/mca.h"
#include "mca/ns/ns_types.h"

#include "orte_rds_types.h"


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
typedef int (*mca_orte_rds_base_module_query_fn_t)(void);


/*
 * Ver 1.0.0
 */
struct mca_orte_rds_base_module_1_0_0_t {
    mca_orte_rds_base_module_query_fn_t query;
};

typedef struct mca_orte_rds_base_module_1_0_0_t mca_orte_rds_base_module_1_0_0_t;
typedef mca_orte_rds_base_module_1_0_0_t mca_orte_rds_base_module_t;

/*
 * RDS Component
 */

typedef mca_orte_rds_base_module_t* (*mca_orte_rds_base_component_init_fn_t)(
    bool *allow_multi_user_threads,
    bool *have_hidden_threads,
    int *priority);

typedef int (*mca_orte_rds_base_component_finalize_fn_t)(void);
 
/*
 * the standard component data structure
 */

struct mca_orte_rds_base_component_1_0_0_t {
    mca_base_component_t rds_version;
    mca_base_component_data_1_0_0_t rds_data;

    mca_orte_rds_base_component_init_fn_t rds_init;
    mca_orte_rds_base_component_finalize_fn_t rds_finalize;
};
typedef struct mca_orte_rds_base_component_1_0_0_t mca_orte_rds_base_component_1_0_0_t;
typedef mca_orte_rds_base_component_1_0_0_t mca_orte_rds_base_component_t;



/*
 * Macro for use in components that are of type ns v1.0.0
 */
#define MCA_ORTE_RDS_BASE_VERSION_1_0_0 \
  /* ras v1.0 is chained to MCA v1.0 */ \
  MCA_BASE_VERSION_1_0_0, \
  /* ras v1.0 */ \
  "orte_rds", 1, 0, 0

/* Global structure for accessing RDS functions
 */
OMPI_DECLSPEC extern mca_orte_rds_base_module_t orte_rds;  /* holds selected module's function pointers */

#endif

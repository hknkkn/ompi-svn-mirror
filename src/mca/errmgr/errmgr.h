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
 * The Open RTE Error Manager
 *
 */

#ifndef ORTE_MCA_ERRMGR_H
#define ORTE_MCA_ERRMGR_H

/*
 * includes
 */

#include "orte_config.h"
#include "include/orte_constants.h"
#include "include/orte_schema.h"

#include "mca/mca.h"

/*
 * Macro definitions
 */
#define ORTE_ERROR_LOG(n) \
    orte_errmgr.log(ORTE_ERROR_NAME(n), __FILE__, __LINE__)


/*
 * Component functions - all MUST be provided!
 */

/**
 * Log an error
 * Log an error that occurred in the runtime environment.
 * 
 * @code
 * orte_errmgr.log("this is an error", __FILE__, __LINE__);
 * @endcode
 */
typedef void (*orte_errmgr_base_module_log_fn_t)(char *msg, char *filename, int line);


/**
 * Alert - process aborted
 */
typedef void (*orte_errmgr_base_module_proc_aborted_fn_t)(orte_process_name_t *proc);

/**
 * Alert - incomplete start of a job
 */
typedef void (*orte_errmgr_base_module_incomplete_start_fn_t)(orte_jobid_t job);


/*
 * Ver 1.0.0
 */
struct orte_errmgr_base_module_1_0_0_t {
    orte_errmgr_base_module_log_fn_t log;
    orte_errmgr_base_module_proc_aborted_fn_t proc_aborted;
    orte_errmgr_base_module_incomplete_start_fn_t incomplete_start;
};

typedef struct orte_errmgr_base_module_1_0_0_t orte_errmgr_base_module_1_0_0_t;
typedef orte_errmgr_base_module_1_0_0_t orte_errmgr_base_module_t;

/*
 * ERRMGR Component
 */

typedef orte_errmgr_base_module_t* (*orte_errmgr_base_component_init_fn_t)(
    bool *allow_multi_user_threads,
    bool *have_hidden_threads,
    int *priority);

typedef int (*orte_errmgr_base_component_finalize_fn_t)(void);
 
/*
 * the standard component data structure
 */

struct mca_errmgr_base_component_1_0_0_t {
    mca_base_component_t errmgr_version;
    mca_base_component_data_1_0_0_t errmgr_data;

    orte_errmgr_base_component_init_fn_t errmgr_init;
    orte_errmgr_base_component_finalize_fn_t errmgr_finalize;
};
typedef struct mca_errmgr_base_component_1_0_0_t mca_errmgr_base_component_1_0_0_t;
typedef mca_errmgr_base_component_1_0_0_t mca_errmgr_base_component_t;



/*
 * Macro for use in components that are of type errmgr v1.0.0
 */
#define ORTE_ERRMGR_BASE_VERSION_1_0_0 \
  /* ns v1.0 is chained to MCA v1.0 */ \
  MCA_BASE_VERSION_1_0_0, \
  /* errmgr v1.0 */ \
  "errmgr", 1, 0, 0

/* Global structure for accessing error manager functions
 */
OMPI_DECLSPEC extern orte_errmgr_base_module_t orte_errmgr;  /* holds selected module's function pointers */

#endif

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
/** @file **/

#ifndef ORTE_SVC_H
#define ORTE_SVC_H

#include "ompi_config.h"

#include "mpi.h"
#include "mca/mca.h"
#include "mca/base/base.h"


/**************************************************************************
 * Service component
 **************************************************************************/

struct orte_svc_base_module_t;
typedef struct orte_svc_base_module_t *(*orte_svc_base_component_init_fn_t) (
    bool* allow_threads,
    bool* use_threads);

/*
 * Structure for svc v1.0.0 components
 * Chained to ORTE v1.0.0
 */
struct orte_svc_base_component_t {
    mca_base_component_t svc_version;
    mca_base_component_data_1_0_0_t svc_data;

    /* Initialization functions */
    orte_svc_base_component_init_fn_t svc_init;
};
typedef struct orte_svc_base_component_t orte_svc_base_component_t;


/**************************************************************************
 * Service module
 **************************************************************************/

typedef int (*orte_svc_base_module_finalize_fn_t) (void);

struct orte_svc_base_module_t {
    orte_svc_base_module_finalize_fn_t finalize;
};
typedef struct orte_svc_base_module_t orte_svc_base_module_t;


/*
 * Macro for use in modules that are of type svc v1.0.0
 */
#define ORTE_SVC_BASE_VERSION_1_0_0 \
  /* svc v1.0 is chained to ORTE v1.0 */ \
  MCA_BASE_VERSION_1_0_0, \
  /* svc v1.0 */ \
  "svc", 1, 0, 0

#endif                          /* ORTE_SVC_H */

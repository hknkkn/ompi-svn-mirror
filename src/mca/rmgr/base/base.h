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
 */

#ifndef ORTE_RMGR_BASE_H
#define ORTE_RMGR_BASE_H

/*
 * includes
 */
#include "orte_config.h"
#include "include/orte_constants.h"
#include "include/orte_types.h"

#include "class/ompi_list.h"
#include "mca/mca.h"
#include "mca/rmgr/rmgr.h"


/*
 * Global functions for MCA overall collective open and close
 */
#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

/*
 * Internal definitions
 */

/*
 * function definitions
 */
OMPI_DECLSPEC int orte_rmgr_base_open(void);
OMPI_DECLSPEC int orte_rmgr_base_select(bool *allow_multi_user_threads,
			                               bool *have_hidden_threads);
OMPI_DECLSPEC int orte_rmgr_base_close(void);

OMPI_DECLSPEC int orte_rmgr_base_put_app_context(
    orte_jobid_t jobid,
    orte_app_context_t** app_context,
    size_t num_context);

OMPI_DECLSPEC int orte_rmgr_base_get_app_context(
    orte_jobid_t jobid,
    orte_app_context_t*** app_context,
    size_t* num_context);

/*
 * Base functions that are common to all implementations - can be overridden
 */
int orte_rmgr_base_create_not_available(
    orte_app_context_t** app_context, 
    size_t num_context, 
    orte_jobid_t* jobid);
int orte_rmgr_base_query_not_available(void);
int orte_rmgr_base_allocate_not_available(orte_jobid_t);
int orte_rmgr_base_deallocate_not_available(orte_jobid_t);
int orte_rmgr_base_map_not_available(orte_jobid_t);
int orte_rmgr_base_launch_not_available(orte_jobid_t);
int orte_rmgr_base_spawn_not_available(
    orte_app_context_t** app_context, 
    size_t num_context, 
    orte_jobid_t* jobid);
int orte_rmgr_base_finalize_not_available(void);

/*
 * globals that might be needed
 */

typedef struct orte_rmgr_base_t {
    int rmgr_output;
    ompi_list_t rmgr_components;
} orte_rmgr_base_t;

OMPI_DECLSPEC extern orte_rmgr_base_t orte_rmgr_base;

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif
#endif

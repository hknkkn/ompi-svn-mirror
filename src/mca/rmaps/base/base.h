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

#ifndef ORTE_MCA_RMAPS_BASE_H
#define ORTE_MCA_RMAPS_BASE_H

/*
 * includes
 */
#include "orte_config.h"
#include "include/orte_constants.h"

#include "class/ompi_list.h"
#include "mca/mca.h"
#include "mca/ns/ns_types.h"

#include "mca/rmaps/rmaps.h"


/*
 * Global functions for MCA overall collective open and close
 */
#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

/*
 * Internal definitions
 */

struct orte_rmaps_base_selected_t {
    ompi_list_item_t super;
    orte_rmaps_base_component_t *component;
    orte_rmaps_base_module_t* module;
};
typedef struct orte_rmaps_base_selected_t orte_rmaps_base_selected_t;
OMPI_DECLSPEC OBJ_CLASS_DECLARATION(orte_rmaps_base_selected_t);
                                                                                                        

/*
 * function definitions
 */
OMPI_DECLSPEC    int orte_rmaps_base_open(void);
OMPI_DECLSPEC    int orte_rmaps_base_select(bool *allow_multi_user_threads,
			                                 bool *have_hidden_threads);
OMPI_DECLSPEC    int orte_rmaps_base_close(void);
OMPI_DECLSPEC    int orte_rmaps_base_map(orte_jobid_t);

/*
 * globals that might be needed
 */

typedef struct orte_rmaps_base_t {
    int rmaps_output;
    ompi_list_t rmaps_components;
    ompi_list_t rmaps_selected;
} orte_rmaps_base_t;

OMPI_DECLSPEC extern orte_rmaps_base_t orte_rmaps_base;

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif
#endif

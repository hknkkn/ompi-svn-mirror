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

#ifndef ORTE_MCA_RAS_BASE_H
#define ORTE_MCA_RAS_BASE_H

/*
 * includes
 */
#include "orte_config.h"
#include "include/orte_constants.h"

#include "class/ompi_list.h"

#include "mca/ras/ras.h"


/*
 * Global functions for MCA overall collective open and close
 */
#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

/*
 * Internal definitions
 */

struct orte_ras_base_selected_t {
    ompi_list_item_t super;
    orte_ras_base_component_t *component;
    orte_ras_base_module_t* module;
};
typedef struct orte_ras_base_selected_t orte_ras_base_selected_t;
OMPI_DECLSPEC OBJ_CLASS_DECLARATION(orte_ras_base_selected_t);

/*
 * function definitions
 */
OMPI_DECLSPEC int orte_ras_base_open(void);
OMPI_DECLSPEC int orte_ras_base_select(bool *allow_multi_user_threads,
			                           bool *have_hidden_threads);
OMPI_DECLSPEC int orte_ras_base_close(void);


/*
 * globals that might be needed
 */


typedef struct orte_ras_base_t {
    int ras_output;
    ompi_list_t ras_components;
    ompi_list_t ras_selected;
} orte_ras_base_t;
 
OMPI_DECLSPEC extern orte_ras_base_t orte_ras_base;


/*
 * external API functions will be documented in the mca/ns/ns.h file
 */

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif
#endif

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

#ifndef MCA_PLS_BASE_H
#define MCA_PLS_BASE_H

/*
 * includes
 */
#include "ompi_config.h"
#include "mca/mca.h"
#include "mca/pls/pls.h"


#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

/*
 * Internal definitions
 */
struct orte_pls_base_available_t {
    ompi_list_item_t super;
    const orte_pls_base_component_t *component;
    const orte_pls_base_module_t* module;

    bool allow_multi_user_threads;
    bool have_hidden_threads;
    int priority;
};
typedef struct orte_pls_base_available_t orte_pls_base_available_t;
OMPI_DECLSPEC OBJ_CLASS_DECLARATION(orte_pls_base_available_t);


/*
 * Global functions for MCA overall collective open and close
 */

OMPI_DECLSPEC int orte_pls_base_open(void);
OMPI_DECLSPEC int orte_pls_base_select(bool *allow_multi_user_threads,
                                       bool *have_hidden_threads);
OMPI_DECLSPEC int orte_pls_base_close(void);
OMPI_DECLSPEC int orte_pls_base_launch(orte_jobid_t);
OMPI_DECLSPEC int orte_pls_base_get_argv(orte_jobid_t jobid, char ***argv);

/*
 * globals that might be needed
 */

typedef struct orte_pls_base_t {
   int pls_output;
   ompi_list_t pls_components;
   ompi_list_t pls_available;
} orte_pls_base_t;

OMPI_DECLSPEC extern orte_pls_base_t orte_pls_base;

/*
 * external API functions will be documented in the mca/pls/pls.h file
 */

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif
#endif

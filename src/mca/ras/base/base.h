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

#ifndef ORTE-MCA_RAS_BASE_H
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
/*

/*
 * function definitions
 */
OMPI_DECLSPEC    int orte_mca_ras_base_open(void);
OMPI_DECLSPEC    int orte_mca_ras_base_select(bool *allow_multi_user_threads,
			                                 bool *have_hidden_threads);
OMPI_DECLSPEC    int orte_mca_ras_base_close(void);

    /*
     * Base functions that are common to all implementations - can be overridden
     */
int orte_mca_ras_base_allocate_not_available(void);

int orte_mca_ras_base_deallocate_not_available(void);

/*
 * globals that might be needed
 */

OMPI_DECLSPEC extern int orte_mca_ras_base_output;
OMPI_DECLSPEC extern bool orte_mca_ras_base_selected;
OMPI_DECLSPEC extern ompi_list_t orte_mca_ras_base_components_available;
OMPI_DECLSPEC extern orte_mca_ras_base_component_t orte_mca_ras_base_selected_component;

/*
 * external API functions will be documented in the mca/ns/ns.h file
 */

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif
#endif

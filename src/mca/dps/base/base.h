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

#ifndef MCA_DPS_BASE_H
#define MCA_DPS_BASE_H

/*
 * includes
 */
#include "ompi_config.h"
#include "class/ompi_list.h"
#include "mca/mca.h"
#include "mca/dps/dps.h"


/*
 * Global functions for MCA overall collective open and close
 */
#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

OMPI_DECLSPEC    int mca_dps_base_open(void);
OMPI_DECLSPEC    int mca_dps_base_select(bool *allow_multi_user_threads,
			                            bool *have_hidden_threads);
OMPI_DECLSPEC    int mca_dps_base_close(void);

    /*
     * Base functions that are common to all implementations - can be overridden
     */


/*
 * globals that might be needed
 */

OMPI_DECLSPEC extern int mca_dps_base_output;
OMPI_DECLSPEC extern mca_dps_base_module_t orte_dps;  /* holds selected module's function pointers */
OMPI_DECLSPEC extern bool mca_dps_base_selected;
OMPI_DECLSPEC extern ompi_list_t mca_dps_base_components_available;
OMPI_DECLSPEC extern mca_dps_base_component_t mca_dps_base_selected_component;

/*
 * external API functions will be documented in the mca/dps/dps.h file
 */

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif
#endif

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

    /**
     * Struct to hold data globale to the pls framework
     */
    typedef struct orte_pls_base_t {
        int pls_output;
        ompi_list_t pls_opened;
        ompi_list_t pls_available;
    } orte_pls_base_t;
    
    /**
     * Global instance of pls-wide framework data
     */
    OMPI_DECLSPEC extern orte_pls_base_t orte_pls_base;

    /**
     * pls component/module/priority tuple
     */
    struct orte_pls_base_cmp_t {
        /** Base object */
        ompi_list_item_t super;
        /** pls component */
        orte_pls_base_component_t *component;
        /** pls module */
        orte_pls_base_module_t* module;
        /** This component's priority */
        int priority;
    };
    /** Convenience typedef */
    typedef struct orte_pls_base_cmp_t orte_pls_base_cmp_t;
    /** Class declaration */
    OMPI_DECLSPEC OBJ_CLASS_DECLARATION(orte_pls_base_cmp_t);

    /*
     * Global functions for MCA overall collective open and close
     */

    /**
     * Open the pls framework
     */
    OMPI_DECLSPEC int orte_pls_base_open(void);
    /**
     * Select a pls module
     */
    OMPI_DECLSPEC orte_pls_base_module_t *orte_pls_base_select(char *preferred);
    /**
     * Close the pls framework
     */
    OMPI_DECLSPEC int orte_pls_base_close(void);

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif
#endif

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

#ifndef ORTE_PLS_FORK_EXPORT_H
#define ORTE_PLS_FORK_EXPORT_H

#include "ompi_config.h"

#include "mpi.h"
#include "mca/mca.h"
#include "mca/pls/pls.h"

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

    /*
     * Globally exported variable
     */
    
    OMPI_COMP_EXPORT extern const orte_pls_base_component_1_0_0_t 
        orte_pls_fork_component;
    OMPI_COMP_EXPORT extern const orte_pls_base_module_1_0_0_t
        orte_pls_fork_module;
    OMPI_COMP_EXPORT extern int orte_pls_fork_param_priorty;

    /* Component functions */

    const struct orte_pls_base_module_1_0_0_t *
        orte_pls_fork_init(bool *allow_multi_user_threads,
                           bool *have_hidden_threads, int *priority);

    /* Module functions */

    int orte_pls_fork_launch(orte_jobid_t jobid);
    int orte_pls_fork_finalize(void);

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif
#endif /* ORTE_PLS_FORK_EXPORT_H */

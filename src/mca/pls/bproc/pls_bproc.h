/* -*- C -*-
 * 
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
 *
 *
 */

#ifndef MCA_PLS_BPROC_H_
#define MCA_PLS_BPROC_H_

#include "ompi_config.h"

#include "mca/pls/base/base.h"

#include <sys/bproc.h>

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

    /*
     * Module open / close
     */
    int mca_pls_bproc_component_open(void);
    int mca_pls_bproc_component_close(void);

    /*
     * Startup / Shutdown
     */
    struct mca_pls_base_module_1_0_0_t* mca_pls_bproc_init(int *priority, 
							   bool have_threads,
							   int constraints);
    int mca_pls_bproc_finalize(struct mca_pls_base_module_1_0_0_t* me);

    /*
     * Interface
     */
    int mca_pls_bproc_spawn_procs(struct mca_pls_base_module_1_0_0_t* me,
                                  mca_ns_base_jobid_t jobid);


#if defined(c_plusplus) || defined(__cplusplus)
}
#endif
#endif /* MCA_PCM_BPROCx_H_ */

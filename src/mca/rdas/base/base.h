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
 */

#ifndef MCA_RDAS_BASE_H_
#define MCA_RDAS_BASE_H_

#include "ompi_config.h"

#include "mca/mca.h"
#include "mca/rdas/rdas.h"


/*
 * Forward define 
 */

/*
 * Global functions for the RDAS
 */

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif
OMPI_DECLSPEC  int mca_rdas_base_open(void);
OMPI_DECLSPEC int mca_rdas_base_select(const char *active_pcm,
                          mca_rdas_base_module_t **selected, 
                          bool have_threads);
OMPI_DECLSPEC int mca_rdas_base_close(void);

/*
 * Globals
 */
OMPI_DECLSPEC extern int mca_rdas_base_output;
OMPI_DECLSPEC extern ompi_list_t mca_rdas_base_components_available;
OMPI_DECLSPEC extern mca_rdas_base_module_t ompi_rdas;  /* holds selected module's function pointers */

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif

#endif /* MCA_RDAS_BASE_H */

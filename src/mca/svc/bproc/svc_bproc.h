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

#ifndef ORTE_SVC_BPROC_H_
#define ORTE_SVC_BPROC_H_

#include "ompi_config.h"
#include "mca/rml/rml.h"
#include "mca/svc/base/base.h"
#include <sys/bproc.h>

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

/**
 * Exported functions
 */

int orte_svc_bproc_component_open(void);
int orte_svc_bproc_component_close(void);
orte_svc_base_module_t* orte_svc_bproc_component_init(bool* allow_threads, bool* have_threads);
int orte_svc_bproc_module_finalize(void);
void orte_svc_bproc_module_recv(
    int status,
    orte_process_name_t* peer,
    orte_buffer_t* buffer,
    orte_rml_tag_t tag,
    void* cbdata);

/**
 * SVC Component
 */
struct orte_svc_bproc_component_t {
    orte_svc_base_component_t super;
    int debug;
};
typedef struct orte_svc_bproc_component_t orte_svc_bproc_component_t;

OMPI_COMP_EXPORT extern orte_svc_bproc_component_t orte_svc_bproc_component;
OMPI_COMP_EXPORT extern orte_svc_base_module_t orte_svc_bproc_module;

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif
#endif /* ORTE_SVC_BPROC_H_ */

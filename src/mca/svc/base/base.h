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
/** @file **/

#ifndef ORTE_SVC_BASE_H
#define ORTE_SVC_BASE_H

#include "ompi_config.h"
#include "mca/mca.h"
#include "mca/svc/svc.h"

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

/*
 * Internal types.
 */

struct orte_svc_base_selected_t {
  ompi_list_item_t super;
  orte_svc_base_component_t *component;
  orte_svc_base_module_t *module;
};
typedef struct orte_svc_base_selected_t orte_svc_base_selected_t;

ORTE_DECLSPEC OBJ_CLASS_DECLARATION(orte_svc_base_selected_t);

/*
 * Global functions for the SVC
 */

ORTE_DECLSPEC int orte_svc_base_open(void);
ORTE_DECLSPEC int orte_svc_base_select(bool* allow_threads, bool* use_threads);
ORTE_DECLSPEC int orte_svc_base_close(void);

/*
 * Globals
 */

typedef struct orte_svc_base_t {
    int svc_output;
    ompi_list_t svc_components;
    ompi_list_t svc_modules;
} orte_svc_base_t;

ORTE_DECLSPEC extern orte_svc_base_t orte_svc_base;

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif
#endif /* ORTE_SVC_BASE_H */

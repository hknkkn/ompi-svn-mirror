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

#ifndef ORTE_RMGR_TYPES_H
#define ORTE_RMGR_TYPES_H

typedef struct {
    ompi_object_t super;
    char *application;
    int32_t num_procs;
} orte_rmgr_app_info_t;

OBJ_CLASS_DECLARATION(orte_rmgr_app_info_t);


typedef struct {
    ompi_object_t super;
    int32_t argc;
    char **argv;
    int32_t num_enviro;
    char **enviro;
} orte_rmgr_app_context_t;

OBJ_CLASS_DECLARATION(orte_rmgr_app_context_t);


/*
 * REGISTRY KEY NAMES FOR COMMON DATA
 */
#define ORTE_RMGR_APP_INFO      "orte-rmgr-app-info"
#define ORTE_RMGR_APP_CONTEXT   "orte-rmgr-app-context"
#define ORTE_RMGR_APP_LOC       "orte-rmgr-app-location"
#define ORTE_RMGR_LAUNCHER      "orte-rmgr-launcher"


#endif

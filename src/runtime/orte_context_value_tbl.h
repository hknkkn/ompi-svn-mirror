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

/**
 * @file
 *
 * Enviro and command line options for processes in the the Open Run Time Environment.
 *
 */

#ifndef ORTE_CONTEXT_VALUE_TBL_H
#define ORTE_CONTEXT_VALUE_TBL_H

#include "orte_config.h"

#include "include/orte_types.h"

typedef void (*orte_context_cb_fn_t)(void);

typedef struct {
    struct {
        char *prime;
        char *second;
        char *third;
    } name;
    char *cmd_line_name;
    int num_params;
    orte_data_type_t type;
    void *dest;
    void *def;  /* default value */
    orte_context_cb_fn_t cbfunc;
} orte_context_value_names_t;

#endif

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
 *
 */
#include "orte_config.h"

#include "plsnds_fns.h"
#include "plsnds.h"

/**
 * globals
 */
bool orte_plsnds_debug;

orte_plsnds_t orte_plsnds[] = {
	{"ENV", orte_plsnds_env},
    {"RSH", orte_plsnds_env}
};

int orte_plsnds_max = 2;


/*
 * open function
 */
int orte_plsnds_open(void)
{
    char *enviro_val;
    
    enviro_val = getenv("ORTE_plsnds_debug");
    if (NULL != enviro_val) {  /* debug requested */
        orte_plsnds_debug = true;
    } else {
        orte_plsnds_debug = false;
    }

    return ORTE_SUCCESS;
}

/*
 * close function
 */
int orte_plsnds_close(void)
{
    /* no idea what this would do right now - include it for completeness */
    return ORTE_SUCCESS;
}

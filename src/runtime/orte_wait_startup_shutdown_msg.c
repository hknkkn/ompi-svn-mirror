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

#include "orte_config.h"

#include "util/output.h"

#include "runtime/runtime.h"
#include "mca/gpr/gpr.h"
#include "mca/rml/rml.h"


/*
 * Main functions
 */
int orte_wait_startup_msg(void)
{

    return orte_rml.xcast(NULL, NULL, 0, NULL, orte_gpr.decode_startup_msg);
}

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

#include "include/constants.h"

#include "runtime/runtime.h"
#include "util/output.h"
#include "mca/rml/base/base.h"

int orte_init_cleanup(bool *allow_multi_user_threads, bool *have_hidden_threads)
{
    int ret;
    bool user_threads, hidden_threads;

    /*
     * Call back into RML to allow do any final initialization
     * (e.g. put contact info in registry).
     */
    user_threads = true;
    hidden_threads = false;
    if (OMPI_SUCCESS != (ret = orte_rml_base_select(&user_threads, &hidden_threads))) {
       ompi_output(0, "orte_init: failed in orte_rml_base_select()\n");
       return ret;
    }
    *allow_multi_user_threads &= user_threads;
    *have_hidden_threads |= hidden_threads;

    return ORTE_SUCCESS;
}

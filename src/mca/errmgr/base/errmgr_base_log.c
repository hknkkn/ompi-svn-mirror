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


#include "orte_config.h"

#include "util/output.h"
#include "util/proc_info.h"
#include "mca/ns/ns_types.h"

#include "mca/errmgr/base/base.h"


void orte_errmgr_base_log(char *msg, char *filename, int line)
{
    if (NULL == orte_process_info.my_name) {
        ompi_output(0, "[NO-NAME] ORTE_ERROR_LOG: %s in file %s at line %d",
                                msg, filename, line);
    } else {
        ompi_output(0, "[%d,%d,%d] ORTE_ERROR_LOG: %s in file %s at line %d",
                ORTE_NAME_ARGS(*(orte_process_info.my_name)), msg, filename, line);
    }
}

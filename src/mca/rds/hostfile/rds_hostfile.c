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
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "include/orte_constants.h"
#include "rds_hostfile.h"


static int orte_rds_hostfile_query(void)
{
    return ORTE_SUCCESS;
}


static int orte_rds_hostfile_finalize(void)
{
    return ORTE_SUCCESS;
}


orte_rds_base_module_t orte_rds_hostfile_module = {
    orte_rds_hostfile_query,
    orte_rds_hostfile_finalize
};


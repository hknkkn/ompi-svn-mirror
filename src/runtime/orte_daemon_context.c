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
 * Parse enviro and command line context for daemons. This program MUST be called early
 * in orte_init, but after the MCA is opened !!!
 *
 */
#include "orte_config.h"

#include "mca/errmgr/errmgr.h"
#include "util/proc_info.h"

#include "runtime/orte_context_value_tbl.h"
#include "runtime/runtime.h"

orte_context_value_names_t orte_daemon_context_tbl[] = {
    {{"universe", "scope", NULL}, "scope", 1, ORTE_STRING, (void*)&(orte_universe_info.scope), "exclusive"},
    {{"universe", "persistence", NULL}, "persistent", 0, ORTE_INT, (void*)&(orte_universe_info.persistence), (void*)1},
    {{"universe", "console", NULL}, "console", 0, ORTE_INT, (void*)&(orte_universe_info.console), (void*)1},
    {{"universe", "script", NULL}, "script", 1, ORTE_STRING, (void*)&(orte_universe_info.scriptfile), NULL},
    {{"bootproxy", NULL, NULL}, "bootproxy", 0, ORTE_INT, (void*)&(orte_universe_info.bootproxy), (void*)1},
    {{NULL, NULL, NULL}, NULL, ORTE_NULL, NULL} /* SIGNIFIES END OF ARRAY */
};

int orte_parse_daemon_context(ompi_cmd_line_t *cmd_line, int argc, char **argv)
{
    int rc;
    
    if (ORTE_SUCCESS != (rc = orte_parse_context(orte_daemon_context_tbl, cmd_line, argc, argv))) {
        ORTE_ERROR_LOG(rc);
    }
    return rc;
}

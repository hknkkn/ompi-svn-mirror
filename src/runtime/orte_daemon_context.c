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
    {{"universe", "persistence", NULL}, "persistent", 0, ORTE_BOOL, (void*)&(orte_universe_info.persistence), (void*)false},
    {{"universe", "console", NULL}, "console", 0, ORTE_BOOL, (void*)&(orte_universe_info.console), (void*)false},
    {{"universe", "script", NULL}, "script", 1, ORTE_STRING, (void*)&(orte_universe_info.scriptfile), NULL},
    {{"orte", "bootproxy", NULL}, "bootproxy", 0, ORTE_INT, (void*)&(orte_universe_info.bootproxy), (void*)false},
    {{"orte", "name", NULL}, "name", 1, ORTE_NAME, (void*)&(orte_process_info.my_name), NULL},
    {{"gpr", "replica", "uri"}, "gprreplica", 1, ORTE_STRING, (void*)&(orte_process_info.gpr_replica_uri), NULL},
    {{"ns", "replica", "uri"}, "nsreplica", 1, ORTE_STRING, (void*)&(orte_process_info.ns_replica_uri), NULL},
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

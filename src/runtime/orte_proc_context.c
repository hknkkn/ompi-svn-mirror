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
 * Parse enviro and command line context for processes. This program MUST be called early
 * in orte_init, but after the MCA is opened !!!
 *
 */
#include "orte_config.h"

#include "mca/errmgr/errmgr.h"
#include "util/proc_info.h"

#include "runtime/orte_context_value_tbl.h"
#include "runtime/runtime.h"

orte_context_value_names_t orte_proc_context_tbl[] = {
    {{"seed", NULL, NULL}, "seed", 0, ORTE_BOOL, (void*)&(orte_process_info.seed), (void*)false},
    {{"universe", "path", NULL}, "universe", 1, ORTE_STRING, (void*)&(orte_universe_info.path), NULL},
    {{"universe", "name", NULL}, NULL, 0, ORTE_STRING, (void*)&(orte_universe_info.name), "default-universe"},
    {{"universe", "host", NULL}, NULL, 0, ORTE_STRING, (void*)&(orte_universe_info.host), NULL},
    {{"universe", "host", "uid"}, NULL, 0, ORTE_STRING, (void*)&(orte_universe_info.uid), NULL},
    {{"universe", "uri", NULL}, "universe_uri", 1, ORTE_STRING, (void*)&(orte_universe_info.seed_uri), NULL},
    {{"tmpdir", "base", NULL}, "tmpdir_base", 1, ORTE_STRING, (void*)&(orte_process_info.tmpdir_base), "/tmp"},
    {{"gpr", "replica", "uri"}, "gprreplica", 1, ORTE_STRING, (void*)&(orte_process_info.gpr_replica_uri), NULL},
    {{"ns", "replica", "uri"}, "nsreplica", 1, ORTE_STRING, (void*)&(orte_process_info.ns_replica_uri), NULL},
    {{NULL, NULL, NULL}, NULL, 0, ORTE_NULL, NULL} /* SIGNIFIES END OF ARRAY */
};

int orte_parse_proc_context(ompi_cmd_line_t *cmd_line, int argc, char **argv)
{
    int rc;
    
    if (ORTE_SUCCESS != (rc = orte_parse_context(orte_proc_context_tbl, cmd_line, argc, argv))) {
        ORTE_ERROR_LOG(rc);
    }
    
    return rc;
}

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
 * Parse command line options for the Open MPI Run Time Environment
 */
#include "ompi_config.h"

#include <string.h>

#include "mca/ns/base/base.h"

#include "util/output.h"
#include "util/cmd_line.h"
#include "util/sys_info.h"
#include "util/proc_info.h"

#include "runtime/runtime.h"

int orte_parse_daemon_cmd_line(ompi_cmd_line_t *cmd_line)
{

    /* see if I'm the seed */
    if (ompi_cmd_line_is_taken(cmd_line, "seed") &&
	false == orte_process_info.seed) {
	orte_process_info.seed = true;
	setenv("OMPI_universe_seed", "1", 1);
    }

    /* see if seed contact info is provided */
    if (ompi_cmd_line_is_taken(cmd_line, "seedcontact")) {
	if (NULL == ompi_cmd_line_get_param(cmd_line, "seedcontact", 0, 0)) {
	    fprintf(stderr, "error retrieving seed contact info - please report error to bugs@open-mpi.org\n");
	    exit(1);
	}
	if (NULL != orte_universe_info.seed_uri) {  /* overwrite it */
	    free(orte_universe_info.seed_uri);
	    orte_universe_info.seed_uri = NULL;
	}
	orte_universe_info.seed_uri = strdup(ompi_cmd_line_get_param(cmd_line, "seedcontact", 0, 0));
	setenv("OMPI_universe_contact", orte_universe_info.seed_uri, 1);
    }

    /* see if I'm to be a bootproxy */
    if (ompi_cmd_line_is_taken(cmd_line, "bootproxy")) {
         setenv("OMPI_orte_bootproxy", "1", 1);
    }

    /* get desired universe scope, if specified */
    if (ompi_cmd_line_is_taken(cmd_line, "scope")) {
	if (NULL == ompi_cmd_line_get_param(cmd_line, "scope", 0, 0)) {
	    fprintf(stderr, "error retrieving universe scope - please report error to bugs@open-mpi.org\n");
	    exit(1);
	}
	if (NULL != orte_universe_info.scope) {
	    free(orte_universe_info.scope);
	    orte_universe_info.scope = NULL;
	}
	orte_universe_info.scope = strdup(ompi_cmd_line_get_param(cmd_line, "scope", 0, 0));
	setenv("OMPI_universe_scope", orte_universe_info.scope, 1);
    }

    /* find out if persistent */
    if (ompi_cmd_line_is_taken(cmd_line, "persistent")) {
	setenv("OMPI_universe_persistent", "1", 1);
	orte_universe_info.persistence = true;
    }

    /* find out if we desire a console */
    if (ompi_cmd_line_is_taken(cmd_line, "console")) {
	setenv("OMPI_universe_console", "1", 1);
	orte_universe_info.console = true;
	orte_universe_info.console_connected = false;
    }

    /* find out if script is to be executed */
    if (ompi_cmd_line_is_taken(cmd_line, "script")) {
	if (NULL == ompi_cmd_line_get_param(cmd_line, "script", 0, 0)) {
	    fprintf(stderr, "error retrieving script file name - please report error to bugs@open-mpi.org\n");
	    exit(1);
	}
	if (NULL != orte_universe_info.scriptfile) {
	    free(orte_universe_info.scriptfile);
	    orte_universe_info.scriptfile = NULL;
	}
	orte_universe_info.scriptfile = strdup(ompi_cmd_line_get_param(cmd_line, "script", 0, 0));
	setenv("OMPI_universe_script", orte_universe_info.scriptfile, 1);
    }

}

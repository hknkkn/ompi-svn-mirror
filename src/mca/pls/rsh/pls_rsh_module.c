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
 *
 * These symbols are in a file by themselves to provide nice linker
 * semantics.  Since linkers generally pull in symbols by object
 * files, keeping these symbols as the only symbols in this file
 * prevents utility programs such as "ompi_info" from having to import
 * entire components just to query their version and parameters.
 */

#include "ompi_config.h"
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "include/orte_constants.h"
#include "util/argv.h"
#include "util/output.h"
#include "mca/ns/ns.h"
#include "mca/pls/pls.h"
#include "mca/rml/rml.h"
#include "mca/ras/base/ras_base_node.h"
#include "pls_rsh.h"

#define NUM_CONCURRENT 128



orte_pls_base_module_1_0_0_t orte_pls_rsh_module = {
    orte_pls_rsh_launch,
    orte_pls_rsh_terminate_job,
    orte_pls_rsh_terminate_proc,
    orte_pls_rsh_finalize
};



static int orte_pls_rsh_dump_params(int p_stdin, int p_stdout)
{
    return ORTE_SUCCESS;
}


int orte_pls_rsh_launch(orte_jobid_t jobid)
{
    ompi_list_t nodes;
    ompi_list_item_t* item;
    size_t num_nodes;
    orte_vpid_t vpid_start;
    int node_name_index;
    int proc_name_index;
    char* jobid_string;
    char** argv;
    int argc;
    int rc;

    /* query the list of nodes allocated to the job - don't need the entire
     * mapping - as the daemon/proxy is responsibe for determining the apps
     * to launch on each node.
     */
    OBJ_CONSTRUCT(&nodes, ompi_list_t);
    rc = orte_ras_base_node_query_alloc(&nodes, jobid);
    if(ORTE_SUCCESS != rc) {
        goto cleanup;
    }

    /*
     * Allocate a range of vpids for the daemons.
     */

    num_nodes = ompi_list_get_size(&nodes);
    if(num_nodes == 0) {
        return ORTE_ERR_BAD_PARAM;
    }
    rc = orte_ns.reserve_range(0, num_nodes, &vpid_start);
    if(ORTE_SUCCESS != rc) {
        goto cleanup;
    }
    rc = orte_ns.convert_jobid_to_string(&jobid_string, jobid);
    if(ORTE_SUCCESS != rc) {
        goto cleanup;
    }

    /*
     * Build argv/env arrays.
     */
    argv = ompi_argv_copy(mca_pls_rsh_component.argv);
    argc = mca_pls_rsh_component.argc;
    node_name_index = argc;
    ompi_argv_append(&argc, &argv, "");  /* placeholder for node name */

    ompi_argv_append(&argc, &argv, "/bin/sh");
    ompi_argv_append(&argc, &argv, "-lc");
    ompi_argv_append(&argc, &argv, "\"");

    /* application */
    ompi_argv_append(&argc, &argv, "orted");
    ompi_argv_append(&argc, &argv, "-bootproxy");
    ompi_argv_append(&argc, &argv, jobid_string);
    ompi_argv_append(&argc, &argv, "-name");
    proc_name_index = argc;
    ompi_argv_append(&argc, &argv, "");

    /* setup ns contact info */
    ompi_argv_append(&argc, &argv, "-nsreplica");
    if(NULL != orte_process_info.ns_replica_uri) {
        ompi_argv_append(&argc, &argv, orte_process_info.ns_replica_uri);
    } else {
        ompi_argv_append(&argc, &argv, orte_rml.get_uri());
    }

    /* setup gpr contact info */
    ompi_argv_append(&argc, &argv, "-gprreplica");
    if(NULL != orte_process_info.gpr_replica_uri) {
        ompi_argv_append(&argc, &argv, orte_process_info.gpr_replica_uri);
    } else {
        ompi_argv_append(&argc, &argv, orte_rml.get_uri());
    }
    ompi_argv_append(&argc, &argv, "\"");

    /*
     * Iterate through each of the nodes and spin
     * up a daemon.
     */

    item = ompi_list_get_first(&nodes);
    while(item != ompi_list_get_end(&nodes)) {
        pid_t pids[NUM_CONCURRENT];
        int num_started = 0;
        int num_failed = 0;
        int i;

        /* start more than one at a time, however must limit the
         * number of open sessions as each consumes resources
        */
        for(i=0; i<NUM_CONCURRENT && item != ompi_list_get_end(&nodes); i++, item = ompi_list_get_next(&nodes)) {
            orte_ras_base_node_t* node = (orte_ras_base_node_t*)item;
            int p_stdin[2];
            int p_stdout[2];

            /* create a pipe to connect to stdin/stdout of the daemon */
            if(pipe(p_stdin) < 0 ||
               pipe(p_stdout) < 0) {
                rc = ORTE_ERR_OUT_OF_RESOURCE;
                goto cleanup;
            }

            /* fork a child to exec the rsh/ssh session */
            pids[i] = fork();
            if(pids[i] < 0) {
                rc = ORTE_ERR_OUT_OF_RESOURCE;
                goto cleanup;
            }

            /* child */
            if(pids[i] == 0) {

                orte_process_name_t* name;
                char* name_string;
                rc = orte_ns.create_process_name(&name, node->node_cellid, 0, vpid_start);
                if(ORTE_SUCCESS != rc) {
                    ompi_output(0, "orte_pls_rsh: unable to create process name");
                    exit(-1);
                }
                rc = orte_ns.get_proc_name_string(&name_string, name);
                if(ORTE_SUCCESS != rc) {
                    ompi_output(0, "orte_pls_rsh: unable to create process name");
                    exit(-1);
                }

                /* setup stdin/stdout/stderr */
                close(p_stdin[1]);
                close(p_stdout[0]);
                dup2(p_stdin[0], 0);
                dup2(p_stdout[1], 1);
                dup2(p_stdout[1], 2);

                /* exec the daemon */
                argv[node_name_index] = node->node_name;
                argv[proc_name_index] = name_string;
                execv(mca_pls_rsh_component.path, argv);
                ompi_output(0, "orte_pls_rsh: execv failed with errno=%d\n", errno);
                exit(-1);

            /* parent - dump context to the pipe */
            } else {
                close(p_stdin[0]);
                close(p_stdout[1]);
                rc = orte_pls_rsh_dump_params(p_stdin[1], p_stdout[0]);
                close(p_stdin[1]);
                close(p_stdout[0]);
                if(rc != ORTE_SUCCESS) {
                    goto cleanup;
                }
            }
            num_started++;
            vpid_start++;
        }

        /* wait on these to complete */
        for(i=0; i<num_started; i++) {
            int status;
            rc = waitpid(pids[i], &status, 0);
            if(rc != 0) {
                num_failed++;
            } else if(WIFEXITED(status) == 0 || WEXITSTATUS(status) != 0) {
                num_failed++;
            }
        }
        if(num_failed != 0) {
            rc = ORTE_ERROR;
            goto cleanup;
        }
    }

cleanup:
    while(NULL != (item = ompi_list_remove_first(&nodes))) {
        OBJ_RELEASE(item);
    }
    OBJ_DESTRUCT(&nodes);
    return rc;
}

int orte_pls_rsh_terminate_job(orte_jobid_t jobid)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_pls_rsh_terminate_proc(const orte_process_name_t* proc)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_pls_rsh_finalize(void)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}



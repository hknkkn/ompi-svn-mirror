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
#include "util/sys_info.h"
#include "runtime/orte_wait.h"
#include "mca/errmgr/errmgr.h"
#include "mca/iof/iof.h"
#include "mca/ns/ns.h"
#include "mca/ns/base/ns_base_nds.h"
#include "mca/pls/pls.h"
#include "mca/pls/base/base.h"
#include "mca/rml/rml.h"
#include "mca/rmaps/base/base.h"
#include "mca/rmaps/base/rmaps_base_map.h"
#include "mca/soh/soh.h"
#include "pls_fork.h"



orte_pls_base_module_1_0_0_t orte_pls_fork_module = {
    orte_pls_fork_launch,
    orte_pls_fork_terminate_job,
    orte_pls_fork_terminate_proc,
    orte_pls_fork_finalize
};


/*
 *  Wait for a callback indicating the child has completed.
 */
                                                                                                                  
static void orte_pls_fork_wait_proc(pid_t pid, int status, void* cbdata)
{
    orte_rmaps_base_proc_t* proc = (orte_rmaps_base_proc_t*)cbdata;
    int rc;

    /* set the state of this process */
    rc = orte_soh.set_proc_soh(&proc->proc_name, ORTE_PROC_STATE_TERMINATED, status);
    if(ORTE_SUCCESS != rc) {
        ORTE_ERROR_LOG(rc);
    }
    OBJ_RELEASE(proc);
                                                                                                                  
    /* release any waiting threads */
    OMPI_THREAD_LOCK(&mca_pls_fork_component.lock);
    mca_pls_fork_component.num_children--;
    ompi_condition_signal(&mca_pls_fork_component.cond);
    OMPI_THREAD_UNLOCK(&mca_pls_fork_component.lock);
}


static int orte_pls_fork_proc(orte_app_context_t* context, orte_rmaps_base_proc_t* proc)
{
    pid_t pid;
    int p_stdout[2];
    int p_stderr[2];
    int rc;

    if(pipe(p_stdout) < 0 ||
       pipe(p_stderr) < 0) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }

    pid = fork();
    if(pid < 0) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }

    if(pid == 0) {

#if 0
        /* set the process name in the environment */
        mca_ns_nds_env_put(&proc->proc_name,...);
#endif
        if(chdir(context->cwd) != 0) {
            ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
        }
        close(p_stdout[0]);
        close(p_stderr[0]);
        if(p_stdout[1] != STDOUT_FILENO) {
            dup2(p_stdout[1], STDOUT_FILENO);
            close(p_stdout[1]);
        }
        if(p_stderr[1] != STDERR_FILENO) {
            dup2(p_stderr[1], STDERR_FILENO);
            close(p_stderr[1]);
        }
        execve(context->app, context->argv, context->env);
        ompi_output(0, "orte_pls_fork: execv failed with errno=%d\n", errno);
        exit(-1);

    } else {

        /* close write end of pipes */
        close(p_stdout[1]);
        close(p_stderr[1]);

        /* save the pid in the registry */
        if(ORTE_SUCCESS != (rc = orte_pls_base_set_proc_pid(&proc->proc_name, pid))) {
            ORTE_ERROR_LOG(rc);
            return rc;
        }

        /* wait for the child process */
        OMPI_THREAD_LOCK(&mca_pls_fork_component.lock);
        mca_pls_fork_component.num_children++;
        OMPI_THREAD_UNLOCK(&mca_pls_fork_component.lock);
        OBJ_RETAIN(proc);
        orte_wait_cb(pid, orte_pls_fork_wait_proc, proc);

        /* connect read end to IOF */
        rc = orte_iof.iof_publish(&proc->proc_name, ORTE_IOF_SOURCE, ORTE_IOF_STDOUT, p_stdout[0]);
        if(ORTE_SUCCESS != rc) {
            ORTE_ERROR_LOG(rc);
            return rc;
        }
                                                                                                                      
        rc = orte_iof.iof_publish(&proc->proc_name, ORTE_IOF_SOURCE, ORTE_IOF_STDERR, p_stderr[0]);
        if(ORTE_SUCCESS != rc) {
            ORTE_ERROR_LOG(rc);
            return rc;
        }
    }
    return ORTE_SUCCESS;
}


int orte_pls_fork_launch(orte_jobid_t jobid)
{
    ompi_list_t map;
    ompi_list_item_t* item;
    int rc;

    /* query the allocation for this node */
    OBJ_CONSTRUCT(&map, ompi_list_t);
    rc = orte_rmaps_base_get_node_map(
        orte_process_info.my_name->cellid,jobid,orte_system_info.nodename,&map);
    if (ORTE_SUCCESS != rc) {
        ORTE_ERROR_LOG(rc);
        goto cleanup;
    }

    /* attempt to launch each of the apps */
    for(item =  ompi_list_get_first(&map);
        item != ompi_list_get_end(&map);
        item =  ompi_list_get_next(item)) {
        orte_rmaps_base_map_t* map = (orte_rmaps_base_map_t*)item;
        size_t i;
        for(i=0; i<map->num_procs; i++) {
            rc = orte_pls_fork_proc(map->app, map->procs[i]);
            if(ORTE_SUCCESS != rc) {
                ORTE_ERROR_LOG(rc);
                goto cleanup;
            }
        }
    }

cleanup:
    while(NULL != (item = ompi_list_remove_first(&map))) {
        OBJ_RELEASE(item);
    }
    OBJ_DESTRUCT(&map);
    return rc;
}

int orte_pls_fork_terminate_job(orte_jobid_t jobid)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_pls_fork_terminate_proc(const orte_process_name_t* proc)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_pls_fork_finalize(void)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}



/* -*- C -*-
 * 
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
 */

#include "ompi_config.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/wait.h>

#include "util/output.h"
#include "util/proc_info.h"
#include "runtime/orte_wait.h"
#include "runtime/runtime.h"
#include "mca/ns/base/base.h"
#include "mca/pls/base/base.h"
#include "mca/rmgr/base/base.h"
#include "mca/ras/base/base.h"
#include "mca/rmaps/base/rmaps_base_map.h"

#include "pls_bproc_seed.h"



orte_pls_base_module_t orte_pls_bproc_seed_module = {
    orte_pls_bproc_seed_launch,
    orte_pls_bproc_seed_finalize
};



static int orte_pls_bproc_nodelist(orte_rmaps_base_map_t* map, int** nodelist, size_t* num_nodes)
{
    ompi_list_item_t* item;
    size_t count = ompi_list_get_size(&map->nodes);
    size_t index = 0;

    /* build the node list */
    *nodelist = (int*)malloc(sizeof(int) * count);
    if(NULL == *nodelist)
        return OMPI_ERR_OUT_OF_RESOURCE;

    for(item =  ompi_list_get_first(&map->nodes);
        item != ompi_list_get_end(&map->nodes);
        item =  ompi_list_get_next(item)) {
        orte_rmaps_base_node_t* node = (orte_rmaps_base_node_t*)item;
        (*nodelist)[index++] = atol(node->node_name);
    }
    *num_nodes = count;
    return OMPI_SUCCESS;
}

/*
 *  Execute/dump a process and read the image into memory.
 */

static int orte_pls_bproc_dump(orte_app_context_t* app, uint8_t** image, size_t* image_len)
{
    pid_t pid;
    int pfd[2];
    size_t cur_offset, tot_offset, num_buffers;
    uint8_t *image_buffer;
    int rc = ORTE_SUCCESS;

    if (pipe(pfd)) {
        ompi_output(0, "orte_pls_bproc_seed: pipe() failed errno=%d\n",errno);
        return ORTE_ERROR;
    }

    if ((pid = fork ()) < 0) {
        ompi_output(0, "orte_pls_bproc_seed: fork() failed errno=%d\n",errno);
        return ORTE_ERROR;
    }

    if (pid == 0) {
        close(pfd[0]);  /* close the read end - we are write only */
        bproc_execdump(pfd[1], BPROC_DUMP_EXEC | BPROC_DUMP_OTHER, app->app, app->argv, app->env);
        exit(0);
    }

    /* this is the parent - I will read the
     * info coming from the pipe
     */

    close(pfd[1]); /* close the sending end - we are read only */
    image_buffer = (uint8_t*)malloc(orte_pls_bproc_seed_component.image_frag_size);
    if (!image_buffer) {
        ompi_output(0, "orte_pls_bproc_seed: couldn't allocate space for image\n");
        rc = ORTE_ERR_OUT_OF_RESOURCE;
        goto cleanup;
    }

    tot_offset = 0;
    cur_offset = 0;
    num_buffers = 1;
    while (1) {
        int num_bytes = read(pfd[0], image_buffer + tot_offset, orte_pls_bproc_seed_component.image_frag_size - cur_offset);
        if (0 > num_bytes) {  /* got an error - abort process */
            free(image_buffer);
            rc = ORTE_ERR_OUT_OF_RESOURCE;
            goto cleanup;
        } else if (0 == num_bytes) {
            break;
        }

        tot_offset += num_bytes;
        cur_offset += num_bytes;
        if (orte_pls_bproc_seed_component.image_frag_size == cur_offset) {  /* filled the current buffer -  need to realloc */
            num_buffers++;
            image_buffer = (uint8_t*)realloc(image_buffer, num_buffers * orte_pls_bproc_seed_component.image_frag_size);
            if(NULL == image_buffer) {
                ompi_output(0, "orte_pls_bproc_seed: couldn't allocate space for image\n");
                goto cleanup;
            }
            cur_offset = 0;
        }
    }
    *image = image_buffer;
    *image_len = tot_offset;

cleanup:
    close(pfd[0]);
    waitpid(pid,0,0);
    return rc;
}

/*
 *  Spew out a new child based on the in-memory process image.
 */

static int orte_pls_bproc_undump(orte_rmaps_base_proc_t* proc, uint8_t* image, size_t image_len, pid_t* pid)
{
    int p_image[2];
    int p_name[2];

    if(pipe(p_image) < 0 ||
       pipe(p_name) < 0) {
       return ORTE_ERR_OUT_OF_RESOURCE;
    }

    /* connect the app to the IOF framework */

    /* fork a child process which is overwritten with the process image */
    *pid = fork();
    if (*pid == 0) {
        close(p_image[1]);  /* child is read only */
        close(p_name[1]);

        if(p_image[0] == orte_pls_bproc_seed_component.name_fd) {
            int fd = dup(p_image[0]);
            close(p_image[0]);
            p_image[0] = fd;
        }
        dup2(p_name[0], orte_pls_bproc_seed_component.name_fd);
        bproc_undump(p_image[0]);  /* child is now executing */
        exit(1);
    }

    /* parent is write-only */
    close(p_image[0]); 
    close(p_name[0]); 

    if(*pid < 0) {
        close(p_image[1]);
        close(p_name[1]);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }

    /* write the process image to the app */
    write(p_name[1], &proc->proc_name, sizeof(proc->proc_name));
    close(p_name[1]);
    write(p_image[1], image, image_len);
    close(p_image[1]);
    return ORTE_SUCCESS;
}


/*
 *  Wait for a callback indicating the child has completed.
 */

static void orte_pls_bproc_wait_proc(pid_t pid, int status, void* cbdata)
{
    OMPI_THREAD_LOCK(&orte_pls_bproc_seed_component.lock);
    orte_pls_bproc_seed_component.num_completed++;
    ompi_condition_signal(&orte_pls_bproc_seed_component.condition);
    OMPI_THREAD_UNLOCK(&orte_pls_bproc_seed_component.lock);

    /* notify SOH the process has exited */
}

/*
 *  Wait for a callback indicating the daemon has exited.
 */
static void orte_pls_bproc_wait_node(pid_t pid, int status, void* cbdata)
{
    orte_rmaps_base_node_t* node = (orte_rmaps_base_node_t*)cbdata;
    ompi_list_item_t* item;
    for(item =  ompi_list_get_first(&node->node_procs);
        item != ompi_list_get_end(&node->node_procs);
        item =  ompi_list_get_next(item)) {

        /* notify SOH all of the procs have exited */
    }
}


/* 
 *  (1) Execute/dump the process image and read into memory.
 *  (2) Fork a daemon across the allocated set of nodes.
 *  (3) Fork/undump the required number of copies of the process 
 *      on each of the nodes.
 */

static int orte_pls_bproc_launch_app(orte_jobid_t jobid, orte_rmaps_base_map_t* map)
{
    uint8_t* image = NULL;
    size_t image_len;
    int* node_list = NULL;
    int* daemon_pids = NULL;
    size_t num_nodes;
    orte_vpid_t daemon_vpid_start;
    int rc, index;

    /* convert node names to bproc nodelist */
    if(ORTE_SUCCESS != (rc = orte_pls_bproc_nodelist(map, &node_list, &num_nodes))) {
        ompi_output(0, "orte_pls_bproc_seed: insufficient resources\n", errno);
        goto cleanup;
    }

    if(NULL == (daemon_pids = (int*)malloc(sizeof(int) * num_nodes))) {
        goto cleanup;
    }

    /* read process image */
    if(ORTE_SUCCESS != (rc = orte_pls_bproc_dump(map->app, &image, &image_len))) {
        ompi_output(0, "orte_pls_bproc_seed: unable to execute: %s\n", map->app->app);
        goto cleanup;
    }
    
    /* allocate a range of vpids for the daemons */
    if(ORTE_SUCCESS != (rc = orte_ns.reserve_range(0, num_nodes, &daemon_vpid_start))) {
        ompi_output(0, "orte_pls_bproc_seed: unable to allocate name: %d\n", rc);
        goto cleanup;
    }

    /* replicate the process image to all nodes */
    rc = bproc_vrfork(num_nodes, node_list, daemon_pids);
    if(rc < 0) {
        ompi_output(0, "orte_pls_bproc_seed: bproc_vrfork failed, errno=%d\n", errno);
        return OMPI_ERROR;
    }

    /* return is the rank of the child or number of nodes in the parent */
    if(rc < (int)num_nodes) {
 
        ompi_list_item_t* item;
        orte_rmaps_base_node_t* node = NULL;
        size_t num_procs;
        orte_process_name_t* daemon_name;

        /* find this node */
        index = 0;
        for(item =  ompi_list_get_first(&map->nodes);
            item != ompi_list_get_end(&map->nodes);
            item =  ompi_list_get_next(item)) {
            if(index++ == rc) {
                node = (orte_rmaps_base_node_t*)item;
                break;
            }
        }
        if(NULL == node) {
            rc = ORTE_ERROR;
            goto cleanup;
        }

        /* restart the daemon w/ the new process name */
        rc = orte_ns.create_process_name(
            &daemon_name, orte_process_info.my_name->cellid, 0, daemon_vpid_start + rc);
        if(ORTE_SUCCESS != rc) {
            exit(1);
        }
        orte_restart(daemon_name);

        /* connect the daemons stdout/stderr to IOF framework */


        /* start the required number of copies of the application */
        index = 0;
        for(item =  ompi_list_get_first(&node->node_procs);
            item != ompi_list_get_end(&node->node_procs);
            item =  ompi_list_get_next(item)) {
            orte_rmaps_base_proc_t* proc = (orte_rmaps_base_proc_t*)item;
            pid_t pid;

            rc = orte_pls_bproc_undump(proc, image, image_len, &pid);
            if(ORTE_SUCCESS != rc) {
                exit(1);
            }
            orte_wait_cb(pid, orte_pls_bproc_wait_proc, proc);
        }

        /* free memory associated with the process image */
        free(image);

        /* wait for all children to complete */
        OMPI_THREAD_LOCK(&orte_pls_bproc_seed_component.lock);
        num_procs = ompi_list_get_size(&node->node_procs);
        while(orte_pls_bproc_seed_component.num_completed < num_procs) {
            ompi_condition_wait(
                &orte_pls_bproc_seed_component.condition,
                &orte_pls_bproc_seed_component.lock);
        }
        OMPI_THREAD_UNLOCK(&orte_pls_bproc_seed_component.lock);

        /* daemon is done when all children have completed */
        exit(0);

    } else {
        ompi_list_item_t* item;

        /* wait for the daemon pids to complete */
        rc = ORTE_SUCCESS;

        index = 0;
        for(item =  ompi_list_get_first(&map->nodes);
            item != ompi_list_get_end(&map->nodes);
            item =  ompi_list_get_next(item)) {
            orte_rmaps_base_node_t* node = (orte_rmaps_base_node_t*)
            orte_wait_cb(daemon_pids[index++], orte_pls_bproc_wait_node, node);
        }
    }
   
cleanup:
    if(NULL != image)
        free(image);
    if(NULL != node_list)
        free(node_list);
    if(NULL != daemon_pids)
        free(daemon_pids);
    return rc;
}

/*
 * Query for the default mapping.  Launch each application context 
 * w/ a distinct set of daemons.
 */

int orte_pls_bproc_seed_launch(orte_jobid_t jobid)
{
    ompi_list_item_t* item;
    ompi_list_t mapping;
    int rc;

    /* query for the application context and allocated nodes */
    OBJ_CONSTRUCT(&mapping, ompi_list_t);
    if(ORTE_SUCCESS != (rc = orte_rmaps_base_get_map(jobid, &mapping))) {
        return rc;
    }

    /* for each application context - launch across the first n nodes required */
    for(item =  ompi_list_get_first(&mapping);
        item != ompi_list_get_end(&mapping);
        item =  ompi_list_get_next(item)) {
        orte_rmaps_base_map_t* map = (orte_rmaps_base_map_t*)item;
        rc = orte_pls_bproc_launch_app(jobid, map); 
        if(rc != ORTE_SUCCESS)
            goto cleanup;
    }
  
cleanup:
    while(NULL != (item = ompi_list_remove_first(&mapping)))
        OBJ_RELEASE(item);
    OBJ_DESTRUCT(&mapping);
    return rc;
}


int orte_pls_bproc_seed_finalize(void)
{
    return ORTE_SUCCESS;
}



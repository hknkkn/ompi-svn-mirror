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


#include "mca/ns/base/base.h"
#include "mca/pls/base/base.h"
#include "mca/rmgr/base/base.h"
#include "mca/ras/base/base.h"

#include "pls_bproc_seed.h"


orte_pls_base_module_t orte_pls_bproc_seed_module = {
    orte_pls_bproc_seed_launch,
    orte_pls_bproc_seed_finalize
};



static int orte_pls_bproc_nodelist(size_t num_procs, ompi_list_t* nodes, int** nodelist, size_t* num_nodes)
{
    ompi_list_item_t* item;
    size_t num_alloc = 0;
    size_t index = 0;

    /* count the number of nodes required to satisfy the number of processes */
    for(item =  ompi_list_get_first(nodes);
        item != ompi_list_get_end(nodes);
        item =  ompi_list_get_next(item)) {
        orte_ras_base_node_t* node = (orte_ras_base_node_t*)item;
        (*num_nodes)++;
        if(num_alloc + node->node_alloc >= app->num_procs) {
            num_alloc += (app->num_procs - num_alloc);
            node->node_alloc -= (app->num_procs - num_alloc);
            break;
        }
        num_alloc += node->node_alloc;
    }

    /* build the node list */
    *nodelist = (int*)malloc(sizeof(int) * (*num_nodes));
    if(NULL == nodelist)
        return OMPI_ERR_OUT_OF_RESOURCE;

    for(item =  ompi_list_get_first(nodes);
        item != ompi_list_get_end(nodes);
        item =  ompi_list_get_next(item)) {
        orte_ras_base_node_t* node = (orte_ras_base_node_t*)item;
        (*nodelist)[index++] = atol(node->node_name);
    }
    if(num_alloc < num_procs) {
        free(nodelist);
        return OMPI_ERR_OUT_OF_RESOURCE;
    }
    return OMPI_SUCCESS;
}

/*
 *
 */

static int orte_pls_bproc_dump(orte_app_context_t* app, uint8_t** image)
{
    pid_t pid;
    int pfd[2];
    int cur_offset, tot_offset, num_buffers;
    int num_nodes, node_list[2], local_pids[2];
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
    image_buffer = (uint8_t*)malloc(APP_PROC_IMAGE_FRAG_SIZE);
    if (!image_buffer) {
        ompi_output(0, "orte_pls_bproc_seed: couldn't allocate space for image\n");
        rc = ORTE_ERR_OUT_OF_RESOURCE;
        goto cleanup;
    }

    tot_offset = 0;
    cur_offset = 0;
    num_buffers = 1;
    while (1) {
        int num_bytes = read(pfd[0], image_buffer + tot_offset, APP_PROC_IMAGE_FRAG_SIZE - cur_offset);
        if (0 > num_bytes) {  /* got an error - abort process */
            free(image_buffer);
            rc = ORTE_ERR_OUT_OF_RESOURCE;
            goto cleanup;
        } else if (0 == num_bytes) {
            break;
        }

        tot_offset += num_bytes;
        cur_offset += num_bytes;
        if (APP_PROC_IMAGE_FRAG_SIZE == cur_offset) {  /* filled the current buffer -  need to realloc */
            num_buffers++;
            image_buffer = (uint8_t*)realloc(image_buffer, num_buffers*APP_PROC_IMAGE_FRAG_SIZE);
            if(NULL == image_buffer) {
                ompi_output(0, "orte_pls_bproc_seed: couldn't allocate space for image\n");
                goto cleanup:
            }
            cur_offset = 0;
        }
    }
    *image = image_buffer;

cleanup:
    close(pfd[0]);
    waitpid(pid,0,0);
    return rc;
}

static int orte_pls_bproc_undump(uint8_t* image, const orte_process_name_t* name, pid_t* pid)
{
      pipe(pfd);
        pid2 = fork();
        if (pid2 == 0) {
            fprintf(fp, "child is alive - calling bproc_undump\n");
            close(pfd[1]);  /* child is read only */
            bproc_undump(pfd[0]);  /* child is now executing */
            exit(1);
        }
                                                                                                                
        close(pfd[0]);  /* parent is write-only */
        write(pfd[1], image_buffer, tot_offset);
        fprintf(fp, "parent image dump completed\n");
        close(pfd[1]);
        waitpid(pid2, 0, 0); /* don't really want to do this - need to just go into local daemon mode */
        exit(0);
    }
}


static int orte_pls_bproc_launch_app(orte_jobid_t jobid, orte_app_context_t* app, ompi_list_t* nodes)
{
    uint8_t* image = NULL;
    int* node_list = NULL;
    int* local_pids = NULL;
    size_t num_nodes;
    orte_vpid_t daemon_vpid_start;
    orte_vpid_t app_vpid_start;
    int rc;

    /* convert node names to bproc nodelist */
    if(ORTE_SUCCESS != (rc = orte_pls_bproc_nodelist(app->num_procs,nodes,&node_list,&num_nodes))) {
        ompi_output(0, "orte_pls_bproc_seed: insufficient resources\n", errno);
        goto cleanup;
    }

    if(NULL == (local_pids = (int*)malloc(sizeof(int) * num_nodes))) {
        goto cleanup;
    }

    /* read process image */
    if(ORTE_SUCCESS != (rc = orte_pls_bproc_dump(app, &image))) {
        ompi_output(0, "orte_pls_bproc_seed: unable to execute: %s\n", app->app);
        goto cleanup;
    }
    
    /* allocate a range of vpids for the daemons and the app */
    if(ORTE_SUCCESS != (rc = orte_ns.reserve_range(0, num_nodes, &daemon_vpid_start))) {
        ompi_output(0, "orte_pls_bproc_seed: unable to allocate name: %d\n", rc);
        goto cleanup;
    }

    if(ORTE_SUCCESS != (rc = orte_ns.reserve_range(jobid, app->num_procs, &app_vpid_start))) {
        ompi_output(0, "orte_pls_bproc_seed: unable to allocate name: %d\n", rc);
        goto cleanup;
    }

    /* replicate the process image to all nodes */
    rc = bproc_vrfork(num_nodes, node_list, local_pids);
    if(rc < 0) {
        ompi_output(0, "orte_pls_bproc_seed: bproc_vrfork failed, errno=%d\n", errno);
        return OMPI_ERROR;
    }

    /* return is the rank of the child or number of nodes in the parent */
    if(rc < num_nodes) {
 
        ompi_list_item_t* item;
        orte_ras_base_node_t* node;
        size_t index = 0;
        orte_vpid_t offset = 0;
        orte_process_name_t* daemon = orte_ns.create_process_name(
            node->node_cellid, 0, daemon_vpid_start + rc);

        /* restart the rte w/ the new process name */
        /* orte_rml.restart(); */

        /* determine the starting rank and number of processes */
        for(item =  ompi_list_get_first(nodes);
            item != ompi_list_get_end(nodes);
            item =  ompi_list_get_next(item)) {
            node = (orte_ras_base_node_t* node);
            if(index++ == rc)
                break;
            offset += node->node_alloc;
        }

        /* start the required number of copies of the application */
        for(i=0; i<node->num_alloc; i++) {
            int pid;
            orte_process_name_t* name = orte_ns.create_process_name(
                node->node_cellid, jobid, app_vpid_start + offset + i);

            rc = orte_pls_bproc_undump(image, name, &pid);
            if(ORTE_SUCCESS != rc) {
                /* notify soh something has failed */
                return rc;
            }

            /* register this process name/pid with soh */
            free(name);
        }

        /* free memory associated with the process image */
        free(image);
        image = NULL;

        /* wait for children to all exit - do I/O forwarding until 
         * they all complete
         */
        while(num_completed < node->num_alloc) {
            int status;
#if OMPI_HAVE_THREADS
            pid_t pid = waitpid(-1, &status, WNOHANG);
            ompi_progress();
#else
            pid_t pid = wait(&status);
#endif
            if(pid > 0) {
                /* need to notify someone that this has exited? */
                num_completed++;
            }
        }

        /* daemon is done when all children have completed */
        orte_rte_finalize();
        exit(0);

    } else {
        /* register the daemon pids with the SOH monitor */
        rc = ORTE_SUCCESS;
    }
   
cleanup:
    if(NULL != image)
        free(image);
    if(NULL != node_list)
        free(node_list);
    if(NULL != local_pids)
        free(local_pids);
    return rc;
}

/*
 *
 */

int orte_pls_bproc_seed_launch(orte_jobid_t jobid)
{
    orte_app_context_t** context;
    size_t num_context;
    ompi_list_t nodes;
    ompi_list_item_t* item;
    int rc;

    /* query for the application context and allocated nodes */
    if(ORTE_SUCCESS != (rc = orte_rmgr_base_get_app_context(jobid, &context, &num_context))) {
        return rc;
    }

    /* query for all nodes allocated to this job */
    OBJ_CONSTRUCT(&nodes, ompi_list_t);
    if(ORTE_SUCCESS != (rc = orte_ras_base_nodes_query_alloc(&nodes,jobid))) {
        return rc;
    }

    /* for each application context - launch across the first n nodes required */
    for(i=0; i<num_context; i++) {
        rc = orte_pls_bproc_seed_launch_app(context[i], nodes); 
        if(rc != ORTE_SUCCESS)
            goto cleanup;
    }
  
cleanup:
    while(NULL != (item = ompi_list_remove_first(&nodes)))
        OBJ_RELEASE(item);
    OBJ_DESTRUCT(&nodes);
    return rc;
}


int orte_pls_bproc_seed_finalize(void)
{
    return ORTE_SUCCESS;
}



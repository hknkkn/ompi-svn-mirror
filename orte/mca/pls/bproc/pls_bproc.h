/* -*- C -*-
 *
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2006 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 *
 *
 */
/**
 * @file:
 * Header file for the bproc launcher. This launcher is actually split into 2
 * modules: pls_bproc & pls_bproc_orted. The general idea behind this launcher is:
 * -# pls_bproc is called by orterun. It figures out the process mapping and
 *    launches orted's on the nodes
 * -# pls_bproc_orted is called by orted. This module intializes either a pty or
 *    pipes, places symlinks to them in well know points of the filesystem, and
 *    sets up the io forwarding. It then sends an ack back to orterun.
 * -# pls_bproc waits for an ack to come back from the orteds, then does several
 *    parallel launches of the application processes. The number of launches is
 *    equal to the maximum number of processes on a node. For example, if there
 *    were 2 processes assigned to node 1, and 1 process asigned to node 2, we
 *    would do a parallel launch that launches on process on each node, then
 *    another which launches another process on node 1.
 */

#ifndef ORTE_PLS_BPROC_H_
#define ORTE_PLS_BPROC_H_

#include "orte_config.h"
#include "orte/orte_constants.h"

#include <sys/bproc.h>

#include "opal/threads/condition.h"

#include "orte/class/orte_pointer_array.h"
#include "orte/util/proc_info.h"

#include "orte/mca/rml/rml_types.h"

#include "orte/mca/pls/base/base.h"

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

/*
 * Module open / close
 */
int orte_pls_bproc_component_open(void);
int orte_pls_bproc_component_close(void);

/*
 * Startup / Shutdown
 */
orte_pls_base_module_t* orte_pls_bproc_init(int *priority);
int orte_pls_bproc_finalize(void);

/*
 * Interface
 */
int orte_pls_bproc_launch(orte_jobid_t);
int orte_pls_bproc_terminate_job(orte_jobid_t);
int orte_pls_bproc_terminate_proc(const orte_process_name_t* proc_name);
int orte_pls_bproc_terminate_orteds(orte_jobid_t jobid);
int orte_pls_bproc_signal_job(orte_jobid_t, int32_t);
int orte_pls_bproc_signal_proc(const orte_process_name_t* proc_name, int32_t);

/* Utility routine to get/set process pid */
ORTE_DECLSPEC int orte_pls_bproc_set_proc_pid(const orte_process_name_t*, pid_t);
ORTE_DECLSPEC int orte_pls_bproc_get_proc_pid(const orte_process_name_t*, pid_t*);
/**
 * Utility routine to retreive all process pids w/in a specified job.
 */
ORTE_DECLSPEC int orte_pls_bproc_get_proc_pids(orte_jobid_t jobid, pid_t** pids, orte_std_cntr_t* num_pids);
/**
 * Utility routine to get/set daemon pid
 */
ORTE_DECLSPEC int orte_pls_bproc_set_node_pid(orte_cellid_t cellid, char* node_name, orte_jobid_t jobid, pid_t pid);
ORTE_DECLSPEC int orte_pls_bproc_get_node_pids(orte_jobid_t jobid, pid_t** pids, orte_std_cntr_t* num_pids);

/* utility functions for abort communications */
int orte_pls_bproc_comm_start(void);
int orte_pls_bproc_comm_stop(void);
void orte_pls_bproc_recv(int status, orte_process_name_t* sender,
                         orte_buffer_t* buffer, orte_rml_tag_t tag,
                         void* cbdata);
    
/**
 * PLS bproc Component
 */
struct orte_pls_bproc_component_t {
    orte_pls_base_component_t super;
    /**< The base class */
    bool done_launching;
    /**< Is true if we are done launching the user's app. */
    char * orted;
    /**< The orted executeable. This can be an absolute path, or if not found
     * we will look for it in the user's path */
    int debug;
    /**< If greater than 0 print debugging information */
    bool timing;
    /**< If true, report launch timing info */
    int num_procs;
    /**< The number of processes that are running */
    int priority;
    /**< The priority of this component. This will be returned if we determine
     * that bproc is available and running on this node, */
    int terminate_sig;
    /**< The signal that gets sent to a process to kill it. */
    size_t num_daemons;
    /**< The number of daemons that are currently running. */
    opal_mutex_t lock;
    /**< Lock used to prevent some race conditions */
    opal_condition_t condition;
    /**< Condition that is signaled when all the daemons have died */
    orte_pointer_array_t * daemon_names;
    /**< Array of the process names of all the daemons. This is used to send
     * the daemons a termonation signal when all the user processes are done */
    orte_pointer_array_t* active_node_names;
    /**< Array of the bproc node  names of all the daemons. This is used to 
     * track which bproc nodes belong to us*/
    bool bynode;
    /**< Indicates whether or not this application is to be mapped by node
     * (if set to true) or by slot (default)
     */
    bool recv_issued;
    /**< Indicates that the comm recv for reporting abnormal proc termination
     * has been issued
     */
    
};
/**
 * Convenience typedef
 */
typedef struct orte_pls_bproc_component_t orte_pls_bproc_component_t;

ORTE_DECLSPEC orte_pls_bproc_component_t mca_pls_bproc_component;
ORTE_DECLSPEC orte_pls_base_module_t orte_pls_bproc_module;

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif
#endif /* ORTE_PLS_BPROC_H_ */


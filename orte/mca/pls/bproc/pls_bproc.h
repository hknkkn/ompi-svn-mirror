/* -*- C -*-
 * 
 * Copyright (c) 2004-2005 The Trustees of Indiana University.
 *                         All rights reserved.
 * Copyright (c) 2004-2005 The Trustees of the University of Tennessee.
 *                         All rights reserved.
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
#include "orte/class/orte_pointer_array.h"
#include "orte/include/orte_constants.h"
#include "orte/mca/pls/base/base.h"
#include "orte/util/proc_info.h"
#include "opal/threads/condition.h"
#include <sys/bproc.h>

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
};
/**
 * Convenience typedef
 */
typedef struct orte_pls_bproc_component_t orte_pls_bproc_component_t;

ORTE_DECLSPEC extern orte_pls_bproc_component_t mca_pls_bproc_component;
ORTE_DECLSPEC extern orte_pls_base_module_t orte_pls_bproc_module;

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif
#endif /* ORTE_PLS_BPROC_H_ */


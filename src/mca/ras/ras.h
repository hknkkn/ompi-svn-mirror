/*
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
 */
/** @file:
 *
 * The Open RTE Resource Allocation Subsystem (RAS)
 * 
 * The resource allocation subsystem is responsible for determining
 * what (if any) resources have been allocated to the specified job
 * (via some prior action), and to obtain an allocation (if possible)
 * if resources have NOT been previously allocated. It is anticipated
 * that ORTE users will execute an "mpirun" or other command that
 * invokes ORTE through one of two channels:
 * 
 * 1. local: the user will login to the computing resource they intend
 * to use, request a resource allocation from that system, and then
 * execute the mpirun or other command. Thus, the allocation has
 * already been obtained prior to ORTE's initialization.  In most
 * cases, systems pass allocation information via environmental
 * parameters.  Thus, the RAS components must know the correct
 * environmental parameter to look for within the environment they
 * seek to support (e.g., an LSF component should know that LSF passes
 * allocation parameters as a specific LSF-named entity).
 * 
 * 2. remote: the user issues an mpirun command on their notebook or
 * desktop computer, indicating that the application is to be executed
 * on a specific remote resource.  In this case, the allocation may
 * not have been previously requested or made. Thus, the associated
 * RAS component must know how to request an allocation from the
 * designated resource. To assist in this process, the RAS can turn to
 * the information provided by the resource discovery subsystem (RDS)
 * to learn what allocator resides on the designated resource.
 * 
 * The RAS operates on a per-job basis - i.e., it serves to allocate
 * the resources for a specific job. It takes several inputs,
 * depending upon what is provided and desired:
 * 
 * - the jobid for which the resources are to be allocated. There are
 * two options here: (a) the jobid can be predefined and provided to
 * the allocator. In this case, the allocator will simply allocate
 * resources to the job; or (b) the jobid can be set by the allocator
 * via a request to the ORTE name services (NS) subsystem. This option
 * is selected by calling the allocate function with the illegal jobid
 * of ORTE_JOBID_MAX. In this case, the new jobid (set by the
 * allocator) will be returned in the provided address (the allocate
 * function takes a pointer to the jobid as its argument).
 * 
 * - MCA parameters specifying preallocated resources. These resources
 * are allocated to the specified jobid (whether set by the allocator
 * or not) on the first request.  However, subsequent requests for
 * allocation do NOT use these parameters - the parameters are "unset"
 * after initial use. This is done to prevent subsequent allocation
 * requests from unintentionally overloading the specified resources
 * in cases where the univese is persistent and therefore servicing
 * multiple applications.
 * 
 * - MCA parameters specifying the name of the application(s) and the
 * number of each application to be executed. These will usually be
 * taken from the command line options, but could be provided via
 * environmental parameters.
 * 
 * - the resources defined in the ORTE_RESOURCE_SEGMENT by the
 * RDS. When an allocation is requested for resources not previously
 * allocated, the RAS will attempt to obtain an allocation that meets
 * the specified requirements. For example, if the user specifies that
 * the application must run on an Intel Itanium 2 resource under the
 * Linux operating system, but doesn't provide the allocation or
 * resource identification, then the allocator can (if possible)
 * search the ORTE_RESOURCE_SEGMENT for resources meeting those
 * specifications and attempt to obtain an allocation from them.
 * 
 * The RAS outputs its results into three registry segments:
 * 
 * (a) the ORTE_NODE_STATUS_SEGMENT. The segment consists of a
 * registry container for each node that has been allocated to a job -
 * for proper operation, each container MUST be described by the
 * following set of tokens:
 * 
 * - nodename: a unique name assigned to each node, usually obtained
 * from the preallocated information in the environmental variables or
 * the resource manager for the specified compute resource (e.g.,
 * LSF). For those cases where specific nodenames are not provided,
 * the allocator can use the info provided by the RDS to attempt to
 * determine the nodenames (e.g., if the RDS learned that the nodes
 * are name q0-q1024 and we obtain an allocation of 100 nodes
 * beginning at node 512, then the RAS can derive the nodenames from
 * this information).
 * 
 * - cellid: the id assigned to the cell within which this node
 * exists. This id is provided in the ORTE_RESOURCE_SEGMENT.
 * 
 * For each node, the RAS stores the following information on the segment:
 * 
 * - number of cpus allocated from this node to the user. This will
 * normally be the number of cpus/node as obtained from the data
 * provided by the RDS, but could differ in some systems.
 * 
 * - the jobids that are utilizing this node. In systems that allow
 * overloading of processes onto nodes, there may be multiple jobs
 * sharing a given node.
 * 
 * - the status of the node (up, down, rebooting, etc.). This
 * information is provided and updated by the state-of-health (SOH)
 * monitoring subsystem.
 * 
 * (b) the ORTE_JOB_SEGMENT. The RAS preallocates this segment,
 * initializing one container for each process plus one container to
 * store information that spans the job. This latter container houses
 * information such as the application names, number of processes per
 * application, process context (including argv and enviro arrays),
 * and i/o forwarding info. The RAS does NOT establish nor fill any of
 * the individual process info containers - rather, it preallocates
 * the storage for those containers and places some of the job-wide
 * information into that container. This info includes:
 * 
 * - application names and number of processes per application
 * 
 * - process context
 * 
 * The remainder of the information in that container will be supplied
 * by other subsystems.
 * 
 * (c) the ORTE_RESOURCE_SEGMENT. The RAS adds information to this
 * segment to indicate consumption of an available resource. In
 * particular, the RAS updates fields in the respective compute
 * resource to indicate the portion of that resource that has been
 * allocated and therefore can be presumed consumed. This includes
 * info on the number of nodes and cpus allocated to existing jobs -
 * these numbers are updated by the RAS when resources are deallocated
 * at the completion of a job.
 * 
 * The information provided by the RAS is consumed by the resource
 * mapper subsystem (RMAPS) that defines which process is executed
 * upon which node/cpu, the process launch subsystem (PLS) that
 * actually launches each process, and others.
 * 
 * Because the RAS operates as a multi-component framework (i.e.,
 * multiple components may be simultaneously instantiated), the RAS
 * functions should NOT be called directly.  Instead, they should be
 * accessed via the ORTE resource manager (RMGR) subsystem.
 * 
 * 
 */

#ifndef ORTE_MCA_RAS_H
#define ORTE_MCA_RAS_H

#include "orte_config.h"
#include "include/orte_constants.h"

#include "mca/mca.h"
#include "mca/ns/ns_types.h"
#include "ras_types.h"


/*
 * ras module functions
 */

/**
 * Allocate resources to a job.
 */
typedef int (*orte_ras_base_module_allocate_fn_t)(orte_jobid_t jobid);

/**
 * Deallocate resources from a job
 */
typedef int (*orte_ras_base_module_deallocate_fn_t)(orte_jobid_t jobid);

/**
 * Cleanup module resources.
 */
typedef int (*orte_ras_base_module_finalize_fn_t)(void);

/**
 * ras module version 1.0.0
 */
struct orte_ras_base_module_1_0_0_t {
    /** Allocation function pointer */
    orte_ras_base_module_allocate_fn_t allocate;
    /** Deallocation function pointer */
    orte_ras_base_module_deallocate_fn_t deallocate;
    /** Finalization function pointer */
    orte_ras_base_module_finalize_fn_t finalize;
};
/** Convenience typedef */
typedef struct orte_ras_base_module_1_0_0_t orte_ras_base_module_1_0_0_t;
/** Convenience typedef */
typedef orte_ras_base_module_1_0_0_t orte_ras_base_module_t;

/*
 * ras component
 */

/**
 * Component init / selection
 */
typedef orte_ras_base_module_t* (*orte_ras_base_component_init_fn_t)(int* priority);

 
/**
 * ras component version 1.0.0
 */
struct orte_ras_base_component_1_0_0_t {
    /** Base MCA structure */
    mca_base_component_t ras_version;
    /** Base MCA data */
    mca_base_component_data_1_0_0_t ras_data;
    /** Initialization / selection function pointer */
    orte_ras_base_component_init_fn_t ras_init;
};
/** Convenience typedef */
typedef struct orte_ras_base_component_1_0_0_t orte_ras_base_component_1_0_0_t;
/** Convenience typedef */
typedef orte_ras_base_component_1_0_0_t orte_ras_base_component_t;


/**
 * Macro for use in components that are of type ras v1.0.0
 */
#define ORTE_RAS_BASE_VERSION_1_0_0 \
  /* ras v1.0 is chained to MCA v1.0 */ \
  MCA_BASE_VERSION_1_0_0, \
  /* ras v1.0 */ \
  "ras", 1, 0, 0

#endif


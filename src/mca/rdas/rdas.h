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
 */
/**
 * @file
 *
 * Resource Discovery & Allocation Subsystem (RDAS)
 *
 * The RDAS is responsible for discovering the resources available to the universe, and
 * for allocating them to the requesting job.
 *
 */

#ifndef MCA_RDAS_H
#define MCA_RDAS_H

#include "ompi_config.h"
#include "mca/mca.h"
#include "class/ompi_list.h"

/*
 * MCA component management functions
 */

/**
 * RDAS initialization function
 *
 * Called by the MCA framework to initialize the component.  Will
 * be called exactly once in the lifetime of the process.
 *
 * @param have_threads (IN) Whether the current running process is
 *                       multi-threaded or not.  true means there
 *                       may be concurrent access into the
 *                       underlying components *and* that the
 *                       components may launch new threads.
 * @param priority (OUT) Relative priority or ranking use by MCA to
 *                       select a module.
 *
 * \warning The only requirement on the returned type is that the
 * first sizeof(struct mca_rdas_base_module_1_0_0_t) bytes have the
 * same structure as a struct mca_rdas_base_module_1_0_0_t.  The rdas is
 * free to return a pointer to a larger structure in order to maintain
 * per-module information it may need.  Therefore, the caller should
 * never copy the structure or assume its size.
 */
typedef struct mca_rdas_base_module_1_0_0_t* 
(*mca_rdas_base_component_init_fn_t)(bool have_threads,
                                    int *priority);


/**
 * RDAS finalization function
 *
 * Called by the MCA framework to finalize the component.  Will be
 * called once per successful call to rdas_base_compoenent_init.
 */
typedef int (*mca_rdas_base_finalize_fn_t)(struct mca_rdas_base_module_1_0_0_t *me);


/** 
 * RDAS module version and interface functions
 *
 * \note the first two entries have type names that are a bit
 *  misleading.  The plan is to rename the mca_base_module_*
 * types in the future.
 */
struct mca_rdas_base_component_1_0_0_t {
  /** component version */
  mca_base_component_t rdas_version;
  /** component data */
  mca_base_component_data_1_0_0_t rdas_data;
  /** Function called when component is initialized  */
  mca_rdas_base_component_init_fn_t rdas_init;
};
/** shorten mca_rdas_base_component_1_0_0_t declaration */
typedef struct mca_rdas_base_component_1_0_0_t mca_rdas_base_component_1_0_0_t;
/** shorten mca_rdas_base_component_t declaration */
typedef mca_rdas_base_component_1_0_0_t mca_rdas_base_component_t;


/*
 * RDAS interface functions
 */

/*
 * Resource Discover functions
 */

/**
 * Get resource discovery specifications from command line and/or environment
 * Parse the command line and/or environment for resource discovery-related parameters
 * and process that information accordingly. Command-line options will override
 * environmental parameters.
 * 
 * Resource discovery options include the name of a hostfile, and the name of a
 * resource file (documented elsewhere).
 * 
 * @param cmd_line Command-line handle containing the options to be parsed. If NULL,
 * only environmental parameters will be checked.
 * 
 * @retval OMPI_SUCCESS Resource discovery options were parsed and allocation successfully
 * placed on registry.
 * @retval OMPI_ERROR No resource discovery options could be found, or ones provided
 * could not be successfully processed.
 */
typedef int
(*mca_rdas_base_process_resource_discovery_options_fn_t)(ompi_cmd_line_t cmd_line);


/*
 * Resource Allocation functions
 */

/**
 * Request resource allocation
 *
 * @param jobid (IN) Jobid with which to associate the given resources.
 * @param num_procs (IN) Number of processes to be spawned. If 0, the allocator
 *                       will allocate \c num_nodes nodes. If non-zero, it is
 *                       used in combination with the \c max_procs_per_node
 *                       value to compute the number of nodes required.
 * @param num_nodes (IN) Number of nodes to try to allocate.  If 0, the
 *                       allocator will try to allocate \c num_procs processes
 *                       on as many nodes as are needed.  If non-zero, 
 *                       will try to allocate \c procs process slots 
 *                       per node.
 * @param max_procs_per_node (IN) Maximum number of processes that are to
 *                       be allocated to each node. If 0, the allocator will
 *                       utilize the \c num_nodes parameter to do the allocation.
 * @retval OMPI_SUCCESS Allocation was computed and placed on the registry.
 * @retval OMPI_ERROR Allocation could not be determined.
 *
 */
typedef int
(*mca_rdas_base_request_allocation_fn_t)(mca_ns_base_jobid_t jobid, 
                                     int num_procs, int num_nodes,
                                     int max_procs_per_node);


/**
 * Set allocation as specified
 * 
 */
typedef int
(*mca_rdas_base_set_allocation_fn_t)(mca_ns_base_jobid_t jobid,
                                     ompi_list_t *allocation);


/**
 * Get allocation information
 * Obtain a report of the current allocation for the specified jobid.
 * 
 * @param jobid (IN) The jobid for which the information is requested
 * 
 * @retval node_list (OUT) An ompi_list of node structures that details
 * the allocation.
 */
typedef ompi_list_t*
(*mca_rdas_base_get_allocation_fn_t)(mca_ns_base_jobid_t jobid);

/**
 * Get allocation specifications from command line and/or environment
 * Parse the command line and/or environment for allocation-related parameters and
 * set the resource allocation accordingly. Command-line options will override
 * environmental parameters.
 * 
 * @param jobid (IN) Jobid with which to associate the given resources.
 * @param cmd_line Command-line handle containing the options to be parsed. If NULL,
 * only environmental parameters will be checked.
 * 
 * @retval OMPI_SUCCESS Allocation options were parsed and allocation successfully
 * placed on registry.
 * @retval OMPI_ERROR Allocation could not be determined.
 */
typedef int
(*mca_rdas_base_process_allocation_options_fn_t)(mca_ns_base_jobid_t jobid,
                                                 ompi_cmd_line_t cmd_line);


/**
 * Deallocate requested resources
 *
 * Return the resources for the given jobid to the system.
 *
 * @param jobid (IN) Jobid associated with the resources to be freed.
 *
 * @retval OMPI_SUCCESS Resources successfully deallocated.
 * @retval OMPI_ERROR Resources could not be deallocated.
 */
typedef int
(*mca_rdas_base_deallocate_resources_fn_t)(mca_ns_base_jobid_t jobid);


/**
 * Base module structure for the RDAS
 *
 * Base module structure for the RDAS - presents the required function
 * pointers to the calling interface. 
 */
struct mca_rdas_base_module_1_0_0_t {
    /* Function to process resource discovery options */
    mca_rdas_base_process_resource_discovery_options_fn_t discover_resources;
    /* Function to be called to request an allocation */
    mca_rdas_base_request_allocation_fn_t request_resource_allocation;
    /* Function to be called to set a specific resource allocation */
    mca_rdas_base_set_allocation_fn_t set_resource_allocation;
    /* Function to be called to parse resource allocation from cmd line or enviro */
    mca_rdas_base_process_allocation_options_fn_t process_allocation_options;
    /* Function to retrieve an allocation */
    mca_rdas_base_get_allocation_fn_t get_resource_allocation;
    /** Function to be called on resource return */ 
    mca_rdas_base_deallocate_resources_fn_t deallocate_resources;
    /** Function called when component is finalized */
    mca_rdas_base_finalize_fn_t rdas_finalize;
};
/** shorten mca_rdas_base_module_1_0_0_t declaration */
typedef struct mca_rdas_base_module_1_0_0_t mca_rdas_base_module_1_0_0_t;
/** shorten mca_rdas_base_module_t declaration */
typedef struct mca_rdas_base_module_1_0_0_t mca_rdas_base_module_t;


/**
 * Macro for use in modules that are of type pml v1.0.0
 */
#define MCA_RDAS_BASE_VERSION_1_0_0 \
  /* rdas v1.0 is chained to MCA v1.0 */ \
  MCA_BASE_VERSION_1_0_0, \
  /* rdas v1.0 */ \
  "rdas", 1, 0, 0

#endif /* MCA_RDAS_H */

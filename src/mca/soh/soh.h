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
/** @file:
 *
 * The Open MPI State-of-Health Monitoring Subsystem
 *
 */

#ifndef ORTE_SOH_H
#define ORTE_SOH_H

/*
 * includes
 */

#include "orte_config.h"
#include "include/orte_constants.h"
#include "include/orte_types.h"

#include "mca/mca.h"
#include "mca/ns/ns_types.h"
#include "mca/soh/soh_types.h"

#include "soh_types.h"	/* gpr keys and external datatypes needed for prototyping */

/*
 * Component functions - all MUST be provided!
 */


/*
 * The SOH consists of a number of distinct interacting parts
 * 
 * The master SOH is responsible for monitoring a proc/job/cell/universe
 * The range of what and who the master(s) monitor is left flexible so that
 * multiple masters may exist that are responsible for different areas of the
 * system to aid scalability. (A master is also usually a slave by default!)
 *
 * A slave SOH is just an end-point for monitoring. Once it has been asked 
 * to monitor it becomes a master itself. How the endpoint works depends on the
 * endpoint module being used (TCP vs UDP-timeout vs Bproc etc).
 *
 * If a slave is asked to monitor someone and can run the master SOH code it becomes
 * a master itself. Special case is when the slave asks for a bi-directional monitor
 * commonly used to monitor the seed/master SOH. This involves an explicit call to the 
 * local error manager on an unexpected change of state.
 *
 * A master can be push or pull based. But the master-slave relationship must be flexible
 * so that a master can ask to monitor another entity or can be asked to be monitored by the entity.
 *
 * A slave can only report a monitoring failure via its local error manager.
 * it is not supposed to access the GPR itself.
 *
 * A master can report a change of state to the GPR, as well as be notified via 
 * an RML message originating from an error manger.
 *
 *
 * An entity that neither monitors or is monitored is a client. These can make remote query/monitor/push requests.
 *
 *
 * Note, a master does not have a register interested parties of changes interface.. this is being performed by 
         entities performing sycro operation on registry segments or via the master error manager.
 *
 * The interface to the SOH consists of the following areas:
 *
 * (1) slave SOH calls : Init (Set up endpoints), poll (where needed / heatbeat), request monitoring, end monitoring
 *
 * (2) master SOH calls : Init (post recvs for requests), poll (where needed  /heatbeat / push), handle remote requests,
 *                        block monitoring functions (like (un)monitor named groups etc)
 *                        
 * (3) query functions : request status of entites (maybe via master or GPR)
 *
 * (4) communication functions : send (un)monitoring requests, send/recv change of state messages
 *
 * (5) support functions : update / query and handle cleanup of GPR entries
 *
 *
 */

/* 
 * 1. Slave SOH calls
 *
 * Anyone wishing to be monitored (an end point for monitoring) needs to use these functions 
 *
 */

 typedef int (*orte_soh_base_module_slave_init_fn_t) (void);
 typedef int (*orte_soh_base_module_slave_poll_fn_t) (void);
 typedef int (*orte_soh_base_module_slave_do_heatbeat_fn_t) (void);
 typedef int (*orte_soh_base_module_slave_request_monitor_fn_t) (orte_soh_entity_type_t type, orte_soh_entity_value_t value);
 typedef int (*orte_soh_base_module_slave_end_monitor_fn_t) (void);
 typedef int (*orte_soh_base_module_slave_finalise_fn_t) (void);
/* endpoint context needs to be added so we can pass from one call to another */
/* as does monitor context if we allow multiple requests to master SOHs   GEF */

/*
 * 2. Master SOH calls 
 *
 * Anyone wishing to monitor others needs to call these functions
 *
 */

 typedef int (*orte_soh_base_module_master_init_fn_t) (void);
 typedef int (*orte_soh_base_module_master_poll_fn_t) (void);
 typedef int (*orte_soh_base_module_master_handle_request_fn_t) (void);
 typedef int (*orte_soh_base_module_master_add_monitor_cell_fn_t) (orte_cellid_t cellid);
 typedef int (*orte_soh_base_module_master_remove_monitor_cell_fn_t) (orte_cellid_t cellid);
 typedef int (*orte_soh_base_module_master_add_monitor_node_fn_t) (orte_process_name_t node);
 typedef int (*orte_soh_base_module_master_remove_monitor_node_fn_t) (orte_process_name_t node);
 typedef int (*orte_soh_base_module_master_add_monitor_job_fn_t) (orte_jobid_t jobid);
 typedef int (*orte_soh_base_module_master_remove_monitor_job_fn_t) (orte_jobid_t jobid);
 typedef int (*orte_soh_base_module_master_add_monitor_proc_fn_t) (orte_process_name_t proc);
 typedef int (*orte_soh_base_module_master_remove_monitor_proc_fn_t) (orte_process_name_t proc);
 typedef int (*orte_soh_base_module_master_update_state_all_fn_t) (void);
 typedef int (*orte_soh_base_module_master_update_state_cell_fn_t) (orte_cellid_t cellid);
 typedef int (*orte_soh_base_module_master_update_state_node_fn_t) (orte_process_name_t node);
 typedef int (*orte_soh_base_module_master_update_state_job_fn_t) (orte_jobid_t jobid );
 typedef int (*orte_soh_base_module_master_update_state_proc_fn_t) (orte_jobid_t jobid );
 typedef int (*orte_soh_base_module_master_pull_state_cell_fn_t) (orte_cellid_t cellid);
 typedef int (*orte_soh_base_module_master_pull_state_node_fn_t) (orte_process_name_t node);
 typedef int (*orte_soh_base_module_master_pull_state_job_fn_t) (orte_jobid_t jobid);
 typedef int (*orte_soh_base_module_master_pull_state_proc_fn_t) (orte_process_name_t proc);
 typedef int (*orte_soh_base_module_master_finalise_fn_t) (void);

/*
 * 3. Query function. 
 * the following is needed by eclipse? *
 */

/*
 * Query the state-of-health of a process
 */
typedef int (*orte_soh_base_module_get_proc_soh_fn_t)(orte_status_key_t *status,
                                                     orte_process_name_t *proc);

/*
 * Query SOH of a node 
 * (guess)
 */
typedef int (*orte_soh_base_module_get_node_soh_fn_t)(orte_node_state_t *status,
                                                     orte_process_name_t *node);

/*
 * 4. Communication functions between clients of the SOH and between master and slave SOH peers
 *
 * These are shorthand functions that do RML/OOB type operations
 *
 */


 typedef int (*orte_soh_base_module_send_monitor_request_fn_t) (orte_soh_entity_type_t type, orte_soh_entity_value_t value);
 typedef int (*orte_soh_base_module_send_unmonitor_request_fn_t) (orte_soh_entity_type_t type, orte_soh_entity_value_t value);
 typedef int (*orte_soh_base_module_send_push_state_fn_t) (orte_soh_entity_type_t type);
 typedef int (*orte_soh_base_module_send_pull_state_fn_t) (orte_soh_entity_type_t type);
 typedef int (*orte_soh_base_module_send_state_change_fn_t) (orte_soh_entity_type_t type, orte_soh_entity_value_t value);
 typedef int (*orte_soh_base_module_recv_state_fn_t) (orte_soh_entity_type_t type, orte_soh_entity_value_t *value);

/*
 * 5. Misc support functions
 *
 * Currently clean up done in finalize calls
 *
 */




/* original push state functions. probably defunc */
/* node / system updates (non process) */

/* Update the state-of-health of a cell
 */
typedef int (*orte_soh_base_module_update_cell_soh_fn_t)(orte_cellid_t cellid);

/* process updates */

/* Update the state-of-health of a job 
 */

typedef int (*orte_soh_base_module_update_job_soh_fn_t)(orte_jobid_t jobid);

/* Update the state-of-health of a proc 
 */

typedef int (*orte_soh_base_module_update_proc_soh_fn_t)(orte_process_name_t procname);


/* Shutdown the module nicely 
 */

typedef int (*orte_soh_base_module_finalize_soh_fn_t)(void);



/* below are the prototypes needed by the MCA */
                                                     
/*
 * Ver 1.0.0
 */
struct orte_soh_base_module_1_0_0_t {

	/* soh slave */
    orte_soh_base_module_slave_init_fn_t             slave_init_soh;
    orte_soh_base_module_slave_poll_fn_t             slave_poll_soh;
    orte_soh_base_module_slave_do_heatbeat_fn_t      slave_do_heartbeat_soh;
    orte_soh_base_module_slave_request_monitor_fn_t  slave_request_monitor_soh;
    orte_soh_base_module_slave_end_monitor_fn_t      slave_end_monitor_soh;
    orte_soh_base_module_slave_finalise_fn_t         slave_finalise_soh;

	/* soh master */
    orte_soh_base_module_master_init_fn_t                 master_init_soh;
    orte_soh_base_module_master_poll_fn_t                 master_poll_soh;
    orte_soh_base_module_master_handle_request_fn_t       master_handle_request_soh;
    orte_soh_base_module_master_add_monitor_cell_fn_t     master_add_monitor_cell_soh;
    orte_soh_base_module_master_remove_monitor_cell_fn_t  master_remove_monitor_cell_soh;
    orte_soh_base_module_master_add_monitor_node_fn_t     master_add_monitor_node_soh;
    orte_soh_base_module_master_remove_monitor_node_fn_t  master_remove_monitor_node_soh;
    orte_soh_base_module_master_add_monitor_job_fn_t      master_add_monitor_job_soh;
    orte_soh_base_module_master_remove_monitor_job_fn_t   master_remove_monitor_job_soh;
    orte_soh_base_module_master_add_monitor_proc_fn_t     master_add_monitor_proc_soh;
    orte_soh_base_module_master_remove_monitor_proc_fn_t  master_remove_monitor_proc_soh;
    orte_soh_base_module_master_update_state_all_fn_t     master_update_state_all_soh;
    orte_soh_base_module_master_update_state_cell_fn_t    master_update_state_cell_soh;
    orte_soh_base_module_master_update_state_node_fn_t    master_update_state_node_soh;
    orte_soh_base_module_master_update_state_job_fn_t     master_update_state_job_soh;
    orte_soh_base_module_master_update_state_proc_fn_t    master_update_state_proc_soh;
    orte_soh_base_module_master_pull_state_cell_fn_t      master_pull_state_cell_soh;
    orte_soh_base_module_master_pull_state_node_fn_t      master_pull_state_node_soh;
    orte_soh_base_module_master_pull_state_job_fn_t       master_pull_state_job_soh;
    orte_soh_base_module_master_pull_state_proc_fn_t      master_pull_state_proc_soh;
    orte_soh_base_module_master_finalise_fn_t             master_finalise_soh;

	/* soh query */
    orte_soh_base_module_get_proc_soh_fn_t      get_proc_soh;
    orte_soh_base_module_get_node_soh_fn_t      get_node_soh;

	/* soh communications */
    orte_soh_base_module_send_monitor_request_fn_t     send_monitor_request_soh;
    orte_soh_base_module_send_unmonitor_request_fn_t   send_unmonitor_request_soh;
    orte_soh_base_module_send_push_state_fn_t          send_push_state_soh;
    orte_soh_base_module_send_pull_state_fn_t          send_pull_state_soh;
    orte_soh_base_module_send_state_change_fn_t        send_state_change_soh;
    orte_soh_base_module_recv_state_fn_t               recv_state_soh;

	/* soh misc support */
	orte_soh_base_module_finalize_soh_fn_t      soh_finalize;

	/* soh previous (used by bproc?) */
    orte_soh_base_module_update_cell_soh_fn_t   update_cell_soh; 
	orte_soh_base_module_update_job_soh_fn_t    update_job_soh;
	orte_soh_base_module_update_proc_soh_fn_t   update_proc_soh;
};

typedef struct orte_soh_base_module_1_0_0_t orte_soh_base_module_1_0_0_t;
typedef orte_soh_base_module_1_0_0_t orte_soh_base_module_t;

/*
 * SOH Component
 */

typedef orte_soh_base_module_t* (*orte_soh_base_component_init_fn_t)(
    int *priority);

typedef int (*orte_soh_base_component_finalize_fn_t)(void);
 
/*
 * the standard component data structure
 */

struct orte_soh_base_component_1_0_0_t {
    mca_base_component_t 							soh_version;
    mca_base_component_data_1_0_0_t 				soh_data;

    orte_soh_base_component_init_fn_t 				soh_init;
};

typedef struct orte_soh_base_component_1_0_0_t orte_soh_base_component_1_0_0_t;

typedef orte_soh_base_component_1_0_0_t orte_soh_base_component_t;



/*
 * Macro for use in components that are of type ns v1.0.0
 */
#define MCA_SOH_BASE_VERSION_1_0_0 \
  /* soh v1.0 is chained to MCA v1.0 */ \
  MCA_BASE_VERSION_1_0_0, \
  /* soh v1.0 */ \
  "soh", 1, 0, 0

/**
  * Global structure for accessing SOH functions
  */

OMPI_DECLSPEC extern orte_soh_base_module_t orte_soh;  /* holds selected module's function pointers */



#endif /* ORTE_SOH_H */

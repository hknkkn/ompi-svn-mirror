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
 */

#ifndef MCA_SOH_BASE_H
#define MCA_SOH_BASE_H

/*
 * includes
 */
#include "orte_config.h"
#include "include/orte_constants.h"
#include "include/orte_types.h"

#include "class/ompi_list.h"
#include "mca/mca.h"
/* #include "mca/ns/ns_types.h" */
#include "mca/soh/soh.h"


/*
 * Global functions for MCA overall collective open and close
 */
#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

OMPI_DECLSPEC    int orte_soh_base_open(void);
OMPI_DECLSPEC    int orte_soh_base_select(void);
OMPI_DECLSPEC    int orte_soh_base_close(void);

/* 
 * 1. slave soh functions 
 * 
 */

OMPI_DECLSPEC    int orte_soh_base_module_slave_init_not_available (void);
OMPI_DECLSPEC    int orte_soh_base_module_slave_poll_not_available (void);
OMPI_DECLSPEC    int orte_soh_base_module_slave_do_heatbeat_not_available (void);
OMPI_DECLSPEC    int orte_soh_base_module_slave_request_monitor_not_available (orte_soh_entity_type_t type, 
                                                                               orte_soh_entity_value_t value);
OMPI_DECLSPEC    int orte_soh_base_module_slave_end_monitor_not_available (void);
OMPI_DECLSPEC    int orte_soh_base_module_slave_finalise_not_available (void);

/* 
 * 2. master soh functions 
 * 
 */

OMPI_DECLSPEC int orte_soh_base_module_master_init_not_available (void);
OMPI_DECLSPEC int orte_soh_base_module_master_poll_not_available (void);
OMPI_DECLSPEC int orte_soh_base_module_master_handle_request_not_available (void);
OMPI_DECLSPEC int orte_soh_base_module_master_add_monitor_cell_not_available (orte_cellid_t cellid);
OMPI_DECLSPEC int orte_soh_base_module_master_remove_monitor_cell_not_available (orte_cellid_t cellid);
OMPI_DECLSPEC int orte_soh_base_module_master_add_monitor_node_not_available (orte_process_name_t node);
OMPI_DECLSPEC int orte_soh_base_module_master_remove_monitor_node_not_available (orte_process_name_t node);
OMPI_DECLSPEC int orte_soh_base_module_master_add_monitor_job_not_available (orte_jobid_t jobid);
OMPI_DECLSPEC int orte_soh_base_module_master_remove_monitor_job_not_available (orte_jobid_t jobid);
OMPI_DECLSPEC int orte_soh_base_module_master_add_monitor_proc_not_available (orte_process_name_t proc);
OMPI_DECLSPEC int orte_soh_base_module_master_remove_monitor_proc_not_available (orte_process_name_t proc);
OMPI_DECLSPEC int orte_soh_base_module_master_update_state_all_not_available (void);
OMPI_DECLSPEC int orte_soh_base_module_master_update_state_cell_not_available (orte_cellid_t cellid);
OMPI_DECLSPEC int orte_soh_base_module_master_update_state_node_not_available (orte_process_name_t node);
OMPI_DECLSPEC int orte_soh_base_module_master_update_state_job_not_available (orte_jobid_t jobid );
OMPI_DECLSPEC int orte_soh_base_module_master_update_state_proc_not_available (orte_jobid_t jobid );
OMPI_DECLSPEC int orte_soh_base_module_master_pull_state_cell_not_available (orte_cellid_t cellid);
OMPI_DECLSPEC int orte_soh_base_module_master_pull_state_node_not_available (orte_process_name_t node);
OMPI_DECLSPEC int orte_soh_base_module_master_pull_state_job_not_available (orte_jobid_t jobid);
OMPI_DECLSPEC int orte_soh_base_module_master_pull_state_proc_not_available (orte_process_name_t proc);
OMPI_DECLSPEC int orte_soh_base_module_master_finalise_not_available (void);


/* 
 * 3. query soh functions 
 * 
 */
OMPI_DECLSPEC    int orte_soh_base_get_proc_soh_not_available(orte_proc_state_t *state,
                                                              int *status,
                                                              orte_process_name_t *proc);
                                                              
OMPI_DECLSPEC    int orte_soh_base_set_proc_soh_not_available(orte_process_name_t *proc,
                                             orte_proc_state_t state,
                                             int status);

OMPI_DECLSPEC    int orte_soh_base_get_node_soh_not_available(orte_node_state_t *state,
                                                      orte_cellid_t cell,
                                                      char *nodename);

OMPI_DECLSPEC    int orte_soh_base_set_node_soh_not_available(orte_cellid_t cell,
                                                    char *nodename,
                                                    orte_node_state_t state);

/* 
 * 4. communication soh functions 
 * 
 */

OMPI_DECLSPEC int orte_soh_base_module_send_monitor_request_not_available (orte_soh_entity_type_t type, orte_soh_entity_value_t value);
OMPI_DECLSPEC int orte_soh_base_module_send_unmonitor_request_not_available (orte_soh_entity_type_t type, orte_soh_entity_value_t value);
OMPI_DECLSPEC int orte_soh_base_module_send_push_state_not_available (orte_soh_entity_type_t type);
OMPI_DECLSPEC int orte_soh_base_module_send_pull_state_not_available (orte_soh_entity_type_t type);
OMPI_DECLSPEC int orte_soh_base_module_send_state_change_not_available (orte_soh_entity_type_t type, orte_soh_entity_value_t value);
OMPI_DECLSPEC int orte_soh_base_module_recv_state_not_available (orte_soh_entity_type_t type, orte_soh_entity_value_t *value);


/* 
 * 5. support soh functions 
 * 
 */

OMPI_DECLSPEC int orte_soh_base_module_finalize_soh_not_available (void);

/* 
 * 6. previous soh functions 
 * 
 */

OMPI_DECLSPEC    int orte_soh_base_update_cell_soh_not_available(orte_cellid_t cellid);
OMPI_DECLSPEC    int orte_soh_base_update_job_soh_not_available(orte_jobid_t jobid);
OMPI_DECLSPEC    int orte_soh_base_update_proc_soh_not_available(orte_process_name_t proc);


/*
 * globals that might be needed
 */

OMPI_DECLSPEC extern int orte_soh_base_output;
OMPI_DECLSPEC extern orte_soh_base_module_t orte_soh;  /* holds selected module's function pointers */
OMPI_DECLSPEC extern bool orte_soh_base_selected;

typedef struct orte_soh_base_t {
    int soh_output;
    ompi_list_t soh_components;
} orte_soh_base_t;

OMPI_DECLSPEC extern orte_soh_base_t orte_soh_base;


/*
 * external API functions will be documented in the mca/soh/soh.h file
 */

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif
#endif

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

/*
 * includes
 */
#include "ompi_config.h"

#include "class/ompi_list.h"
#include "mca/mca.h"

#include "mca/soh/base/base.h"

/* needed to get OMPI_ERR_NOT_IMPLEMENTED.. */
#include "include/constants.h"


/*
 * 1. slave SOH 
 *
 */
int orte_soh_base_module_slave_init_not_available (void)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_soh_base_module_slave_poll_not_available (void)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_soh_base_module_slave_do_heatbeat_not_available (void)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_soh_base_module_slave_request_monitor_not_available (orte_soh_entity_type_t type,
                                                                               orte_soh_entity_value_t value)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_soh_base_module_slave_end_monitor_not_available (void)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_soh_base_module_slave_finalise_not_available (void)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}



/*
 * 2. master SOH 
 *
 */

int orte_soh_base_module_master_init_not_available (void)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_soh_base_module_master_poll_not_available (void)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_soh_base_module_master_handle_request_not_available (void)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_soh_base_module_master_add_monitor_cell_not_available (orte_cellid_t cellid)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_soh_base_module_master_remove_monitor_cell_not_available (orte_cellid_t cellid)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_soh_base_module_master_add_monitor_node_not_available (orte_process_name_t node)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_soh_base_module_master_remove_monitor_node_not_available (orte_process_name_t node)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_soh_base_module_master_add_monitor_job_not_available (orte_jobid_t jobid)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_soh_base_module_master_remove_monitor_job_not_available (orte_jobid_t jobid)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_soh_base_module_master_add_monitor_proc_not_available (orte_process_name_t proc)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_soh_base_module_master_remove_monitor_proc_not_available (orte_process_name_t proc)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_soh_base_module_master_update_state_all_not_available (void)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_soh_base_module_master_update_state_cell_not_available (orte_cellid_t cellid)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_soh_base_module_master_update_state_node_not_available (orte_process_name_t node)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_soh_base_module_master_update_state_job_not_available (orte_jobid_t jobid)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_soh_base_module_master_update_state_proc_not_available (orte_jobid_t jobid)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_soh_base_module_master_pull_state_cell_not_available (orte_cellid_t cellid)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_soh_base_module_master_pull_state_node_not_available (orte_process_name_t node)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_soh_base_module_master_pull_state_job_not_available (orte_jobid_t jobid)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_soh_base_module_master_pull_state_proc_not_available (orte_process_name_t proc)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_soh_base_module_master_finalise_not_available (void)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}



/*
 * 3. query SOH 
 *
 */
int orte_soh_base_get_proc_soh_not_available(orte_proc_state_t *state,
                                             int *status,
                                             orte_process_name_t *proc)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_soh_base_set_proc_soh_not_available(orte_process_name_t *proc,
                                             orte_proc_state_t state,
                                             int status)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_soh_base_get_node_soh_not_available(orte_node_state_t *state,
                                                      orte_cellid_t cell,
                                                      char *nodename)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_soh_base_set_node_soh_not_available(orte_cellid_t cell,
                                             char *nodename,
                                             orte_node_state_t state)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

/*
 * 4. comms SOH 
 *
 */

int orte_soh_base_module_send_monitor_request_not_available (orte_soh_entity_type_t type, orte_soh_entity_value_t value)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_soh_base_module_send_unmonitor_request_not_available (orte_soh_entity_type_t type, orte_soh_entity_value_t value)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_soh_base_module_send_push_state_not_available (orte_soh_entity_type_t type)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_soh_base_module_send_pull_state_not_available (orte_soh_entity_type_t type)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_soh_base_module_send_state_change_not_available (orte_soh_entity_type_t type, orte_soh_entity_value_t value)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_soh_base_module_recv_state_not_available (orte_soh_entity_type_t type, orte_soh_entity_value_t *value)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}



/*
 * 5. other SOH 
 *
 */


int orte_soh_base_module_finalize_soh_not_available (void)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}



int orte_soh_base_update_cell_soh_not_available(orte_cellid_t cellid)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_soh_base_update_job_soh_not_available(orte_jobid_t jobid)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_soh_base_update_proc_soh_not_available(orte_process_name_t proc)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}





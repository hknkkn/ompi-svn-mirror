/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
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
 */
/** @file:
 */

#ifndef ORTE_MCA_RMAPS_PRIVATE_H
#define ORTE_MCA_RMAPS_PRIVATE_H

/*
 * includes
 */
#include "orte_config.h"
#include "orte/orte_constants.h"

#include "orte/mca/ns/ns_types.h"
#include "orte/mca/gpr/gpr_types.h"
#include "orte/mca/rml/rml_types.h"

#include "orte/mca/rmaps/rmaps.h"

/*
 * Functions for use solely within the RMAPS framework
 */
#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

/* Define the RMAPS command flag */
typedef uint8_t orte_rmaps_cmd_flag_t;
#define ORTE_RMAPS_CMD	ORTE_UINT8
    
/* define some commands */
#define ORTE_RMAPS_MAP_CMD     0x01
    
/*
 * RMAPS component/module/priority tuple
 */
struct orte_rmaps_base_cmp_t {
    /** Base object */
    opal_list_item_t super;
    /** rmaps component */
    orte_rmaps_base_component_t *component;
    /** rmaps module */
    orte_rmaps_base_module_t* module;
    /** This component's priority */
    int priority;
};
/* Convenience typedef */
typedef struct orte_rmaps_base_cmp_t orte_rmaps_base_cmp_t;
/* Class declaration */
OBJ_CLASS_DECLARATION(orte_rmaps_base_cmp_t);


/*
 * Base functions
 */

int orte_rmaps_base_map(orte_jobid_t job, char *desired_mapper);

/*
 * NO_OP functions
 */
int orte_rmaps_base_map_no_op(orte_jobid_t job, char *desired_mapper);

/*
 * communication functions
 */
int orte_rmaps_base_comm_start(void);
int orte_rmaps_base_comm_stop(void);
void orte_rmaps_base_recv(int status, orte_process_name_t* sender,
                          orte_buffer_t* buffer, orte_rml_tag_t tag,
                          void* cbdata);

/*
 * Internal support functions
 */
ORTE_DECLSPEC int orte_rmaps_base_mapped_node_query(opal_list_t* mapping_list, opal_list_t* nodes_alloc, orte_jobid_t jobid);
ORTE_DECLSPEC int orte_rmaps_base_get_map(orte_jobid_t, opal_list_t* mapping);
ORTE_DECLSPEC int orte_rmaps_base_set_map(orte_jobid_t, opal_list_t* mapping);
ORTE_DECLSPEC int orte_rmaps_base_get_node_map(orte_cellid_t, orte_jobid_t, const char*, opal_list_t* mapping);

ORTE_DECLSPEC int orte_rmaps_base_get_target_nodes(opal_list_t* node_list, orte_jobid_t jobid, orte_std_cntr_t *total_num_slots);
ORTE_DECLSPEC int orte_rmaps_base_update_node_usage(opal_list_t *nodes);
ORTE_DECLSPEC int orte_rmaps_base_get_mapped_targets(opal_list_t *mapped_node_list,
                                                     orte_app_context_t *app,
                                                     opal_list_t *master_node_list,
                                                     orte_std_cntr_t *total_num_slots);

ORTE_DECLSPEC int orte_rmaps_base_claim_slot(orte_rmaps_base_map_t *map,
                                             orte_ras_node_t *current_node,
                                             orte_jobid_t jobid, orte_vpid_t vpid,
                                             int proc_index,
                                             opal_list_t *nodes,
                                             opal_list_t *fully_used_nodes);

/** Local data type functions */
void orte_rmaps_base_std_obj_release(orte_data_value_t *value);

/* JOB_MAP */
int orte_rmaps_base_copy_map(orte_rmaps_base_map_t **dest, orte_rmaps_base_map_t *src, orte_data_type_t type);
int orte_rmaps_base_compare_map(orte_rmaps_base_map_t *value1, orte_rmaps_base_map_t *value2, orte_data_type_t type);
int orte_rmaps_base_pack_map(orte_buffer_t *buffer, void *src,
                            orte_std_cntr_t num_vals, orte_data_type_t type);
int orte_rmaps_base_print_map(char **output, char *prefix, orte_rmaps_base_map_t *src, orte_data_type_t type);
int orte_rmaps_base_size_map(size_t *size, orte_rmaps_base_map_t *src, orte_data_type_t type);
int orte_rmaps_base_unpack_map(orte_buffer_t *buffer, void *dest,
                              orte_std_cntr_t *num_vals, orte_data_type_t type);

/* MAPPED_PROC */
int orte_rmaps_base_copy_mapped_proc(orte_rmaps_base_proc_t **dest, orte_rmaps_base_proc_t *src, orte_data_type_t type);
int orte_rmaps_base_compare_mapped_proc(orte_rmaps_base_proc_t *value1, orte_rmaps_base_proc_t *value2, orte_data_type_t type);
int orte_rmaps_base_pack_mapped_proc(orte_buffer_t *buffer, void *src,
                             orte_std_cntr_t num_vals, orte_data_type_t type);
int orte_rmaps_base_print_mapped_proc(char **output, char *prefix, orte_rmaps_base_proc_t *src, orte_data_type_t type);
int orte_rmaps_base_size_mapped_proc(size_t *size, orte_rmaps_base_proc_t *src, orte_data_type_t type);
int orte_rmaps_base_unpack_mapped_proc(orte_buffer_t *buffer, void *dest,
                               orte_std_cntr_t *num_vals, orte_data_type_t type);

/* MAPPED_NODE */
int orte_rmaps_base_copy_mapped_node(orte_rmaps_base_node_t **dest, orte_rmaps_base_node_t *src, orte_data_type_t type);
int orte_rmaps_base_compare_mapped_node(orte_rmaps_base_node_t *value1, orte_rmaps_base_node_t *value2, orte_data_type_t type);
int orte_rmaps_base_pack_mapped_node(orte_buffer_t *buffer, void *src,
                             orte_std_cntr_t num_vals, orte_data_type_t type);
int orte_rmaps_base_print_mapped_node(char **output, char *prefix, orte_rmaps_base_node_t *src, orte_data_type_t type);
int orte_rmaps_base_size_mapped_node(size_t *size, orte_rmaps_base_node_t *src, orte_data_type_t type);
int orte_rmaps_base_unpack_mapped_node(orte_buffer_t *buffer, void *dest,
                               orte_std_cntr_t *num_vals, orte_data_type_t type);

/*
 * external API functions will be documented in the mca/rmaps/rmaps.h file
 */

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif
#endif

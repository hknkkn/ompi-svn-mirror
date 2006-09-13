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

#ifndef MCA_SOH_BASE_H
#define MCA_SOH_BASE_H

/*
 * includes
 */
#include "orte_config.h"
#include "orte/orte_constants.h"
#include "orte/orte_types.h"

#include "opal/class/opal_list.h"
#include "orte/dss/dss_types.h"
#include "opal/mca/mca.h"
/* #include "orte/mca/ns/ns_types.h" */
#include "orte/mca/soh/soh.h"


/*
 * Global functions for MCA overall collective open and close
 */
#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

OMPI_DECLSPEC    int orte_soh_base_open(void);
OMPI_DECLSPEC    int orte_soh_base_select(void);
OMPI_DECLSPEC    int orte_soh_base_close(void);

int orte_soh_base_get_proc_soh(orte_proc_state_t *state,
                               int *status,
                               orte_process_name_t *proc);

int orte_soh_base_set_proc_soh(orte_process_name_t *proc,
                               orte_proc_state_t state,
                               int status);

int orte_soh_base_get_node_soh_not_available(orte_node_state_t *state,
                                             orte_cellid_t cell,
                                             char *nodename);

int orte_soh_base_set_node_soh_not_available(orte_cellid_t cell,
                                             char *nodename,
                                             orte_node_state_t state);

int orte_soh_base_get_job_soh(orte_job_state_t *state,
                              orte_jobid_t jobid);

int orte_soh_base_set_job_soh(orte_jobid_t jobid,
                              orte_job_state_t state);

int orte_soh_base_begin_monitoring_not_available(orte_jobid_t job);


int orte_soh_base_module_finalize_not_available (void);

/*
 * DATA TYPE PACKING FUNCTIONS
 */
int orte_soh_base_pack_exit_code(orte_buffer_t *buffer, void *src,
                                 size_t num_vals, orte_data_type_t type);

int orte_soh_base_pack_node_state(orte_buffer_t *buffer, void *src,
                                  size_t num_vals, orte_data_type_t type);

int orte_soh_base_pack_proc_state(orte_buffer_t *buffer, void *src,
                                  size_t num_vals, orte_data_type_t type);

int orte_soh_base_pack_job_state(orte_buffer_t *buffer, void *src,
                                  size_t num_vals, orte_data_type_t type);

/*
 * DATA TYPE UNPACKING FUNCTIONS
 */
int orte_soh_base_unpack_exit_code(orte_buffer_t *buffer, void *dest,
                                 size_t *num_vals, orte_data_type_t type);

int orte_soh_base_unpack_node_state(orte_buffer_t *buffer, void *dest,
                                  size_t *num_vals, orte_data_type_t type);

int orte_soh_base_unpack_proc_state(orte_buffer_t *buffer, void *dest,
                                  size_t *num_vals, orte_data_type_t type);

int orte_soh_base_unpack_job_state(orte_buffer_t *buffer, void *dest,
                                  size_t *num_vals, orte_data_type_t type);

/*
 * DATA TYPE COMPARE FUNCTIONS
 */
int orte_soh_base_compare_exit_code(orte_exit_code_t *value1,
                                    orte_exit_code_t *value2,
                                    orte_data_type_t type);

int orte_soh_base_compare_node_state(orte_node_state_t *value1,
                                     orte_node_state_t *value2,
                                     orte_node_state_t type);

int orte_soh_base_compare_proc_state(orte_proc_state_t *value1,
                                     orte_proc_state_t *value2,
                                     orte_proc_state_t type);

int orte_soh_base_compare_job_state(orte_job_state_t *value1,
                                    orte_job_state_t *value2,
                                    orte_job_state_t type);

/*
 * DATA TYPE COPY FUNCTIONS
 */
int orte_soh_base_copy_proc_state(orte_proc_state_t **dest, orte_proc_state_t *src, orte_data_type_t type);

int orte_soh_base_copy_job_state(orte_job_state_t **dest, orte_job_state_t *src, orte_data_type_t type);

int orte_soh_base_copy_node_state(orte_node_state_t **dest, orte_node_state_t *src, orte_data_type_t type);

int orte_soh_base_copy_exit_code(orte_exit_code_t **dest, orte_exit_code_t *src, orte_data_type_t type);

/*
 * DATA TYPE PRINT FUNCTIONS
 */
int orte_soh_base_std_print(char **output, char *prefix, void *src, orte_data_type_t type);

/*
 * DATA TYPE SIZE FUNCTIONS
 */
int orte_soh_base_std_size(size_t *size, void *src, orte_data_type_t type);

/*
 * DATA TYPE RELEASE FUNCTIONS
 */
void orte_soh_base_std_release(orte_data_value_t *value);

/*
 * globals that might be needed
 */

OMPI_DECLSPEC extern int orte_soh_base_output;
OMPI_DECLSPEC extern bool orte_soh_base_selected;

typedef struct orte_soh_base_t {
    int soh_output;
    opal_list_t soh_components;
} orte_soh_base_t;

OMPI_DECLSPEC extern orte_soh_base_t orte_soh_base;


/*
 * external API functions will be documented in the mca/soh/soh.h file
 */

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif
#endif

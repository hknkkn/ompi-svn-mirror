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

#ifndef ORTE_TYPES_H
#define ORTE_TYPES_H

typedef int8_t orte_exit_code_t;
typedef int8_t orte_status_key_t;
typedef int8_t orte_node_state_t;

struct orte_process_status_t {
     int32_t rank;
     int32_t local_pid;
     char *nodename;
     orte_status_key_t status_key;
     orte_exit_code_t exit_code;
};
typedef struct orte_process_status_t orte_process_status_t;

/* constants defining runtime-related segment naming conventions for the
 * registry
 */
#define OMPI_RTE_JOB_STATUS_SEGMENT     "ompi-job-status"
#define OMPI_RTE_OOB_SEGMENT            "ompi-oob"
#define OMPI_RTE_VM_STATUS_SEGMENT      "ompi-vm-status"
#define OMPI_RTE_CELL_STATUS_SEGMENT    "ompi-cell-status"
#define OMPI_RTE_SCHED_SEGMENT          "ompi-sched"
#define OMPI_RTE_MODEX_SEGMENT          "ompi-modex"



#endif  /* ORTE_TYPES_H */

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

/**
 * Supported datatypes for messaging and storage operations.
 *
 */
typedef enum {
    ORTE_BYTE,              /**< a byte of data */
    ORTE_BOOL,              /**< boolean */
    /* all the integer flavors */
    ORTE_INT,               /**< generic integer */
    ORTE_INT8,              /**< an 8-bit integer */
    ORTE_INT16,             /**< a 16-bit integer */
    ORTE_INT32,             /**< a 32-bit integer */
    ORTE_INT64,             /**< a 64-bit integer */
    /* all the unsigned integer flavors */
    ORTE_UINT,              /**< generic unsigned integer */
    ORTE_UINT8,             /**< an 8-bit unsigned integer */
    ORTE_UINT16,            /**< a 16-bit unsigned integer */
    ORTE_UINT32,            /**< a 32-bit unsigned integer */
    ORTE_UINT64,            /**< a 64-bit unsigned integer */
    /* all the floating point flavors */
    ORTE_FLOAT,             /**< single-precision float */
    ORTE_FLOAT4,            /**< 4-byte float - usually equiv to single */
    ORTE_DOUBLE,            /**< double-precision float */
    ORTE_FLOAT8,            /**< 8-byte float - usually equiv to double */
    ORTE_LONG_DOUBLE,       /**< long-double precision float */
    ORTE_FLOAT12,           /**< 12-byte float - used as long-double on some systems */
    ORTE_FLOAT16,           /**< 16-byte float - used as long-double on some systems */
    /* string */
    ORTE_STRING,            /**< a NULL terminated string */
    /* orte-specific typedefs */
    ORTE_NAME,              /**< an ompi_process_name_t */
    ORTE_JOBID,             /**< a jobid */
    ORTE_CELLID,            /**< a cellid */
    ORTE_NODE_STATE,        /**< node status flag */
    ORTE_PROCESS_STATUS,    /**< process status key */
    ORTE_EXIT_CODE          /**< process exit code */
} orte_data_type_t ;



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

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

typedef uint8_t orte_data_type_t ;

#define    ORTE_BYTE                (orte_data_type_t)    1 /**< a byte of data */
#define    ORTE_BOOL                (orte_data_type_t)    2 /**< boolean */
#define    ORTE_STRING              (orte_data_type_t)    3 /**< a NULL terminated string */
    /* all the integer flavors */
#define    ORTE_INT                 (orte_data_type_t)    4 /**< generic integer */
#define    ORTE_INT8                (orte_data_type_t)    5 /**< an 8-bit integer */
#define    ORTE_INT16               (orte_data_type_t)    6 /**< a 16-bit integer */
#define    ORTE_INT32               (orte_data_type_t)    7 /**< a 32-bit integer */
#define    ORTE_INT64               (orte_data_type_t)    8 /**< a 64-bit integer */
    /* all the unsigned integer flavors */
#define    ORTE_UINT                (orte_data_type_t)   10 /**< generic unsigned integer */
#define    ORTE_UINT8               (orte_data_type_t)   11 /**< an 8-bit unsigned integer */
#define    ORTE_UINT16              (orte_data_type_t)   12 /**< a 16-bit unsigned integer */
#define    ORTE_UINT32              (orte_data_type_t)   13 /**< a 32-bit unsigned integer */
#define    ORTE_UINT64              (orte_data_type_t)   14 /**< a 64-bit unsigned integer */
    /* all the floating point flavors */
#define    ORTE_FLOAT               (orte_data_type_t)   15 /**< single-precision float */
#define    ORTE_FLOAT4              (orte_data_type_t)   16 /**< 4-byte float - usually equiv to single */
#define    ORTE_DOUBLE              (orte_data_type_t)   17 /**< double-precision float */
#define    ORTE_FLOAT8              (orte_data_type_t)   18 /**< 8-byte float - usually equiv to double */
#define    ORTE_LONG_DOUBLE         (orte_data_type_t)   19 /**< long-double precision float */
#define    ORTE_FLOAT12             (orte_data_type_t)   20 /**< 12-byte float - used as long-double on some systems */
#define    ORTE_FLOAT16             (orte_data_type_t)   21 /**< 16-byte float - used as long-double on some systems */
    /* orte-specific typedefs */
#define    ORTE_NAME                (orte_data_type_t)   22 /**< an ompi_process_name_t */
#define    ORTE_VPID                (orte_data_type_t)   23 /**< a vpid */
#define    ORTE_JOBID               (orte_data_type_t)   24 /**< a jobid */
#define    ORTE_CELLID              (orte_data_type_t)   25 /**< a cellid */
#define    ORTE_NODE_STATE          (orte_data_type_t)   26 /**< node status flag */
#define    ORTE_PROCESS_STATUS      (orte_data_type_t)   27 /**< process status key */
#define    ORTE_EXIT_CODE           (orte_data_type_t)   28 /**< process exit code */
#define    ORTE_BYTE_OBJECT         (orte_data_type_t)   29 /**< byte object structure */

typedef struct {
    size_t size;
    uint8_t *bytes;
} orte_byte_object_t;

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


#endif  /* ORTE_TYPES_H */

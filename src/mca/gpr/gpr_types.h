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
/** @file 
 */

/** 
 *  \brief General Purpose Registry (GPR) API
 *
 * The Open MPI General Purpose Registry (GPR) 
 * 
 * This file contains the public type definitions supporting the GPR
 */

#ifndef ORTE_GPR_TYPES_H_
#define ORTE_GPR_TYPES_H_

#include "orte_config.h"
#include "include/orte_types.h"
#include "class/ompi_object.h"

#include "mca/ns/ns_types.h"

/** Define the notification actions for the subscription system - can be OR'd
 * to create multiple actions
 */
#define ORTE_GPR_NOTIFY_MODIFICATION           (uint16_t)0x0001   /**< Notifies subscriber when object modified */
#define ORTE_GPR_NOTIFY_ADD_SUBSCRIBER         (uint16_t)0x0002   /**< Notifies subscriber when another subscriber added */
#define ORTE_GPR_NOTIFY_DELETE_ENTRY           (uint16_t)0x0004   /**< Notifies subscriber when object deleted */
#define ORTE_GPR_NOTIFY_ADD_ENTRY              (uint16_t)0x0008   /**< Notifies subscriber when object added */
#define ORTE_GPR_NOTIFY_ON_STARTUP             (uint16_t)0x0010   /**< Provide me with startup message - no data */
#define ORTE_GPR_NOTIFY_ON_SHUTDOWN            (uint16_t)0x0020   /**< Provide me with shutdown message - no data */
#define ORTE_GPR_NOTIFY_PRE_EXISTING           (uint16_t)0x0040   /**< Provide list of all pre-existing data */
#define ORTE_GPR_NOTIFY_INCLUDE_STARTUP_DATA   (uint16_t)0x0080   /**< Provide data with startup message */
#define ORTE_GPR_NOTIFY_INCLUDE_SHUTDOWN_DATA  (uint16_t)0x0100   /**< Provide data with shutdown message */
#define ORTE_GPR_NOTIFY_ONE_SHOT               (uint16_t)0x0200   /**< Only trigger once - then delete subscription */
#define ORTE_GPR_NOTIFY_ALL                    (uint16_t)0x8000   /**< Notifies subscriber upon any action */

typedef uint16_t orte_gpr_notify_action_t;

typedef uint32_t orte_gpr_notify_id_t;
#define ORTE_GPR_NOTIFY_ID_MAX UINT32_MAX

/*
 * Define synchro mode flags - can be OR'd to create multiple actions
 */
#define ORTE_GPR_SYNCHRO_MODE_ASCENDING   (uint16_t)0x0001   /**< Notify when trigger is reached, ascending mode */
#define ORTE_GPR_SYNCHRO_MODE_DESCENDING  (uint16_t)0x0002   /**< Notify when trigger is reached, descending mode */
#define ORTE_GPR_SYNCHRO_MODE_LEVEL       (uint16_t)0x0004   /**< Notify when trigger is reached, regardless of direction */
#define ORTE_GPR_SYNCHRO_MODE_GT_EQUAL    (uint16_t)0x0008   /**< Notify if level greater than or equal */
#define ORTE_GPR_SYNCHRO_MODE_LT_EQUAL    (uint16_t)0x0010   /**< Notify if level less than or equal */
#define ORTE_GPR_SYNCHRO_MODE_CONTINUOUS  (uint16_t)0x0020   /**< Notify whenever conditions are met */
#define ORTE_GPR_SYNCHRO_MODE_ONE_SHOT    (uint16_t)0x0040   /**< Fire once, then terminate synchro command */
#define ORTE_GPR_SYNCHRO_MODE_STARTUP     (uint16_t)0x0080   /**< Indicates associated with application startup */

typedef uint16_t orte_gpr_synchro_mode_t;

/*
 * Define flag values for remote commands - normally used internally, but required
 * here to allow for decoding of notify messages
 */
#define ORTE_GPR_DELETE_SEGMENT_CMD     (uint16_t)0x0001
#define ORTE_GPR_PUT_CMD                (uint16_t)0x0002
#define ORTE_GPR_DELETE_ENTRIES_CMD     (uint16_t)0x0004
#define ORTE_GPR_INDEX_CMD              (uint16_t)0x0008
#define ORTE_GPR_SUBSCRIBE_CMD          (uint16_t)0x0010
#define ORTE_GPR_UNSUBSCRIBE_CMD        (uint16_t)0x0020
#define ORTE_GPR_SYNCHRO_CMD            (uint16_t)0x0040
#define ORTE_GPR_CANCEL_SYNCHRO_CMD     (uint16_t)0x0080
#define ORTE_GPR_GET_CMD                (uint16_t)0x0100
#define ORTE_GPR_TEST_INTERNALS_CMD     (uint16_t)0x0200
#define ORTE_GPR_NOTIFY_CMD             (uint16_t)0x0400
#define ORTE_GPR_DUMP_CMD               (uint16_t)0x0800
#define ORTE_GPR_ASSIGN_OWNERSHIP_CMD   (uint16_t)0x1000
#define ORTE_GPR_NOTIFY_ON_CMD          (uint16_t)0x2000
#define ORTE_GPR_NOTIFY_OFF_CMD         (uint16_t)0x4000
#define ORTE_GPR_COMPOUND_CMD           (uint16_t)0x8000
#define ORTE_GPR_GET_STARTUP_MSG_CMD    (uint16_t)0x8020
#define ORTE_GPR_GET_SHUTDOWN_MSG_CMD   (uint16_t)0x8040
#define ORTE_GPR_TRIGGERS_ACTIVE_CMD    (uint16_t)0x8080
#define ORTE_GPR_TRIGGERS_INACTIVE_CMD  (uint16_t)0x8100
#define ORTE_GPR_CLEANUP_JOB_CMD        (uint16_t)0x8200
#define ORTE_GPR_CLEANUP_PROC_CMD       (uint16_t)0x8400
#define ORTE_GPR_ERROR                  (uint16_t)0xffff

typedef uint16_t orte_gpr_cmd_flag_t;


/** Define the addressing mode bit-masks for registry operations.
 */
#define ORTE_GPR_OVERWRITE  (uint16_t)0x0001   /**< Overwrite Permission */
#define ORTE_GPR_AND        (uint16_t)0x0002   /**< AND tokens together for search results */
#define ORTE_GPR_OR         (uint16_t)0x0004   /**< OR tokens for search results */
#define ORTE_GPR_XAND       (uint16_t)0x0008   /**< All tokens required, nothing else allowed */
#define ORTE_GPR_XOR        (uint16_t)0x0010   /**< Any one of the tokens required, nothing else allowed */

typedef uint16_t orte_gpr_addr_mode_t;

/*
 * typedefs
 */
typedef union {                             /* shared storage for the value */
    char *strptr;
    uint8_t ui8;
    uint16_t ui16;
    uint32_t ui32;
#ifdef HAVE_I64
    uint64_t ui64;
#endif
    int8_t i8;
    int16_t i16;
    int32_t i32;
#ifdef HAVE_I64
    int64_t i64;
#endif
    orte_byte_object_t byteobject;
    orte_process_name_t proc;
    orte_jobid_t jobid;
    orte_node_state_t node_state;
    orte_status_key_t proc_status;
    orte_exit_code_t exit_code;
} orte_gpr_value_union_t;

 /*
  * Key-value pairs for registry operations
  */
typedef struct {
    ompi_object_t super;                /* required for this to be an object */
    char *key;                          /* string key for this value */
    orte_data_type_t type;              /* the type of value stored */
    orte_gpr_value_union_t value;
} orte_gpr_keyval_t;

OMPI_DECLSPEC OBJ_CLASS_DECLARATION(orte_gpr_keyval_t);


/** Return value structure for registry requests.
 * A request for information stored within the registry returns an array of values that
 * correspond to the provided tokens.
 * Each element in the array contains an array of keyval objects - note that the array
 * contains \em copies of the data in the registry. This prevents inadvertent
 * modification of the registry, but requires the recipient to release the data's
 * memory when done.
 */
typedef struct {
    ompi_object_t super;                    /**< Makes this an object */
    uint32_t cnt;                           /**< Number of keyval objects returned */
    orte_gpr_keyval_t **keyvals;             /**< Contiguous array of keyval object pointers */
    char *segment;                          /**< Name of the segment this came from */
    uint32_t num_tokens;                    /**< Number of tokens used to recover data */
    char **tokens;                          /**< List of tokens that described this data */
} orte_gpr_value_t;

OMPI_DECLSPEC OBJ_CLASS_DECLARATION(orte_gpr_value_t);


/** Return message for notify requests
 */
typedef struct {
    ompi_object_t super;                        /**< Make this an object */
    orte_gpr_notify_id_t idtag;                 /**< Referenced notify request */
    char *segment;                              /**< Name of originating segment */
    size_t cnt;                                 /**< number of registry value objects */
    orte_gpr_value_t *values;                   /**< Contiguous array of gpr values */
    orte_gpr_cmd_flag_t cmd;                    /**< command that generated the notify msg */
    union {
        orte_gpr_notify_action_t trig_action;   /**< If subscription, action that triggered message */
        orte_gpr_synchro_mode_t trig_synchro;   /**< If synchro, action that triggered message */
        orte_exit_code_t exit_code;             /**< status code of command that was executed */
        int32_t cmd_return;                     /**< return value from gpr commands in compound cmd */
    } flag;
} orte_gpr_notify_message_t;

OMPI_DECLSPEC OBJ_CLASS_DECLARATION(orte_gpr_notify_message_t);

/** Notify callback function */
typedef void (*orte_gpr_notify_cb_fn_t)(orte_gpr_notify_message_t *notify_msg, void *user_tag);


/** Return value for test results on internal test
 */
struct orte_gpr_internal_test_results_t {
    ompi_list_item_t item;          /**< Allows this item to be placed on a list */
    char *test;
    char *message;
    int exit_code;
};
typedef struct orte_gpr_internal_test_results_t orte_gpr_internal_test_results_t;

OMPI_DECLSPEC OBJ_CLASS_DECLARATION(orte_gpr_internal_test_results_t);

#endif /* GPR_TYPES_H */

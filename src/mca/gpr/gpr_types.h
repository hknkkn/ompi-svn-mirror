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
#include "class/ompi_list.h"

#include "mca/ns/ns_types.h"

/** Define the notification actions for the subscription system - can be OR'd
 * to create multiple actions
 */
#define ORTE_REGISTRY_NOTIFY_MODIFICATION           (uint16_t)0x0001   /**< Notifies subscriber when object modified */
#define ORTE_REGISTRY_NOTIFY_ADD_SUBSCRIBER         (uint16_t)0x0002   /**< Notifies subscriber when another subscriber added */
#define ORTE_REGISTRY_NOTIFY_DELETE_ENTRY           (uint16_t)0x0004   /**< Notifies subscriber when object deleted */
#define ORTE_REGISTRY_NOTIFY_ADD_ENTRY              (uint16_t)0x0008   /**< Notifies subscriber when object added */
#define ORTE_REGISTRY_NOTIFY_ON_STARTUP             (uint16_t)0x0010   /**< Provide me with startup message - no data */
#define ORTE_REGISTRY_NOTIFY_ON_SHUTDOWN            (uint16_t)0x0020   /**< Provide me with shutdown message - no data */
#define ORTE_REGISTRY_NOTIFY_PRE_EXISTING           (uint16_t)0x0040   /**< Provide list of all pre-existing data */
#define ORTE_REGISTRY_NOTIFY_INCLUDE_STARTUP_DATA   (uint16_t)0x0080   /**< Provide data with startup message */
#define ORTE_REGISTRY_NOTIFY_INCLUDE_SHUTDOWN_DATA  (uint16_t)0x0100   /**< Provide data with shutdown message */
#define ORTE_REGISTRY_NOTIFY_ONE_SHOT               (uint16_t)0x0200   /**< Only trigger once - then delete subscription */
#define ORTE_REGISTRY_NOTIFY_ALL                    (uint16_t)0x8000   /**< Notifies subscriber upon any action */

typedef uint16_t orte_registry_notify_action_t;

typedef uint32_t orte_registry_notify_id_t;
#define ORTE_REGISTRY_NOTIFY_ID_MAX UINT32_MAX

/*
 * Define synchro mode flags - can be OR'd to create multiple actions
 */
#define ORTE_REGISTRY_SYNCHRO_MODE_ASCENDING   (uint16_t)0x0001   /**< Notify when trigger is reached, ascending mode */
#define ORTE_REGISTRY_SYNCHRO_MODE_DESCENDING  (uint16_t)0x0002   /**< Notify when trigger is reached, descending mode */
#define ORTE_REGISTRY_SYNCHRO_MODE_LEVEL       (uint16_t)0x0004   /**< Notify when trigger is reached, regardless of direction */
#define ORTE_REGISTRY_SYNCHRO_MODE_GT_EQUAL    (uint16_t)0x0008   /**< Notify if level greater than or equal */
#define ORTE_REGISTRY_SYNCHRO_MODE_LT_EQUAL    (uint16_t)0x0010   /**< Notify if level less than or equal */
#define ORTE_REGISTRY_SYNCHRO_MODE_CONTINUOUS  (uint16_t)0x0020   /**< Notify whenever conditions are met */
#define ORTE_REGISTRY_SYNCHRO_MODE_ONE_SHOT    (uint16_t)0x0040   /**< Fire once, then terminate synchro command */
#define ORTE_REGISTRY_SYNCHRO_MODE_STARTUP     (uint16_t)0x0080   /**< Indicates associated with application startup */

typedef uint16_t orte_registry_synchro_mode_t;

/*
 * Define flag values for remote commands - normally used internally, but required
 * here to allow for decoding of notify messages
 */
#define ORTE_GPR_DELETE_SEGMENT_CMD     (uint16_t)0x0001
#define ORTE_GPR_PUT_CMD                (uint16_t)0x0002
#define ORTE_GPR_DELETE_OBJECT_CMD      (uint16_t)0x0004
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

/** Return value for notify requests
 */
typedef struct {
    ompi_object_t super;                         /**< Make this an object */
    char *segment;                               /**< Name of originating segment */
    orte_gpr_cmd_flag_t cmd;                     /**< command that generated the notify msg */
    int32_t cnt;                                 /**< Number of keyval strucs returned */
    orte_registry_keyval_t *keyvals;             /**< Array of keyval structures */
    union {
        orte_registry_notify_action_t trig_action;   /**< If subscription, action that triggered message */
        orte_registry_synchro_mode_t trig_synchro;   /**< If synchro, action that triggered message */
        orte_exit_code_t status_code;                /**< status code of command that was executed */
    } flag;
    uint32_t num_tokens;                         /**< Number of tokens used to recover data in list */
    char **tokens;                               /**< List of tokens used to recover data in list */
} orte_registry_notify_message_t;

OMPI_DECLSPEC OBJ_CLASS_DECLARATION(orte_registry_notify_message_t);

/** Notify callback function */
typedef void (*orte_registry_notify_cb_fn_t)(orte_registry_notify_message_t *notify_msg, void *user_tag);



/** Define the addressing mode bit-masks for registry operations.
 */
#define ORTE_REGISTRY_OVERWRITE  (uint16_t)0x0001   /**< Overwrite Permission */
#define ORTE_REGISTRY_AND        (uint16_t)0x0002   /**< AND tokens together for search results */
#define ORTE_REGISTRY_OR         (uint16_t)0x0004   /**< OR tokens for search results */
#define ORTE_REGISTRY_XAND       (uint16_t)0x0008   /**< All tokens required, nothing else allowed */
#define ORTE_REGISTRY_XOR        (uint16_t)0x0010   /**< Any one of the tokens required, nothing else allowed */

typedef uint16_t orte_registry_addr_mode_t;

/** Define flag values for requesting return data from compound commands
 */

#define ORTE_REGISTRY_RETURN_REQUESTED      true    /**< Return information from compound command */
#define ORTE_REGISTRY_NO_RETURN_REQUESTED   false   /**< Do not return information from compound cmd */


/*
 * typedefs
 */
 
 /*
  * Key-value pairs for registry operations
  */
typedef struct {
    char *key;
    orte_data_type_t type;
    union {
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
        orte_process_name_t proc;
        orte_jobid_t jobid;
        orte_node_state_t node_state;
        orte_process_status_t proc_status;
        orte_exit_code_t exit_code;
    } value;
} orte_registry_keyval_t;


/** Return value structure for registry requests.
 * A request for information stored within the registry returns a linked list of values that
 * correspond to the provided tokens. The linked list is terminated by a "next" value of NULL.
 * Each link in the list contains a pointer to a copy of the registry object, and the size
 * of that object in bytes. Note that the pointer is to a \em copy of the object, and not
 * to the registry object itself. This prevents inadvertent modification of the registry, but
 * may require the recipient to release the structure's memory when done.
 */
typedef struct orte_registry_value_t {
    ompi_list_item_t item;                    /**< Allows this item to be placed on a list */
    orte_registry_keyval_t keyval;       /**< keyvalue pair */
} orte_registry_value_t;

OMPI_DECLSPEC OBJ_CLASS_DECLARATION(orte_registry_value_t);

/** Return value structure for index requests.
 */
struct orte_registry_index_value_t {
    ompi_list_item_t item;           /**< Allows this item to be placed on a list */
    char *token;                     /**< Pointer to the token string */
};
typedef struct orte_registry_index_value_t orte_registry_index_value_t;

OMPI_DECLSPEC OBJ_CLASS_DECLARATION(orte_registry_index_value_t);

/** Return value structure for compound registry commands.
 * A compound registry command contains multiple registry commands, all transferred
 * in a single communication. Because of this, data returned by the individual
 * commands within the compound command must be separated out so it can be clearly
 * retrieved. This structure provides a wrapper for data returned by each of the
 * individual commands.
 */
struct orte_registry_compound_cmd_results_t {
    ompi_list_item_t item;          /**< Allows this item to be placed on a list */
    int32_t status_code;            /**< Status code resulting from the command */
    ompi_list_t data;              /**< Any returned data coming from the command */
};
typedef struct orte_registry_compound_cmd_results_t orte_registry_compound_cmd_results_t;

OBJ_CLASS_DECLARATION(orte_registry_compound_cmd_results_t);


/** Return value for test results on internal test
 */
struct orte_registry_internal_test_results_t {
    ompi_list_item_t item;          /**< Allows this item to be placed on a list */
    char *test;
    char *message;
};
typedef struct orte_registry_internal_test_results_t orte_registry_internal_test_results_t;

OMPI_DECLSPEC OBJ_CLASS_DECLARATION(orte_registry_internal_test_results_t);

#endif /* GPR_TYPES_H */

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
#include "include/orte_schema.h"
#include "class/ompi_object.h"

#include "mca/ns/ns_types.h"
#include "mca/soh/soh_types.h"

/** Define the notify actions for the subscription system - can be OR'd
 * to create multiple actions
 */
#define ORTE_GPR_NOTIFY_NONE            (uint16_t)0x0000   /**< No trigger action */
#define ORTE_GPR_NOTIFY_VALUE_CHG_TO    (uint16_t)0x0001   /**< Notifies subscriber when value changes to specified value */
#define ORTE_GPR_NOTIFY_VALUE_CHG_FRM   (uint16_t)0x0002   /**< Notifies subscriber when value changes away from specified value */
#define ORTE_GPR_NOTIFY_VALUE_CHG       (uint16_t)0x0003   /**< Notifies subscriber when value changes */
#define ORTE_GPR_NOTIFY_ADD_ENTRY       (uint16_t)0x0004   /**< Notifies subscriber when entry added */
#define ORTE_GPR_NOTIFY_DEL_ENTRY       (uint16_t)0x0008   /**< Notifies subscriber when entry deleted */
#define ORTE_GPR_NOTIFY_ALL             (uint16_t)0x000f   /**< Notifies subscriber upon any action */
#define ORTE_GPR_NOTIFY_PRE_EXISTING    (uint16_t)0x0010   /**< Provide list of all pre-existing data */
#define ORTE_GPR_TRIG_ONE_SHOT          (uint16_t)0x0100   /**< Only trigger once - then delete subscription */
#define ORTE_GPR_TRIG_AT_LEVEL          (uint16_t)0x0200   /**< Trigger whenever count reaches specified level */
#define ORTE_GPR_TRIG_CMP_LEVELS        (uint16_t)0x0400   /**< Trigger when all the specified values are equal */
#define ORTE_GPR_TRIG_MONITOR_ONLY      (uint16_t)0x0800   /**< Monitor the provided trigger keyval - counting done by someone else */
#define ORTE_GPR_TRIG_NOTIFY_START      (uint16_t)0x1000   /**< Notifies are off when subscription entered - turned on when trigger fires */
#define ORTE_GPR_TRIG_INCLUDE_DATA      (uint16_t)0x2000   /**< Include the trigger data in the trigger msg */
#define ORTE_GPR_TRIG_ALL_AT            (uint16_t)0xdb00   /**< Use all trig defs except include trig data with AT - a typical situation */
#define ORTE_GPR_TRIG_ALL_CMP           (uint16_t)0xdd00   /**< Use all trig defs except include trig data with CMP */
typedef uint16_t orte_gpr_notify_action_t;

typedef int32_t orte_gpr_notify_id_t;
#define ORTE_GPR_NOTIFY_ID_MAX INT32_MAX

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
#define ORTE_GPR_GET_CMD                (uint16_t)0x0100
#define ORTE_GPR_TEST_INTERNALS_CMD     (uint16_t)0x0200
#define ORTE_GPR_NOTIFY_CMD             (uint16_t)0x0400
#define ORTE_GPR_DUMP_CMD               (uint16_t)0x0800
#define ORTE_GPR_INCREMENT_VALUE_CMD    (uint16_t)0x2000
#define ORTE_GPR_DECREMENT_VALUE_CMD    (uint16_t)0x4000
#define ORTE_GPR_COMPOUND_CMD           (uint16_t)0x8000
#define ORTE_GPR_CLEANUP_JOB_CMD        (uint16_t)0x8200
#define ORTE_GPR_CLEANUP_PROC_CMD       (uint16_t)0x8400
#define ORTE_GPR_ERROR                  (uint16_t)0xffff

typedef uint16_t orte_gpr_cmd_flag_t;


/** Define the addressing mode bit-masks for registry operations.
 *
 * Token modes
 */
#define ORTE_GPR_TOKENS_AND     (uint16_t)0x0001    /**< AND tokens together for search results */
#define ORTE_GPR_TOKENS_OR      (uint16_t)0x0002    /**< OR tokens for search results */
#define ORTE_GPR_TOKENS_XAND    (uint16_t)0x0004    /**< All tokens required, nothing else allowed */
#define ORTE_GPR_TOKENS_XOR     (uint16_t)0x0008    /**< Any one of the tokens required, nothing else allowed */
#define ORTE_GPR_TOKENS_NOT     (uint16_t)0x0040    /**< Everything except those that meet specs */
/*
 * Key modes
 */
#define ORTE_GPR_KEYS_AND       (uint16_t)0x0100    /**< AND keys together */
#define ORTE_GPR_KEYS_OR        (uint16_t)0x0200    /**< OR keys together */
#define ORTE_GPR_KEYS_XAND      (uint16_t)0x0400    /**< All keys required, nothing else allowed */
#define ORTE_GPR_KEYS_XOR       (uint16_t)0x0800    /**< Any one of the keys required, nothing else allowed */
#define ORTE_GPR_KEYS_NOT       (uint16_t)0x4000    /**< Everything except those that meet specs */
/*
 * General modes
 */
#define ORTE_GPR_OVERWRITE      (uint16_t)0x8000    /**< Allow overwrite of existing info */
#define ORTE_GPR_NO_OVERWRITE   (uint16_t)0x0000    /**< Do not allow overwrite of existing info */

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
    orte_vpid_t vpid;
    orte_jobid_t jobid;
    orte_cellid_t cellid;
    orte_node_state_t node_state;
    orte_proc_state_t proc_state;
    orte_app_context_t *app_context;
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
    orte_gpr_addr_mode_t addr_mode;         /**< Address mode for combining keys/tokens */
    int32_t cnt;                            /**< Number of keyval objects returned */
    orte_gpr_keyval_t **keyvals;            /**< Contiguous array of keyval object pointers */
    char *segment;                          /**< Name of the segment this came from */
    int32_t num_tokens;                     /**< Number of tokens used to recover data */
    char **tokens;                          /**< List of tokens that described this data */
} orte_gpr_value_t;

OMPI_DECLSPEC OBJ_CLASS_DECLARATION(orte_gpr_value_t);


/** Return message for notify requests
 */
typedef struct {
    ompi_object_t super;                        /**< Make this an object */
    orte_gpr_notify_id_t idtag;                 /**< Referenced notify request */
    int32_t cnt;                                 /**< number of registry value objects */
    orte_gpr_value_t **values;                  /**< Contiguous array of pointers to gpr value objects */
} orte_gpr_notify_message_t;

OMPI_DECLSPEC OBJ_CLASS_DECLARATION(orte_gpr_notify_message_t);

/** Notify callback function 
 * notify_msg = message containing data provided by trigger
 * 
 * user_tag = whatever tag data the user provided when filing the subscription
 */
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

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
 * @page gpr_api
 */

/** 
 *  \brief General Purpose Registry (GPR) API
 *
 * The Open MPI General Purpose Registry (GPR) 
 */

#ifndef MCA_GPR_TYPES_H_
#define MCA_GPR_TYPES_H_

/** Define the notification actions for the subscription system
 */
#define OMPI_REGISTRY_NOTIFY_NONE                   (uint16_t)0x0000   /**< Null case */
#define OMPI_REGISTRY_NOTIFY_MODIFICATION           (uint16_t)0x0001   /**< Notifies subscriber when object modified */
#define OMPI_REGISTRY_NOTIFY_ADD_SUBSCRIBER         (uint16_t)0x0002   /**< Notifies subscriber when another subscriber added */
#define OMPI_REGISTRY_NOTIFY_DELETE_ENTRY           (uint16_t)0x0004   /**< Notifies subscriber when object deleted */
#define OMPI_REGISTRY_NOTIFY_ADD_ENTRY              (uint16_t)0x0008   /**< Notifies subscriber when object added */
#define OMPI_REGISTRY_NOTIFY_ON_STARTUP             (uint16_t)0x0010   /**< Provide me with startup message - no data */
#define OMPI_REGISTRY_NOTIFY_ON_SHUTDOWN            (uint16_t)0x0020   /**< Provide me with shutdown message - no data */
#define OMPI_REGISTRY_NOTIFY_PRE_EXISTING           (uint16_t)0x0040   /**< Provide list of all pre-existing data */
#define OMPI_REGISTRY_NOTIFY_INCLUDE_STARTUP_DATA   (uint16_t)0x0080   /**< Provide data with startup message */
#define OMPI_REGISTRY_NOTIFY_INCLUDE_SHUTDOWN_DATA  (uint16_t)0x0100   /**< Provide data with shutdown message */
#define OMPI_REGISTRY_NOTIFY_ONE_SHOT               (uint16_t)0x0200   /**< Only trigger once - then delete subscription */
#define OMPI_REGISTRY_NOTIFY_ALL                    (uint16_t)0x8000   /**< Notifies subscriber upon any action */

typedef uint16_t ompi_registry_notify_action_t;

typedef uint32_t ompi_registry_notify_id_t;
#define OMPI_REGISTRY_NOTIFY_ID_MAX UINT32_MAX

/*
 * Define synchro mode flags
 */
#define OMPI_REGISTRY_SYNCHRO_MODE_NONE        (uint16_t)0x0000   /**< No synchronization */
#define OMPI_REGISTRY_SYNCHRO_MODE_ASCENDING   (uint16_t)0x0001   /**< Notify when trigger is reached, ascending mode */
#define OMPI_REGISTRY_SYNCHRO_MODE_DESCENDING  (uint16_t)0x0002   /**< Notify when trigger is reached, descending mode */
#define OMPI_REGISTRY_SYNCHRO_MODE_LEVEL       (uint16_t)0x0004   /**< Notify when trigger is reached, regardless of direction */
#define OMPI_REGISTRY_SYNCHRO_MODE_GT_EQUAL    (uint16_t)0x0008   /**< Notify if level greater than or equal */
#define OMPI_REGISTRY_SYNCHRO_MODE_LT_EQUAL    (uint16_t)0x0010   /**< Notify if level less than or equal */
#define OMPI_REGISTRY_SYNCHRO_MODE_CONTINUOUS  (uint16_t)0x0020   /**< Notify whenever conditions are met */
#define OMPI_REGISTRY_SYNCHRO_MODE_ONE_SHOT    (uint16_t)0x0040   /**< Fire once, then terminate synchro command */
#define OMPI_REGISTRY_SYNCHRO_MODE_STARTUP     (uint16_t)0x0080   /**< Indicates associated with application startup */

typedef uint16_t ompi_registry_synchro_mode_t;

/** Return value for notify requests
 */
struct ompi_registry_notify_message_t {
    ompi_object_t super;                         /**< Make this an object */
    char *segment;                               /**< Name of originating segment */
    orte_jobid_t owning_job;                     /**< Job that owns that segment */
    ompi_list_t data;                            /**< List of data objects */
    ompi_registry_notify_action_t trig_action;   /**< If subscription, action that triggered message */
    ompi_registry_synchro_mode_t trig_synchro;   /**< If synchro, action that triggered message */
    uint32_t num_tokens;                         /**< Number of tokens in subscription/synchro */
    char **tokens;                               /**< List of tokens in subscription/synchro */
};
typedef struct ompi_registry_notify_message_t ompi_registry_notify_message_t;

OMPI_DECLSPEC OBJ_CLASS_DECLARATION(ompi_registry_notify_message_t);

/** Notify callback function */
typedef void (*ompi_registry_notify_cb_fn_t)(ompi_registry_notify_message_t *notify_msg, void *user_tag);



/** Define the addressing mode bit-masks for registry operations.
 */
#define OMPI_REGISTRY_NONE       (uint16_t)0x0000   /**< None */
#define OMPI_REGISTRY_OVERWRITE  (uint16_t)0x0001   /**< Overwrite Permission */
#define OMPI_REGISTRY_AND        (uint16_t)0x0002   /**< AND tokens together for search results */
#define OMPI_REGISTRY_OR         (uint16_t)0x0004   /**< OR tokens for search results */
#define OMPI_REGISTRY_XAND       (uint16_t)0x0008   /**< All tokens required, nothing else allowed */
#define OMPI_REGISTRY_XOR        (uint16_t)0x0010   /**< Any one of the tokens required, nothing else allowed */

typedef uint16_t ompi_registry_mode_t;

/** Define flag values for requesting return data from compound commands
 */

#define OMPI_REGISTRY_RETURN_REQUESTED      true    /**< Return information from compound command */
#define OMPI_REGISTRY_NO_RETURN_REQUESTED   false   /**< Do not return information from compound cmd */


/*
 * typedefs
 */

typedef void* ompi_registry_object_t;
typedef uint32_t ompi_registry_object_size_t;

/*
 * structures
 */

/** Return value structure for registry requests.
 * A request for information stored within the registry returns a linked list of values that
 * correspond to the provided tokens. The linked list is terminated by a "next" value of NULL.
 * Each link in the list contains a pointer to a copy of the registry object, and the size
 * of that object in bytes. Note that the pointer is to a \em copy of the object, and not
 * to the registry object itself. This prevents inadvertent modification of the registry, but
 * may require the recipient to release the structure's memory when done.
 */
struct ompi_registry_value_t {
    ompi_list_item_t item;                    /**< Allows this item to be placed on a list */
    ompi_registry_object_t object;           /**< Pointer to object being returned */
    ompi_registry_object_size_t object_size;  /**< Size of returned object, in bytes */
};
typedef struct ompi_registry_value_t ompi_registry_value_t;

OMPI_DECLSPEC OBJ_CLASS_DECLARATION(ompi_registry_value_t);

/** Return value structure for index requests.
 */
struct ompi_registry_index_value_t {
    ompi_list_item_t item;           /**< Allows this item to be placed on a list */
    char *token;                     /**< Pointer to the token string */
};
typedef struct ompi_registry_index_value_t ompi_registry_index_value_t;

OMPI_DECLSPEC OBJ_CLASS_DECLARATION(ompi_registry_index_value_t);

/** Return value structure for compound registry commands.
 * A compound registry command contains multiple registry commands, all transferred
 * in a single communication. Because of this, data returned by the individual
 * commands within the compound command must be separated out so it can be clearly
 * retrieved. This structure provides a wrapper for data returned by each of the
 * individual commands.
 */
struct ompi_registry_compound_cmd_results_t {
    ompi_list_item_t item;          /**< Allows this item to be placed on a list */
    int32_t status_code;            /**< Status code resulting from the command */
    ompi_list_t data;              /**< Any returned data coming from the command */
};
typedef struct ompi_registry_compound_cmd_results_t ompi_registry_compound_cmd_results_t;

OBJ_CLASS_DECLARATION(ompi_registry_compound_cmd_results_t);


/** Return value for test results on internal test
 */
struct ompi_registry_internal_test_results_t {
    ompi_list_item_t item;          /**< Allows this item to be placed on a list */
    char *test;
    char *message;
};
typedef struct ompi_registry_internal_test_results_t ompi_registry_internal_test_results_t;

OMPI_DECLSPEC OBJ_CLASS_DECLARATION(ompi_registry_internal_test_results_t);

#endif /* GPR_TYPES_H */

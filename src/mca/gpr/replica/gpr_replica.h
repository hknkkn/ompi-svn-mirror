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
 *
 */
/** @file 
 */

#ifndef ORTE_GPR_REPLICA_H
#define ORTE_GPR_REPLICA_H


#include "orte_config.h"

#include <time.h>

#include "class/orte_pointer_array.h"

#include "threads/mutex.h"
#include "threads/condition.h"

#include "mca/ns/ns_types.h"

#include "mca/gpr/base/base.h"

/*
 * typedefs needed in replica component
 */

#define ORTE_GPR_REPLICA_MAX_SIZE UINT32_MAX
#define ORTE_GPR_REPLICA_BLOCK_SIZE 100

typedef int32_t orte_gpr_replica_itag_t;
#define ORTE_GPR_REPLICA_ITAG_MAX INT32_MAX

typedef struct {
    int debug;
    uint32_t block_size;
    uint32_t max_size;
    ompi_mutex_t mutex;
    bool compound_cmd_mode;
    bool exec_compound_cmd_mode;
    orte_buffer_t *compound_cmd;
    ompi_mutex_t wait_for_compound_mutex;
    ompi_condition_t compound_cmd_condition;
    int compound_cmd_waiting;
} orte_gpr_replica_globals_t;


/** Dictionary of string-itag pairs.
 * This structure is used to create a linked list of string-itag pairs. All calls to
 * registry functions pass character strings for programming clarity - the replica_dict
 * structure is used to translate those strings into an integer itag value, thus allowing
 * for faster searches of the registry.
 */
struct orte_gpr_replica_dict_t {
    char *entry;                   /**< Char string that defines the itag */
    orte_gpr_replica_itag_t itag;  /**< Numerical value assigned by registry to represent string */
};
typedef struct orte_gpr_replica_dict_t orte_gpr_replica_dict_t;

/*
 * Registry "head"
 * The registry "head" contains:
 * 
 * (2) the next available itag for the segment dictionary.
 * 
 * (3) a managed array of pointers to segment objects.
 *
 * (4) a managed array of pointers to triggers acting on the entire registry
 * 
 */
struct orte_gpr_replica_t {
    orte_pointer_array_t *segments;  /**< Managed array of pointers to segment objects */
    orte_pointer_array_t *triggers;  /**< Managed array of pointers to triggers */
    ompi_list_t callbacks;          /**< List of callbacks to be processed */
    ompi_list_t notify_offs;        /**< List of processes with triggers inactive */
};
typedef struct orte_gpr_replica_t orte_gpr_replica_t;


/** Registry segment definition.
 * The registry is subdivided into segments, each defining a unique domain. The "universe" segment
 * is automatically created to allow the exchange of information supporting universe-level functions.
 * Similarly, a segment is automatically created for each MPI CommWorld within the universe - the
 * name for that segment is stored in each CommWorld's ompi_system_info structure so program
 * elements within that CommWorld can access it. The segment structure serves as the "head" of a linked
 * list of registry elements for that segment. Each segment also holds its own token-itag dictionary
 * to avoid naming conflicts between tokens from CommWorlds sharing a given universe.
 */
struct orte_gpr_replica_segment_t {
    ompi_object_t super;                /**< Make this an object */
    char *name;                         /**< Name of the segment */
    orte_gpr_replica_itag_t itag;       /**< itag of this segment */
    orte_pointer_array_t *dict;         /**< Managed array of dict structs */
    orte_pointer_array_t *containers;   /**< Managed array of pointers to containers on this segment */
    orte_gpr_notify_id_t *triggers;     /**< Array of indices for triggers on this segment */
    int num_trigs;                      /**< Number of triggers in array */
    bool triggers_active;               /**< Indicates if triggers are active or not */
};
typedef struct orte_gpr_replica_segment_t orte_gpr_replica_segment_t;

OBJ_CLASS_DECLARATION(orte_gpr_replica_segment_t);


/** The core registry structure.
 * Each segment of the registry contains an array of registry containers, each composed
 * of:
 * 
 * (1) An object structure that allows the structure to be treated with the OBJ 
 * memory management system
 * 
 * (2) An array of itags that define the container - these are 1:1 correspondents with
 * the character string tokens provided by caller
 * 
 * (3) An array of indices into the trigger notifier array - each index points to
 * a notifier whose trigger refers to this container.
 * 
 * (4) An array of pointers to keyval objects that actually hold the data.
 * 
 * At this time, no security is provided on an object-level basis. Thus, all requests for an
 * object are automatically granted. This may be changed at some future time by adding an
 * "authorization" linked list of ID's and their access rights to this structure.
 */
struct orte_gpr_replica_container_t {
    ompi_object_t super;              /**< Make this an object */
    int index;                        /**< Location in the pointer array */
    orte_gpr_replica_itag_t *itags;   /**< Array of itags that define this container */
    int num_itags;                    /**< Number of itags in array */
    orte_gpr_notify_id_t *triggers;   /**< Array of indices into notifier array */
    int num_trigs;                    /**< Number of triggers in array */
    orte_pointer_array_t *itagvals;   /**< Array of itagval pointers */
};
typedef struct orte_gpr_replica_container_t orte_gpr_replica_container_t;

OBJ_CLASS_DECLARATION(orte_gpr_replica_container_t);


/* The itag-value pair for storing data entries in the registry
 */
typedef struct {
    ompi_object_t super;                /* required for this to be an object */
    orte_gpr_replica_itag_t itag;       /* itag for this value's key */
    orte_data_type_t type;              /* the type of value stored */
    orte_gpr_value_union_t value;
} orte_gpr_replica_itagval_t;

OMPI_DECLSPEC OBJ_CLASS_DECLARATION(orte_gpr_replica_itagval_t);


struct orte_gpr_replica_notify_tracker_t {
    ompi_object_t super;                    /**< Make this an object */
    orte_process_name_t *requestor;         /**< Name of requesting process */
    orte_gpr_notify_cb_fn_t callback;       /**< Function to be called for notificaiton */
    void *user_tag;                         /**< User-provided tag for callback function */
    orte_gpr_notify_id_t local_idtag;       /**< Local ID tag of associated subscription */
    orte_gpr_notify_id_t remote_idtag;      /**< Remote ID tag of subscription */
    orte_gpr_replica_segment_t *segptr;     /**< Pointer to segment that subscription was
                                                  placed upon */
    orte_gpr_addr_mode_t addr_mode;         /**< Addressing mode */
    char **tokens;                          /**< Array of tokens defining which containers are affected */
    uint32_t num_tokens;                    /**< Number of tokens in array */
    char *key;                              /**< Key defining which key-value pairs are affected */
    orte_gpr_cmd_flag_t cmd;                    /**< command that generated the notify msg */
    union {
        orte_gpr_notify_action_t trig_action;   /**< If subscription, action that triggered message */
        orte_gpr_synchro_mode_t trig_synchro;   /**< If synchro, action that triggered message */
    } flag;
    uint32_t trigger;                       /**< Number of objects that trigger notification */
    uint32_t count;                         /**< Number of qualifying objects currently in segment */
    int8_t above_below;                     /**< Tracks transitions across level */

};
typedef struct orte_gpr_replica_notify_tracker_t orte_gpr_replica_notify_tracker_t;

OMPI_DECLSPEC OBJ_CLASS_DECLARATION(orte_gpr_replica_notify_tracker_t);

/* define flags for synchro evaluation */
#define ORTE_GPR_REPLICA_TRIGGER_ABOVE_LEVEL   (int8_t) 1
#define ORTE_GPR_REPLICA_TRIGGER_BELOW_LEVEL   (int8_t) -1
#define ORTE_GPR_REPLICA_TRIGGER_AT_LEVEL      (int8_t) 0

/* define a few action flags for trigger evaluation
 */
#define ORTE_GPR_REPLICA_ENTRY_ADDED       (int8_t) 1
#define ORTE_GPR_REPLICA_ENTRY_DELETED     (int8_t) 2
#define ORTE_GPR_REPLICA_ENTRY_UPDATED     (int8_t) 3
#define ORTE_GPR_REPLICA_SUBSCRIBER_ADDED  (int8_t) 4

/*
 * Callback list objects
 */
struct orte_gpr_replica_callbacks_t {
    ompi_list_item_t item;
    orte_gpr_notify_cb_fn_t cb_func;
    void *user_tag;
    orte_gpr_notify_message_t *message;
    orte_process_name_t *requestor;
    orte_gpr_notify_id_t remote_idtag;
};
typedef struct orte_gpr_replica_callbacks_t orte_gpr_replica_callbacks_t;

OBJ_CLASS_DECLARATION(orte_gpr_replica_callbacks_t);

/*
 * List of process names who have notification turned OFF
 */
struct orte_gpr_replica_notify_off_t {
    ompi_list_item_t item;
    orte_gpr_notify_id_t sub_number;
    orte_process_name_t *proc;
};
typedef struct orte_gpr_replica_notify_off_t orte_gpr_replica_notify_off_t;

OBJ_CLASS_DECLARATION(orte_gpr_replica_notify_off_t);

/** List of replicas that hold a stored entry.
 * Each entry can have an arbitrary number of replicas that hold a copy
 * of the entry. The GPR requires that each entry be replicated in at least
 * two locations. This structure is used to create a linked list of
 * replicas for the entry.
 * 
 * THIS IS NOT IMPLEMENTED YET
 */
struct orte_gpr_replica_list_t {
    ompi_list_item_t item;         /**< Allows this item to be placed on a list */
    orte_process_name_t *replica;  /**< Name of the replica */
};
typedef struct orte_gpr_replica_list_t orte_gpr_replica_list_t;

OBJ_CLASS_DECLARATION(orte_gpr_replica_list_t);

/** Write invalidate structure.
 * The structure used to indicate that an entry has been updated somewhere else in the GPR.
 * The structure contains a flag indicating that the locally stored copy of the entry
 * is no longer valid, a time tag indicating the time of the last known modification
 * of the entry within the global registry, and the replica holding the last known
 * up-to-date version of the entry.
 * 
 * THIS IS NOT IMPLEMENTED YET
 */
struct orte_gpr_replica_write_invalidate_t {
    bool invalidate;
    time_t last_mod;
    orte_process_name_t *valid_replica;
};
typedef struct orte_gpr_replica_write_invalidate_t orte_gpr_replica_write_invalidate_t;


/*
 * globals needed within component
 */
extern orte_gpr_replica_t orte_gpr_replica;
extern orte_gpr_replica_globals_t orte_gpr_replica_globals;


/*
 * Module open / close
 */
int orte_gpr_replica_open(void);
int orte_gpr_replica_close(void);


/*
 * Startup / Shutdown
 */
orte_gpr_base_module_t *orte_gpr_replica_init(bool *allow_multi_user_threads, bool *have_hidden_threads, int *priority);
int orte_gpr_replica_finalize(void);

#endif

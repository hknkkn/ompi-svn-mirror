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
#ifndef ORTE_GPR_PROXY_H
#define ORTE_GPR_PROXY_H


#include "orte_config.h"

#include "include/orte_types.h"
#include "class/ompi_list.h"
#include "dps/dps_types.h"

#include "mca/gpr/base/base.h"

/*
 * Module open / close
 */
int orte_gpr_proxy_open(void);
int orte_gpr_proxy_close(void);


/*
 * Startup / Shutdown
 */
orte_gpr_base_module_t*
orte_gpr_proxy_init(bool *allow_multi_user_threads, bool *have_hidden_threads, int *priority);

int orte_gpr_proxy_finalize(void);

/*
 * proxy-local types
 */

struct orte_gpr_proxy_notify_request_tracker_t {
    ompi_list_item_t item;                   /**< Allows this item to be placed on a list */
    orte_gpr_notify_cb_fn_t callback;   /**< Function to be called for notificaiton */
    void *user_tag;                          /**< User-provided tag for callback function */
    orte_gpr_notify_id_t local_idtag;   /**< Local ID tag of associated subscription */
    orte_gpr_notify_id_t remote_idtag;  /**< Remote ID tag of subscription */
    char *segment;                           /**< Pointer to name of segment */
    orte_gpr_notify_action_t action;    /**< Action that triggers notification */
};
typedef struct orte_gpr_proxy_notify_request_tracker_t orte_gpr_proxy_notify_request_tracker_t;

OMPI_DECLSPEC OBJ_CLASS_DECLARATION(orte_gpr_proxy_notify_request_tracker_t);


/*
 * globals used within proxy component
 */

extern orte_process_name_t *orte_gpr_my_replica;
extern ompi_list_t orte_gpr_proxy_notify_request_tracker;
extern orte_gpr_notify_id_t orte_gpr_proxy_last_notify_id_tag;
extern ompi_list_t orte_gpr_proxy_free_notify_id_tags;
extern int orte_gpr_proxy_debug;
extern ompi_mutex_t orte_gpr_proxy_mutex;
extern bool orte_gpr_proxy_compound_cmd_mode;
extern orte_buffer_t *orte_gpr_proxy_compound_cmd;
extern ompi_mutex_t orte_gpr_proxy_wait_for_compound_mutex;
extern ompi_condition_t orte_gpr_proxy_compound_cmd_condition;
extern int orte_gpr_proxy_compound_cmd_waiting;

/*
 * Compound cmd functions
 */
int orte_gpr_proxy_begin_compound_cmd(void);

int orte_gpr_proxy_stop_compound_cmd(void);

int orte_gpr_proxy_exec_compound_cmd(void);
    
/*
 * Mode operations
 */
int orte_gpr_proxy_notify_off(orte_gpr_notify_id_t sub_number);

int orte_gpr_proxy_notify_on(orte_gpr_notify_id_t sub_number);

int orte_gpr_proxy_triggers_active(orte_jobid_t jobid);

int orte_gpr_proxy_triggers_inactive(orte_jobid_t jobid);

/*
 * Delete-index functions
 */
int orte_gpr_proxy_delete_segment(char *segment);

int orte_gpr_proxy_delete_segment_nb(char *segment,
                                orte_gpr_notify_cb_fn_t cbfunc, void *user_tag);

int orte_gpr_proxy_delete_entries(orte_gpr_addr_mode_t mode,
			    char *segment, char **tokens, char **keys);

int orte_gpr_proxy_delete_entries_nb(
                            orte_gpr_addr_mode_t addr_mode,
                            char *segment, char **tokens, char **keys,
                            orte_gpr_notify_cb_fn_t cbfunc, void *user_tag);
                            
int orte_gpr_proxy_index(char *segment, size_t *cnt, char **index);

int orte_gpr_proxy_index_nb(char *segment,
                        orte_gpr_notify_cb_fn_t cbfunc, void *user_tag);


/*
 * Cleanup functions
 */
int orte_gpr_proxy_cleanup_job(orte_jobid_t jobid);

int orte_gpr_proxy_cleanup_proc(bool purge, orte_process_name_t *proc);


/*
 * Put-get functions
 */
int orte_gpr_proxy_put(orte_gpr_addr_mode_t mode, char *segment,
		  char **tokens, size_t cnt, orte_gpr_keyval_t **keyvals);

int orte_gpr_proxy_put_nb(orte_gpr_addr_mode_t addr_mode, char *segment,
                      char **tokens, size_t cnt, orte_gpr_keyval_t **keyvals,
                      orte_gpr_notify_cb_fn_t cbfunc, void *user_tag);
                      
int orte_gpr_proxy_get(orte_gpr_addr_mode_t addr_mode,
                                char *segment, char **tokens, char **keys,
                                size_t *cnt, orte_gpr_keyval_t **keyvals);

int orte_gpr_proxy_get_nb(orte_gpr_addr_mode_t addr_mode,
                                char *segment, char **tokens, char **keys,
                                orte_gpr_notify_cb_fn_t cbfunc, void *user_tag);


/*
 * Subscribe functions
 */
int orte_gpr_proxy_subscribe(orte_gpr_addr_mode_t addr_mode,
                            orte_gpr_notify_action_t action,
                            char *segment, char **tokens, char **keys,
                            orte_gpr_notify_id_t *sub_number,
                            orte_gpr_notify_cb_fn_t cb_func, void *user_tag);

int orte_gpr_proxy_unsubscribe(orte_gpr_notify_id_t sub_number);


/*
 * Synchro functions
 */
int orte_gpr_proxy_synchro(orte_gpr_addr_mode_t addr_mode,
                            orte_gpr_synchro_mode_t synchro_mode,
                            char *segment, char **tokens, char **keys, int trigger,
                            orte_gpr_notify_id_t *synch_number,
                            orte_gpr_notify_cb_fn_t cb_func, void *user_tag);

int orte_gpr_proxy_cancel_synchro(orte_gpr_notify_id_t synch_number);


/*
 * Dump function
 */
int orte_gpr_proxy_dump(int output_id);


/*
 * Messaging functions
 */
int orte_gpr_proxy_deliver_notify_msg(orte_gpr_notify_action_t state,
				      orte_gpr_notify_message_t *message);


/*
 * Test internals
 */
int orte_gpr_proxy_test_internals(int level, ompi_list_t *results);


/*
 * Startup functions
 */
int orte_gpr_proxy_get_startup_msg(orte_jobid_t jobid,
                                    orte_buffer_t **msg,
                                    size_t *cnt,
                                    orte_process_name_t **procs);


/*
 * Functions that interface to the replica
 */
void orte_gpr_proxy_notify_recv(int status, orte_process_name_t* sender,
			       orte_buffer_t *buffer, orte_rml_tag_t tag,
			       void* cbdata);


/*
 * Internal functions
 */

int
orte_gpr_proxy_enter_notify_request(orte_gpr_notify_id_t *idtag, char *segment,
                    orte_gpr_notify_action_t action,
				   orte_gpr_notify_cb_fn_t cb_func,
				   void *user_tag);

int
orte_gpr_proxy_remove_notify_request(orte_gpr_notify_id_t local_idtag);

int orte_gpr_proxy_set_remote_idtag(orte_gpr_notify_id_t local_idtag,
				   orte_gpr_notify_id_t remote_idtag);

#endif

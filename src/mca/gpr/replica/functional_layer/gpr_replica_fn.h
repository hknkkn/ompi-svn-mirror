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
/** @file:
 *
 * The Open MPI general purpose registry - support functions.
 *
 */

#ifndef MCA_GPR_REPLICA_FN_H_
#define MCA_GPR_REPLICA_FN_H_

#include "orte_config.h"

#include "mca/ns/ns_types.h"
#include "mca/gpr/gpr_types.h"
#include "mca/gpr/replica/gpr_replica.h"

/*
 * The "fn" layer of the registry API functions - not accessible from outside
 * the replica
 */
int orte_gpr_replica_notify_off_fn(orte_gpr_notify_id_t sub_number);

int orte_gpr_replica_notify_on_fn(orte_gpr_notify_id_t sub_number);

int orte_gpr_replica_triggers_active_fn(orte_jobid_t jobid);

int orte_gpr_replica_triggers_inactive_fn(orte_jobid_t jobid);

/*
 * Delete-index functions
 */
int orte_gpr_replica_delete_entries_fn(orte_gpr_addr_mode_t mode,
                char *segment, char **tokens, char **keys);

int orte_gpr_replica_delete_entries_nb_fn(
                            orte_gpr_addr_mode_t addr_mode,
                            char *segment, char **tokens, char **keys,
                            orte_gpr_notify_cb_fn_t cbfunc, void *user_tag);
                            
int orte_gpr_replica_index_fn(char *segment, size_t *cnt, char **index);

int orte_gpr_replica_index_nb_fn(char *segment,
                        orte_gpr_notify_cb_fn_t cbfunc, void *user_tag);


/*
 * Cleanup functions
 */
int orte_gpr_replica_cleanup_job_fn(orte_jobid_t jobid);

int orte_gpr_replica_cleanup_proc_fn(bool purge, orte_process_name_t *proc);


/*
 * Put-get functions
 */
int orte_gpr_replica_put_fn(orte_gpr_addr_mode_t mode, char *segment,
       char **tokens, size_t cnt, orte_gpr_keyval_t **keyvals);

int orte_gpr_replica_put_nb_fn(orte_gpr_addr_mode_t addr_mode, char *segment,
                      char **tokens, size_t cnt, orte_gpr_keyval_t **keyvals,
                      orte_gpr_notify_cb_fn_t cbfunc, void *user_tag);
                      
int orte_gpr_replica_get_fn(orte_gpr_addr_mode_t addr_mode,
                                char *segment, char **tokens, char **keys,
                                size_t *cnt, orte_gpr_keyval_t **keyvals);

int orte_gpr_replica_get_nb_fn(orte_gpr_addr_mode_t addr_mode,
                                char *segment, char **tokens, char **keys,
                                orte_gpr_notify_cb_fn_t cbfunc, void *user_tag);


/*
 * Subscribe functions
 */
int orte_gpr_replica_subscribe_fn(orte_gpr_addr_mode_t addr_mode,
                            orte_gpr_notify_action_t action,
                            char *segment, char **tokens, char **keys,
                            orte_gpr_notify_id_t *sub_number,
                            orte_gpr_notify_cb_fn_t cb_func, void *user_tag);

int orte_gpr_replica_unsubscribe_fn(orte_gpr_notify_id_t sub_number);


/*
 * Synchro functions
 */
int orte_gpr_replica_synchro_fn(orte_gpr_addr_mode_t addr_mode,
                            orte_gpr_synchro_mode_t synchro_mode,
                            char *segment, char **tokens, char **keys, int trigger,
                            orte_gpr_notify_id_t *synch_number,
                            orte_gpr_notify_cb_fn_t cb_func, void *user_tag);

int orte_gpr_replica_cancel_synchro_fn(orte_gpr_notify_id_t synch_number);


/*
 * Dump function
 */
int orte_gpr_replica_dump_fn(int output_id);


/*
 * Messaging functions
 */
int orte_gpr_replica_deliver_notify_msg_fn(orte_gpr_notify_action_t state,
                      orte_gpr_notify_message_t *message);


/*
 * Test internals
 */
int orte_gpr_replica_test_internals_fn(int level, ompi_list_t *results);


/*
 * Startup functions
 */
int orte_gpr_replica_get_startup_msg_fn(orte_jobid_t jobid,
                                    orte_buffer_t **msg,
                                    size_t *cnt,
                                    orte_process_name_t **procs);


/** Empty the specified segment and remove it from the registry
 */
int orte_gpr_replica_release_segment(orte_gpr_replica_segment_t *seg);


/*
 * DICTIONARY OPERATIONS
 */
 
bool orte_gpr_replica_check_key_list(orte_gpr_addr_mode_t mode,
				    orte_gpr_replica_key_t num_keys_search,
				    orte_gpr_replica_key_t *keys,
				    orte_gpr_replica_key_t num_keys_entry,
				    orte_gpr_replica_key_t *entry_keys);

orte_gpr_replica_trigger_list_t
*orte_gpr_replica_construct_trigger(orte_gpr_synchro_mode_t synchro_mode,
				   orte_gpr_notify_action_t action,
				   orte_gpr_addr_mode_t addr_mode,
				   orte_gpr_replica_segment_t *seg,
				   orte_gpr_replica_key_t *keys,
				   int num_keys,
				   int trigger,
				   orte_gpr_notify_id_t id_tag,
                    orte_jobid_t owning_jobid);

orte_gpr_notify_message_t
*orte_gpr_replica_construct_notify_message(orte_gpr_replica_segment_t *seg,
					  orte_gpr_replica_trigger_list_t *trig);

bool orte_gpr_replica_process_triggers(orte_gpr_replica_segment_t *seg,
				      orte_gpr_replica_trigger_list_t *trig,
				      orte_gpr_notify_message_t *message);

orte_gpr_notify_id_t
orte_gpr_replica_remove_trigger(orte_gpr_notify_id_t idtag);

orte_gpr_notify_id_t
orte_gpr_replica_enter_notify_request(orte_gpr_replica_segment_t *seg,
				     orte_gpr_notify_action_t action,
				     orte_process_name_t *requestor,
				     orte_gpr_notify_id_t idtag,
				     orte_gpr_notify_cb_fn_t cb_func,
				     void *user_tag);

orte_gpr_notify_id_t
orte_gpr_replica_remove_notify_request(orte_gpr_notify_id_t idtag);

void orte_gpr_replica_process_callbacks(void);

int orte_gpr_replica_check_synchros(orte_gpr_replica_segment_t *seg);

void orte_gpr_replica_check_subscriptions(orte_gpr_replica_segment_t *seg, int8_t action_taken);

int orte_gpr_replica_purge_subscriptions(orte_process_name_t *proc);

orte_buffer_t
orte_gpr_replica_process_command_buffer(orte_buffer_t buffer,
				       orte_process_name_t *sender,
				       bool *return_requested,
				       bool *compound_cmd_detected);


#endif

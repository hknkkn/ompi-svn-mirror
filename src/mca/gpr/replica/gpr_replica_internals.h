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

#ifndef MCA_GPR_REPLICA_INTERNALS_H_
#define MCA_GPR_REPLICA_INTERNALS_H_

#include "orte_config.h"

#include "mca/ns/ns_types.h"

/*
 * The "locked" layer of the registry API functions - not accessible from outside
 * the replica
 */
int orte_gpr_replica_notify_off_locked(orte_gpr_notify_id_t sub_number);

int orte_gpr_replica_notify_on_locked(orte_gpr_notify_id_t sub_number);

int orte_gpr_replica_triggers_active_locked(orte_jobid_t jobid);

int orte_gpr_replica_triggers_inactive_locked(orte_jobid_t jobid);

/*
 * Delete-index functions
 */
int orte_gpr_replica_delete_segment_nb_locked(char *segment,
                                orte_gpr_notify_cb_fn_t cbfunc, void *user_tag);

int orte_gpr_replica_delete_entries_locked(orte_gpr_addr_mode_t mode,
                char *segment, char **tokens, char **keys);

int orte_gpr_replica_delete_entries_nb_locked(
                            orte_gpr_addr_mode_t addr_mode,
                            char *segment, char **tokens, char **keys,
                            orte_gpr_notify_cb_fn_t cbfunc, void *user_tag);
                            
int orte_gpr_replica_index_locked(char *segment, size_t *cnt, char **index);

int orte_gpr_replica_index_nb_locked(char *segment,
                        orte_gpr_notify_cb_fn_t cbfunc, void *user_tag);


/*
 * Cleanup functions
 */
int orte_gpr_replica_cleanup_job_locked(orte_jobid_t jobid);

int orte_gpr_replica_cleanup_proc_locked(bool purge, orte_process_name_t *proc);


/*
 * Put-get functions
 */
int orte_gpr_replica_put_locked(orte_gpr_addr_mode_t mode, char *segment,
       char **tokens, size_t cnt, orte_gpr_keyval_t **keyvals);

int orte_gpr_replica_put_nb_locked(orte_gpr_addr_mode_t addr_mode, char *segment,
                      char **tokens, size_t cnt, orte_gpr_keyval_t **keyvals,
                      orte_gpr_notify_cb_fn_t cbfunc, void *user_tag);
                      
int orte_gpr_replica_get_locked(orte_gpr_addr_mode_t addr_mode,
                                char *segment, char **tokens, char **keys,
                                size_t *cnt, orte_gpr_keyval_t **keyvals);

int orte_gpr_replica_get_nb_locked(orte_gpr_addr_mode_t addr_mode,
                                char *segment, char **tokens, char **keys,
                                orte_gpr_notify_cb_fn_t cbfunc, void *user_tag);


/*
 * Subscribe functions
 */
int orte_gpr_replica_subscribe_locked(orte_gpr_addr_mode_t addr_mode,
                            orte_gpr_notify_action_t action,
                            char *segment, char **tokens, char **keys,
                            orte_gpr_notify_id_t *sub_number,
                            orte_gpr_notify_cb_fn_t cb_func, void *user_tag);

int orte_gpr_replica_unsubscribe_locked(orte_gpr_notify_id_t sub_number);


/*
 * Synchro functions
 */
int orte_gpr_replica_synchro_locked(orte_gpr_addr_mode_t addr_mode,
                            orte_gpr_synchro_mode_t synchro_mode,
                            char *segment, char **tokens, char **keys, int trigger,
                            orte_gpr_notify_id_t *synch_number,
                            orte_gpr_notify_cb_fn_t cb_func, void *user_tag);

int orte_gpr_replica_cancel_synchro_locked(orte_gpr_notify_id_t synch_number);


/*
 * Dump function
 */
int orte_gpr_replica_dump_locked(int output_id);


/*
 * Messaging functions
 */
int orte_gpr_replica_deliver_notify_msg_locked(orte_gpr_notify_action_t state,
                      orte_gpr_notify_message_t *message);


/*
 * Test internals
 */
int orte_gpr_replica_test_internals_locked(int level, ompi_list_t *results);


/*
 * Startup functions
 */
int orte_gpr_replica_get_startup_msg_locked(orte_jobid_t jobid,
                                    orte_buffer_t **msg,
                                    size_t *cnt,
                                    orte_process_name_t **procs);



/** Retrieve a registry itag value for a given token string.
 * The orte_gpr_replica_get_itag() function is used to translate a token string for a particular
 * segment of the registry into its associated (integer) key value.
 *
 * @param seg (IN) Index to the segment of the registry being queried.
 * @param token (IN) Pointer to a character string containing the token to be translated. If token=NULL,
 * the function returns the itag value corresponding to the specified segment itself.
 *
 * @param *itag (OUT) orte_gpr_replica_itag_t value corresponding to the specified token
 * within the specified segment.
 * 
 * @retval ORTE_SUCCESS Operation successful.
 * @retval ORTE_ERR_BAD_PARAM Indicates that the segment and/or token could not be found.
 */
int orte_gpr_replica_get_key(orte_gpr_replica_itag_t *itag, int seg, char *token);

/** Add a token to a segment's dictionary.
 * The gpr_replica_define_key() function allows the addition of a new definition to
 * the registry's token-key dictionaries. The specified token is assigned an integer
 * value within the specified segment, and the entry is added to the segment's token-key
 * dictionary.
 *
 * @param segment Pointer to a character string defining the segment of the registry being queried.
 * @param token Pointer to a character string containing the token to be defined. If segment=NULL,
 * the function adds the token to the segment dictionary, thus defining a new segment name.
 *
 * @param *itag (OUT) orte_gpr_replica_itag_t value of corresponding token.
 * 
 * @retval ORTE_SUCCESS Operation successful
 * @retval ORTE_ERROR(s) Indicates that the dictionary is full or some other error.
 */
int orte_gpr_replica_define_key(orte_gpr_replica_itag_t *itag,
                                orte_gpr_replica_segment_t *seg, char *token);

/** Delete a token from a segment's dictionary.
 * The gpr_replica_deletekey() function allows the removal of a definition from the
 * registry's token-key dictionaries. This should be used with caution! Deletion of
 * a token-key pair causes the registry to search through all entries within that segment
 * for objects that include the specified token-key pair in their description. The reference
 * is subsequently removed, and any object for which this was the SOLE key will also
 * be removed from the registry!
 *
 * @param segment Pointer to a character string defining the segment of the registry.
 * @param token Pointer to a character string containing the token to be deleted. If token=NULL,
 * the function deletes the specified segment name from the segment dictionary.
 *
 * @retval ORTE_SUCCESS Indicating that the operation was successful.
 * @retval ORTE_ERROR Indicates that the operation failed - most likely caused by specifying
 * a token that did not exist within the specified segment, or a non-existent segment.
 */
int orte_gpr_replica_delete_key(orte_gpr_replica_segment_t *seg, char *token);


/*
 * SEGMENT OPERATIONS
 */
 
/** Find a requested registry segment.
 * The gpr_replica_findseq() function finds the registry segment corresponding to
 * the specified name.
 *
 * @param create A boolean that indicates whether or not to create the segment if it
 * doesn't already exist. TRUE => create it, FALSE => don't create it.
 * @param segment Pointer to a string containing the name of the segment to be found.
 *
 * @retval *seg Pointer to the segment
 * @retval NULL Indicates that the specified segment could not be found
 */
orte_gpr_replica_segment_t *orte_gpr_replica_find_seg(bool create, char *segment,
						    orte_jobid_t jobid);

/** Empty the specified segment and remove it from the registry
 */
int orte_gpr_replica_release_segment(orte_gpr_replica_segment_t *seg);


/*
 * DICTIONARY OPERATIONS
 */
 
orte_gpr_replica_keytable_t
*orte_gpr_replica_find_dict_entry(orte_gpr_replica_segment_t *seg, char *token);


orte_gpr_replica_key_t
*orte_gpr_replica_get_key_list(orte_gpr_replica_segment_t *seg, char **tokens,
			      int *num_tokens);

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

char *orte_gpr_replica_get_token(orte_gpr_replica_segment_t *seg, orte_gpr_replica_key_t key);

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

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
/** @file:
 *
 * The Open MPI General Purpose Registry - Proxy component
 *
 */

/*
 * includes
 */
#include "orte_config.h"

#include "include/orte_constants.h"
#include "include/orte_types.h"
#include "dps/dps.h"

#include "mca/ns/ns.h"

#include "gpr_proxy.h"


/*
 * Struct of function pointers that need to be initialized
 */
OMPI_COMP_EXPORT mca_gpr_base_component_t orte_gpr_proxy_component = {
    {
	MCA_GPR_BASE_VERSION_1_0_0,

	"orte_gpr_proxy", /* MCA module name */
	1,  /* MCA module major version */
	0,  /* MCA module minor version */
	0,  /* MCA module release version */
	orte_gpr_proxy_open,  /* module open */
	orte_gpr_proxy_close /* module close */
    },
    {
	false /* checkpoint / restart */
    },
    orte_gpr_proxy_init,    /* module init */
    orte_gpr_proxy_finalize /* module shutdown */
};

/*
 * setup the function pointers for the module
 */
static orte_gpr_base_module_t orte_gpr_proxy = {
   /* BLOCKING OPERATIONS */
    orte_gpr_proxy_module_get_fn_t get,
    orte_gpr_proxy_module_put_fn_t put,
    orte_gpr_proxy_module_delete_entries_fn_t delete_entries,
    orte_gpr_proxy_module_delete_segment_fn_t delete_segment,
    /* NON-BLOCKING OPERATIONS */
    orte_gpr_proxy_module_get_nb_fn_t get_nb,
    orte_gpr_proxy_module_put_nb_fn_t put_nb,
    orte_gpr_proxy_module_delete_entries_nb_fn_t delete_entries_nb,
    orte_gpr_proxy_module_delete_segment_nb_fn_t delete_segment_nb,
    /* SUBSCRIBE OPERATIONS */
    orte_gpr_proxy_module_subscribe_fn_t subscribe,
    orte_gpr_proxy_module_unsubscribe_fn_t unsubscribe,
    /* SYNCHRO OPERATIONS */
    orte_gpr_proxy_module_synchro_fn_t synchro,
    orte_gpr_proxy_module_cancel_synchro_fn_t cancel_synchro,
    /* COMPOUND COMMANDS */
    orte_gpr_proxy_module_begin_compound_cmd_fn_t begin_compound_cmd,
    orte_gpr_proxy_module_stop_compound_cmd_fn_t stop_compound_cmd,
    orte_gpr_proxy_module_exec_compound_cmd_fn_t exec_compound_cmd,
    /* DUMP/INDEX */
    orte_gpr_proxy_module_dump_fn_t dump,
    orte_gpr_proxy_module_index_fn_t index,
    /* MODE OPERATIONS */
    orte_gpr_proxy_module_notify_on_fn_t notify_on,
    orte_gpr_proxy_module_notify_off_fn_t notify_off,
    orte_gpr_proxy_module_triggers_active_fn_t triggers_active,
    orte_gpr_proxy_module_triggers_inactive_fn_t triggers_inactive,
    /* MESSAGING OPERATIONS */
    orte_gpr_proxy_module_get_startup_msg_fn_t get_startup_msg,
    orte_gpr_proxy_module_deliver_notify_msg_fn_t deliver_notify_msg,
    /* CLEANUP OPERATIONS */
    orte_gpr_proxy_module_cleanup_job_fn_t cleanup_job,
    orte_gpr_proxy_module_cleanup_proc_fn_t cleanup_process,
    /* TEST INTERFACE */
    orte_gpr_proxy_module_test_internals_fn_t test_internals
};


/*
 * Whether or not we allowed this component to be selected
 */
static bool initialized = false;

/*
 * globals needed within proxy component
 */
orte_process_name_t *orte_gpr_my_replica;
ompi_list_t orte_gpr_proxy_notify_request_tracker;
orte_gpr_notify_id_t orte_gpr_proxy_last_notify_id_tag;
ompi_list_t orte_gpr_proxy_free_notify_id_tags;
int orte_gpr_proxy_debug;
ompi_mutex_t orte_gpr_proxy_mutex;
bool orte_gpr_proxy_compound_cmd_mode;
orte_buffer_t orte_gpr_proxy_compound_cmd;
ompi_mutex_t orte_gpr_proxy_wait_for_compound_mutex;
ompi_condition_t orte_gpr_proxy_compound_cmd_condition;
int orte_gpr_proxy_compound_cmd_waiting;
bool orte_gpr_proxy_silent_mode;


/* constructor - used to initialize notify message instance */
static void orte_gpr_proxy_notify_request_tracker_construct(orte_gpr_proxy_notify_request_tracker_t* req)
{
    req->callback = NULL;
    req->user_tag = NULL;
    req->local_idtag = ORTE_REGISTRY_NOTIFY_ID_MAX;
    req->remote_idtag = ORTE_REGISTRY_NOTIFY_ID_MAX;
    req->segment = NULL;
    req->action = ORTE_REGISTRY_NOTIFY_NONE;
}

/* destructor - used to free any resources held by instance */
static void orte_gpr_proxy_notify_request_tracker_destructor(orte_gpr_proxy_notify_request_tracker_t* req)
{
    if (NULL != req->segment) {
	free(req->segment);
    }
}

/* define instance of ompi_class_t */
OBJ_CLASS_INSTANCE(
		   orte_gpr_proxy_notify_request_tracker_t,            /* type name */
		   ompi_list_item_t,                          /* parent "class" name */
		   orte_gpr_proxy_notify_request_tracker_construct,    /* constructor */
		   orte_gpr_proxy_notify_request_tracker_destructor);  /* destructor */


/*
 * Open the component
 */
int orte_gpr_proxy_open(void)
{
    int id;

    id = mca_base_param_register_int("gpr", "proxy", "debug", NULL, 0);
    mca_base_param_lookup_int(id, &orte_gpr_proxy_debug);

    return ORTE_SUCCESS;
}

/*
 * Close the component
 */
int orte_gpr_proxy_close(void)
{
    return ORTE_SUCCESS;
}

orte_gpr_base_module_t* orte_gpr_proxy_init(bool *allow_multi_user_threads, bool *have_hidden_threads, int *priority)
{
    int rc;

    if (orte_gpr_proxy_debug) {
	ompi_output(0, "gpr_proxy_init called");
    }

    /* If we are NOT to host a replica, then we want to be selected, so do all
       the setup and return the module */
    if (NULL != ompi_process_info.gpr_replica) {

	if (orte_gpr_proxy_debug) {
	    ompi_output(0, "gpr_proxy_init: proxy selected");
	}

	/* Return a module (choose an arbitrary, positive priority --
	   it's only relevant compared to other ns components).  If
	   we're not the seed, then we don't want to be selected, so
	   return NULL. */

	*priority = 10;

	/* We allow multi user threads but don't have any hidden threads */

	*allow_multi_user_threads = true;
	*have_hidden_threads = false;

	/* setup thread locks and condition variable */
	OBJ_CONSTRUCT(&orte_gpr_proxy_mutex, ompi_mutex_t);
	OBJ_CONSTRUCT(&orte_gpr_proxy_wait_for_compound_mutex, ompi_mutex_t);
	OBJ_CONSTRUCT(&orte_gpr_proxy_compound_cmd_condition, ompi_condition_t);

	/* initialize the registry compound mode */
	orte_gpr_proxy_compound_cmd_mode = false;
	orte_gpr_proxy_compound_cmd_waiting = 0;
	orte_gpr_proxy_compound_cmd = NULL;

	/* define the replica for us to use - get it from process_info */
    if (ORTE_SUCCESS != orte_name_services.copy_process_name(orte_gpr_my_replica, ompi_process_info.gpr_replica)) {
        return NULL;
    }
    
	if (NULL == orte_gpr_my_replica) { /* can't function */
	    return NULL;
	}

	/* initialize the notify request tracker */
	OBJ_CONSTRUCT(&orte_gpr_proxy_notify_request_tracker, ompi_list_t);
	orte_gpr_proxy_last_notify_id_tag = 0;
	OBJ_CONSTRUCT(&orte_gpr_proxy_free_notify_id_tags, ompi_list_t);

	/* initialize any local variables */
	orte_gpr_proxy_silent_mode = false;

	/* issue the non-blocking receive */
	rc = mca_oob_recv_packed_nb(MCA_OOB_NAME_ANY, MCA_OOB_TAG_GPR_NOTIFY, 0, orte_gpr_proxy_notify_recv, NULL);
	if(rc != ORTE_SUCCESS && rc != ORTE_ERR_NOT_IMPLEMENTED) {
	    return NULL;
	}

	/* Return the module */

	initialized = true;
	return &orte_gpr_proxy;
    } else {
	return NULL;
    }
}

/*
 * finalize routine
 */
int orte_gpr_proxy_finalize(void)
{

    if (orte_gpr_proxy_debug) {
	ompi_output(0, "finalizing gpr proxy");
    }

    if (initialized) {
	initialized = false;
    }

    /* All done */
    mca_oob_recv_cancel(MCA_OOB_NAME_ANY, MCA_OOB_TAG_GPR_NOTIFY);
    return ORTE_SUCCESS;
}

/* 
 * handle notify messages from replicas
 */

void orte_gpr_proxy_notify_recv(int status, orte_process_name_t* sender,
			       orte_buffer_t buffer, int tag,
			       void* cbdata)
{
    char **tokptr;
    orte_gpr_cmd_flag_t command;
    uint32_t num_items;
    uint32_t i;
    orte_gpr_notify_id_t id_tag;
    orte_gpr_value_t *regval;
    orte_gpr_notify_message_t *message;
    bool found;
    orte_gpr_proxy_notify_request_tracker_t *trackptr;

    if (orte_gpr_proxy_debug) {
	ompi_output(0, "[%d,%d,%d] gpr proxy: received trigger message",
				ORTE_NAME_ARGS(*ompi_rte_get_self()));
    }

    message = OBJ_NEW(orte_gpr_notify_message_t);

    if ((ORTE_SUCCESS != (rc = orte_dps.unpack(buffer, &command, 1, MCA_GPR_OOB_PACK_CMD)) ||
	(MCA_GPR_NOTIFY_CMD != command)) {
	goto RETURN_ERROR;
    }

    if (0 > (rc = orte_dps.unpack_string(buffer, &message->segment)) {
	goto RETURN_ERROR;
    }

    if (ORTE_SUCCESS != (rc = orte_dps.unpack(buffer, &i, 1, ORTE_INT32)) {
	goto RETURN_ERROR;
    }
    message->owning_job = (orte_jobid_t)i;

    if (ORTE_SUCCESS != (rc = orte_dps.unpack(buffer, &i, 1, ORTE_INT32)) {
	goto RETURN_ERROR;
    }
	id_tag = (orte_gpr_notify_id_t)i;
	
    if (orte_gpr_proxy_debug) {
	ompi_output(0, "[%d,%d,%d] trigger from segment %s id %d",
				ORTE_NAME_ARGS(*ompi_rte_get_self()), message->segment, (int)id_tag);
    }

    if (ORTE_SUCCESS != (rc = orte_dps.unpack(buffer, &message->trig_action, 1, MCA_GPR_OOB_PACK_ACTION)) {
	goto RETURN_ERROR;
    }

    if (ORTE_SUCCESS != (rc = orte_dps.unpack(buffer, &message->trig_synchro, 1, MCA_GPR_OOB_PACK_SYNCHRO_MODE)) {
	goto RETURN_ERROR;
    }

    if (ORTE_SUCCESS != (rc = orte_dps.unpack(buffer, &num_items, 1, ORTE_INT32)) {
	goto RETURN_ERROR;
    }

    for (i=0; i < num_items; i++) {
	regval = OBJ_NEW(orte_gpr_value_t);
	if (ORTE_SUCCESS != (rc = orte_dps.unpack(buffer, &regval->object_size, 1, MCA_GPR_OOB_PACK_OBJECT_SIZE)) {
	    OBJ_RELEASE(regval);
	    goto RETURN_ERROR;
	}
	if((regval->object = malloc(regval->object_size)) == NULL) {
	    OBJ_RELEASE(regval);
	    goto RETURN_ERROR;
	}
	if (ORTE_SUCCESS != (rc = orte_dps.unpack(buffer, regval->object, regval->object_size, ORTE_BYTE)) {
	    OBJ_RELEASE(regval);
	    goto RETURN_ERROR;
	}
	ompi_list_append(&message->data, &regval->item);
    }

    if (ORTE_SUCCESS != (rc = orte_dps.unpack(buffer, &message->num_tokens, 1, ORTE_INT32)) {
	goto RETURN_ERROR;
    }

    if(message->num_tokens > 0) {
        message->tokens = (char**)malloc(message->num_tokens*sizeof(char*));
        for (i=0, tokptr=message->tokens; i < message->num_tokens; i++, tokptr++) {
	    if ((rc = orte_dps.unpack_string(buffer, tokptr) < 0) {
		goto RETURN_ERROR;
	    }
        }
    } else {
        message->tokens = NULL;
    }

    OMPI_THREAD_LOCK(&orte_gpr_proxy_mutex);

    /* find the request corresponding to this notify */
    found = false;
    for (trackptr = (orte_gpr_proxy_notify_request_tracker_t*)ompi_list_get_first(&orte_gpr_proxy_notify_request_tracker);
         trackptr != (orte_gpr_proxy_notify_request_tracker_t*)ompi_list_get_end(&orte_gpr_proxy_notify_request_tracker);
         trackptr = (orte_gpr_proxy_notify_request_tracker_t*)ompi_list_get_next(trackptr)) {
     	if (orte_gpr_proxy_debug) {
     		ompi_output(0, "\tchecking idtag %d for segment %s\n", trackptr->local_idtag, trackptr->segment);
     	}
		if (trackptr->local_idtag == id_tag) {
            found = true;
            break;
        }
    }

    OMPI_THREAD_UNLOCK(&orte_gpr_proxy_mutex);

    if (!found) {  /* didn't find request */
        	ompi_output(0, "[%d,%d,%d] Proxy notification error - received request not found",
                        ORTE_NAME_ARGS(*ompi_rte_get_self()));
        	return;
    }

    /* process request */
    trackptr->callback(message, trackptr->user_tag);

    /* dismantle message and free memory */

 RETURN_ERROR:
    OBJ_RELEASE(message);

    /* reissue non-blocking receive */
    mca_oob_recv_packed_nb(MCA_OOB_NAME_ANY, MCA_OOB_TAG_GPR_NOTIFY, 0, orte_gpr_proxy_notify_recv, NULL);

}


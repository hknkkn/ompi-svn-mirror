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
 * The Open MPI General Purpose Registry - Replica component
 *
 */

/*
 * includes
 */
#include "orte_config.h"

#include "util/output.h"
#include "util/proc_info.h"

#include "mca/rml/rml.h"

#include "gpr_replica.h"
#include "mca/gpr/replica/api_layer/gpr_replica_api.h"
#include "mca/gpr/replica/communications/gpr_replica_comm.h"


/*
 * Struct of function pointers that need to be initialized
 */
OMPI_COMP_EXPORT mca_gpr_base_component_t mca_gpr_replica_component = {
    {
	MCA_GPR_BASE_VERSION_1_0_0,

	"replica", /* MCA module name */
	1,  /* MCA module major version */
	0,  /* MCA module minor version */
	0,  /* MCA module release version */
	orte_gpr_replica_open,  /* module open */
	orte_gpr_replica_close /* module close */
    },
    {
	false /* checkpoint / restart */
    },
    orte_gpr_replica_init,    /* module init */
    orte_gpr_replica_finalize /* module shutdown */
};

/*
 * setup the function pointers for the module
 */
static orte_gpr_base_module_t orte_gpr_replica_module = {
    orte_gpr_replica_get,
    orte_gpr_replica_put,
    orte_gpr_replica_delete_entries,
    orte_gpr_replica_delete_segment,
    orte_gpr_replica_index,
    orte_gpr_replica_get_nb,
    orte_gpr_replica_put_nb,
    orte_gpr_replica_delete_entries_nb,
    orte_gpr_replica_delete_segment_nb,
    orte_gpr_replica_index_nb,
    orte_gpr_replica_preallocate_segment,
    orte_gpr_replica_get_startup_msg,
    orte_gpr_replica_decode_startup_msg,
    orte_gpr_replica_subscribe,
    orte_gpr_replica_unsubscribe,
    orte_gpr_replica_synchro,
    orte_gpr_replica_cancel_synchro,
    orte_gpr_replica_begin_compound_cmd,
    orte_gpr_replica_stop_compound_cmd,
    orte_gpr_replica_exec_compound_cmd,
    orte_gpr_replica_dump,
    orte_gpr_replica_notify_on,
    orte_gpr_replica_notify_off,
    orte_gpr_replica_triggers_active,
    orte_gpr_replica_triggers_inactive,
    orte_gpr_replica_cleanup_job,
    orte_gpr_replica_cleanup_proc,
    orte_gpr_replica_test_internals
};

/*
 * Whether or not we allowed this component to be selected
 */
static bool initialized = false;


/*
 * globals needed within replica component
 */
orte_gpr_replica_t orte_gpr_replica;

orte_gpr_replica_globals_t orte_gpr_replica_globals;

/*
 * CONSTRUCTORS AND DESTRUCTORS
 */

/*  SEGMENT */
/* constructor - used to initialize state of segment instance */
static void orte_gpr_replica_segment_construct(orte_gpr_replica_segment_t* seg)
{
    seg->name = NULL;
    seg->itag = ORTE_GPR_REPLICA_ITAG_MAX;
    
    orte_pointer_array_init(&(seg->dict), orte_gpr_replica_globals.block_size,
                            orte_gpr_replica_globals.max_size,
                            orte_gpr_replica_globals.block_size);

    orte_pointer_array_init(&(seg->containers), orte_gpr_replica_globals.block_size,
                            orte_gpr_replica_globals.max_size,
                            orte_gpr_replica_globals.block_size);

    seg->triggers = NULL;
    seg->num_trigs = 0;
    seg->triggers_active = false;
}

/* destructor - used to free any resources held by instance */
static void orte_gpr_replica_segment_destructor(orte_gpr_replica_segment_t* seg)
{
    if (NULL != seg->name) {
        free(seg->name);
    }

    if (NULL != seg->dict) {
       OBJ_RELEASE(seg->dict);
    }
    
    if (NULL != seg->containers) {
        OBJ_RELEASE(seg->containers);
    }
    
    if (NULL != seg->triggers) {
        free(seg->triggers);
    }

}

/* define instance of orte_gpr_replica_segment_t */
OBJ_CLASS_INSTANCE(
          orte_gpr_replica_segment_t,  /* type name */
          ompi_object_t, /* parent "class" name */
          orte_gpr_replica_segment_construct, /* constructor */
          orte_gpr_replica_segment_destructor); /* destructor */


/* CONTAINER */
/* constructor - used to initialize state of registry container instance */
static void orte_gpr_replica_container_construct(orte_gpr_replica_container_t* reg)
{
    orte_pointer_array_init(&(reg->itagvals), orte_gpr_replica_globals.block_size,
                            orte_gpr_replica_globals.max_size,
                            orte_gpr_replica_globals.block_size);

    reg->itags = NULL;
    reg->num_itags = 0;
    reg->triggers = NULL;
    reg->num_trigs = 0;

}

/* destructor - used to free any resources held by instance */
static void orte_gpr_replica_container_destructor(orte_gpr_replica_container_t* reg)
{
    orte_gpr_replica_itagval_t *ptr;
    int i;

    if (NULL != reg->itags) {
         free(reg->itags);
    }

    if (NULL != reg->triggers) {
        free(reg->triggers);
    }

    if (NULL != reg->itagvals) {
        ptr = (orte_gpr_replica_itagval_t*)((reg->itagvals)->addr);
        for (i=0; i < (reg->itagvals)->size; i++) {
            if (NULL != ptr) {
                OBJ_RELEASE(ptr);
            }
            ptr++;
        }
        OBJ_RELEASE(reg->itagvals);
    }
    
}

/* define instance of ompi_class_t */
OBJ_CLASS_INSTANCE(
         orte_gpr_replica_container_t,  /* type name */
         ompi_object_t, /* parent "class" name */
         orte_gpr_replica_container_construct, /* constructor */
         orte_gpr_replica_container_destructor); /* destructor */


/* ITAG-VALUE PAIR */
/* constructor - used to initialize state of itagval instance */
static void orte_gpr_replica_itagval_construct(orte_gpr_replica_itagval_t* ptr)
{
    ptr->itag = ORTE_GPR_REPLICA_ITAG_MAX;
    ptr->type = ORTE_NULL;
    (ptr->value).strptr = NULL;
}

/* destructor - used to free any resources held by instance */
static void orte_gpr_replica_itagval_destructor(orte_gpr_replica_itagval_t* ptr)
{
    if (ORTE_BYTE_OBJECT == ptr->type) {
        free(((ptr->value).byteobject).bytes);
    }
}

/* define instance of ompi_class_t */
OBJ_CLASS_INSTANCE(
         orte_gpr_replica_itagval_t,  /* type name */
         ompi_object_t, /* parent "class" name */
         orte_gpr_replica_itagval_construct, /* constructor */
         orte_gpr_replica_itagval_destructor); /* destructor */


/* NOTIFY TRACKER */
/* constructor - used to initialize state of notify tracker instance */
static void orte_gpr_replica_notify_tracker_construct(orte_gpr_replica_notify_tracker_t* trig)
{
    trig->requestor = NULL;
    trig->callback = NULL;
    trig->user_tag = NULL;
    trig->local_idtag = ORTE_GPR_NOTIFY_ID_MAX;
    trig->remote_idtag = ORTE_GPR_NOTIFY_ID_MAX;
    trig->segptr = NULL;
    trig->addr_mode = 0;
    trig->tokens = NULL;
    trig->num_tokens = 0;
    trig->key = NULL;
    trig->cmd = 0;
    trig->flag.trig_action = 0;
    trig->trigger = 0;
    trig->count = 0;
    trig->above_below = 0;
}

/* destructor - used to free any resources held by instance */
static void orte_gpr_replica_notify_tracker_destructor(orte_gpr_replica_notify_tracker_t* trig)
{
    char **tok;
    uint32_t i;

    if (NULL != trig->requestor) {
        free(trig->requestor);
    }

    if (NULL != trig->tokens) {
 for (i=0, tok=trig->tokens; i< trig->num_tokens; i++) {
        free(*tok);
        *tok = NULL;
       tok++;
 }
  free(trig->tokens);
    trig->tokens = NULL;
    }

    if (NULL != trig->key) {
        free(trig->key);
    }
    
}

/* define instance of ompi_class_t */
OBJ_CLASS_INSTANCE(
         orte_gpr_replica_notify_tracker_t,           /* type name */
         ompi_object_t,                 /* parent "class" name */
         orte_gpr_replica_notify_tracker_construct,   /* constructor */
         orte_gpr_replica_notify_tracker_destructor); /* destructor */


/* CALLBACKS */
/* constructor - used to initialize state of callback list instance */
static void orte_gpr_replica_callbacks_construct(orte_gpr_replica_callbacks_t* cb)
{
    cb->cb_func = NULL;
    cb->message = NULL;
    cb->requestor = NULL;
    cb->remote_idtag = 0;
    cb->user_tag = NULL;
}

/* destructor - used to free any resources held by instance */
static void orte_gpr_replica_callbacks_destructor(orte_gpr_replica_callbacks_t* cb)
{
    if (NULL != cb->requestor) {
      free(cb->requestor);
        cb->requestor = NULL;
    }
}

/* define instance of ompi_class_t */
OBJ_CLASS_INSTANCE(
         orte_gpr_replica_callbacks_t,           /* type name */
         ompi_list_item_t,            /* parent "class" name */
         orte_gpr_replica_callbacks_construct,   /* constructor */
         orte_gpr_replica_callbacks_destructor); /* destructor */


/* NOTIFY OFF */
/* constructor - used to initialize state of notify_off instance */
static void orte_gpr_replica_notify_off_construct(orte_gpr_replica_notify_off_t* off)
{
    off->sub_number = ORTE_GPR_NOTIFY_ID_MAX;
    off->proc = NULL;
}

/* destructor - used to free any resources held by notify_off instance */
static void orte_gpr_replica_notify_off_destructor(orte_gpr_replica_notify_off_t* off)
{
    if (NULL != off->proc) {
	    free(off->proc);
        off->proc = NULL;
    }
}

/* define instance of notify_off class */
OBJ_CLASS_INSTANCE(
		   orte_gpr_replica_notify_off_t,
		   ompi_list_item_t,
		   orte_gpr_replica_notify_off_construct,
		   orte_gpr_replica_notify_off_destructor);


/* REPLICA LIST - NOT IMPLEMENTED YET! */
/* constructor - used to initialize state of replica list instance */
static void orte_gpr_replica_list_construct(orte_gpr_replica_list_t* replica)
{
    replica->replica = NULL;
}

/* destructor - used to free any resources held by instance */
static void orte_gpr_replica_list_destructor(orte_gpr_replica_list_t* replica)
{
    if (NULL != replica->replica) {
	   free(replica->replica);
	   replica->replica = NULL;
    }
}

/* define instance of ompi_class_t */
OBJ_CLASS_INSTANCE(
		   orte_gpr_replica_list_t,           /* type name */
		   ompi_list_item_t,                 /* parent "class" name */
		   orte_gpr_replica_list_construct,   /* constructor */
		   orte_gpr_replica_list_destructor); /* destructor */


/* WRITE INVALIDATE - NOT IMPLEMENTED YET! */
/* define instance of ompi_class_t */
OBJ_CLASS_INSTANCE(
		   orte_gpr_replica_write_invalidate_t,            /* type name */
		   ompi_list_item_t,                          /* parent "class" name */
		   NULL,    /* constructor */
		   NULL);  /* destructor */



int orte_gpr_replica_open(void)
{
    int id;

    id = mca_base_param_register_int("gpr", "replica", "debug", NULL, 0);
    mca_base_param_lookup_int(id, &orte_gpr_replica_globals.debug);
    
    id = mca_base_param_register_int("gpr", "replica", "maxsize", NULL,
                                     ORTE_GPR_REPLICA_MAX_SIZE);
    mca_base_param_lookup_int(id, &orte_gpr_replica_globals.max_size);

    id = mca_base_param_register_int("gpr", "replica", "blocksize", NULL,
                                     ORTE_GPR_REPLICA_BLOCK_SIZE);
    mca_base_param_lookup_int(id, &orte_gpr_replica_globals.block_size);

    id = mca_base_param_register_int("gpr", "replica", "isolate", NULL, 0);
    mca_base_param_lookup_int(id, &orte_gpr_replica_globals.isolate);

    return ORTE_SUCCESS;
}

/*
 * close function
 */
int orte_gpr_replica_close(void)
{
    return ORTE_SUCCESS;
}

orte_gpr_base_module_t *orte_gpr_replica_init(bool *allow_multi_user_threads, bool *have_hidden_threads, int *priority)
{

    /* If we are to host a replica, then we want to be selected, so do all the
       setup and return the module */

    if (NULL == orte_process_info.gpr_replica) {
        int rc;

	/* Return a module (choose an arbitrary, positive priority --
	   it's only relevant compared to other ns components).  If
	   we're not the seed, then we don't want to be selected, so
	   return NULL. */

	*priority = 50;

	/* We allow multi user threads but don't have any hidden threads */

	*allow_multi_user_threads = true;
	*have_hidden_threads = false;

	/* setup the thread locks and condition variables */
	OBJ_CONSTRUCT(&orte_gpr_replica_globals.mutex, ompi_mutex_t);
	OBJ_CONSTRUCT(&orte_gpr_replica_globals.wait_for_compound_mutex, ompi_mutex_t);
	OBJ_CONSTRUCT(&orte_gpr_replica_globals.compound_cmd_condition, ompi_condition_t);

	/* initialize the registry compound mode */
	orte_gpr_replica_globals.compound_cmd_mode = false;
	orte_gpr_replica_globals.exec_compound_cmd_mode = false;
	orte_gpr_replica_globals.compound_cmd_waiting = 0;
	orte_gpr_replica_globals.compound_cmd = NULL;

	/* initialize the registry head */
    if (ORTE_SUCCESS != orte_pointer_array_init(&(orte_gpr_replica.segments),
                            orte_gpr_replica_globals.block_size,
                            orte_gpr_replica_globals.max_size,
                            orte_gpr_replica_globals.block_size)) {
        return NULL;
    }
    if (ORTE_SUCCESS != orte_pointer_array_init(&(orte_gpr_replica.triggers),
                            orte_gpr_replica_globals.block_size,
                            orte_gpr_replica_globals.max_size,
                            orte_gpr_replica_globals.block_size)) {
        return NULL;
    }

	/* initialize the notify request tracker */
    if (ORTE_SUCCESS != orte_pointer_array_init(&(orte_gpr_replica.triggers),
                            orte_gpr_replica_globals.block_size,
                            orte_gpr_replica_globals.max_size,
                            orte_gpr_replica_globals.block_size)) {
        return NULL;
    }

	/* initialize the callback list head */
	OBJ_CONSTRUCT(&orte_gpr_replica.callbacks, ompi_list_t);

	/* initialize the mode tracker */
	OBJ_CONSTRUCT(&orte_gpr_replica.notify_offs, ompi_list_t);

 	/* issue the non-blocking receive */ 
    if (!orte_gpr_replica_globals.isolate) {
	   rc = orte_rml.recv_buffer_nb(ORTE_RML_NAME_ANY, ORTE_RML_TAG_GPR, 0,
                                 orte_gpr_replica_recv, NULL);
	   if(rc != ORTE_SUCCESS && rc != ORTE_ERR_NOT_IMPLEMENTED) { 
	    return NULL;
	   }
    }

	if (orte_gpr_replica_globals.debug) {
	    ompi_output(0, "nb receive setup");
	}

	/* Return the module */

	initialized = true;
	return &orte_gpr_replica_module;
    } else {
	return NULL;
    }
}

/*
 * finalize routine
 */
int orte_gpr_replica_finalize(void)
{
    int i;
    orte_gpr_replica_segment_t* seg;
    orte_gpr_replica_notify_tracker_t* trig;
    orte_gpr_replica_callbacks_t* cb;
    orte_gpr_replica_notify_off_t* no;
    
    if (orte_gpr_replica_globals.debug) {
	    ompi_output(0, "finalizing gpr replica");
    }

    seg = (orte_gpr_replica_segment_t*)(orte_gpr_replica.segments)->addr;
    for (i=0; i < (orte_gpr_replica.segments)->size; i++) {
         if (NULL != seg) {
             OBJ_RELEASE(seg);
         }
         seg++;
    }
    
    trig = (orte_gpr_replica_notify_tracker_t*)(orte_gpr_replica.triggers)->addr;
    for (i=0; i < (orte_gpr_replica.triggers)->size; i++) {
         if (NULL != trig) {
             OBJ_RELEASE(trig);
         }
         trig++;
    }
    
    while (NULL != (cb = (orte_gpr_replica_callbacks_t*)ompi_list_remove_first(&orte_gpr_replica.callbacks))) {
        OBJ_RELEASE(cb);
    }
    OBJ_DESTRUCT(&orte_gpr_replica.callbacks);


    while (NULL != (no = (orte_gpr_replica_notify_off_t*)ompi_list_remove_first(&orte_gpr_replica.notify_offs))) {
        OBJ_RELEASE(no);
    }
    OBJ_DESTRUCT(&orte_gpr_replica.notify_offs);

    /* clean up the globals */
    if (NULL != orte_gpr_replica_globals.compound_cmd) {
        OBJ_RELEASE(orte_gpr_replica_globals.compound_cmd);
    }
    
    /* All done */
    if (orte_gpr_replica_globals.isolate) {
        return ORTE_SUCCESS;
    }
    
	orte_rml.recv_cancel(ORTE_RML_NAME_ANY, ORTE_RML_TAG_GPR);
    return ORTE_SUCCESS;
}

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

#include "ompi_config.h"

#include <string.h>
#include "class/ompi_hash_table.h"
#include "threads/condition.h"
#include "util/output.h"
#include "util/proc_info.h"

#include "mca/mca.h"
#include "mca/base/base.h"
#include "mca/errmgr/errmgr.h"
#include "mca/rml/rml.h"
#include "mca/gpr/gpr.h"
#include "mca/ns/ns.h"
#include "mca/pml/pml.h"
#include "mca/base/mca_base_module_exchange.h"
#include "runtime/runtime.h"

/**
 *
 */

/**
 * mca_base_modex_module_t
 *
 * Data for a specic proc and module.
 */

struct mca_base_modex_module_t {
    ompi_list_item_t super;
    mca_base_component_t component;
    void *module_data;
    size_t module_data_size;
    bool module_data_avail;
    ompi_condition_t module_data_cond;
};
typedef struct mca_base_modex_module_t mca_base_modex_module_t;

static void mca_base_modex_module_construct(mca_base_modex_module_t *module)
{
    OBJ_CONSTRUCT(&module->module_data_cond, ompi_condition_t);
    memset(&module->component, 0, sizeof(module->component));
    module->module_data = NULL;
    module->module_data_size = 0;
    module->module_data_avail = false;
}

static void mca_base_modex_module_destruct(mca_base_modex_module_t *module)
{
    OBJ_DESTRUCT(&module->module_data_cond);
}

OBJ_CLASS_INSTANCE(
    mca_base_modex_module_t, 
    ompi_list_item_t, 
    mca_base_modex_module_construct, 
    mca_base_modex_module_destruct
);

/**
 * mca_base_modex_t
 *
 * List of modules (mca_base_modex_module_t) for which data has been 
 * received from peers.
 */
struct mca_base_modex_t {
    ompi_object_t super;
    ompi_list_t modex_modules;
};
typedef struct mca_base_modex_t mca_base_modex_t;

static void mca_base_modex_construct(mca_base_modex_t* modex)
{
    OBJ_CONSTRUCT(&modex->modex_modules, ompi_list_t);
}

static void mca_base_modex_destruct(mca_base_modex_t* modex)
{
    OBJ_DESTRUCT(&modex->modex_modules);
}

OBJ_CLASS_INSTANCE(
    mca_base_modex_t, 
    ompi_object_t, 
    mca_base_modex_construct, 
    mca_base_modex_destruct
);

/**
 * mca_base_modex_subscription_t
 *
 * Track segments we have subscribed to.
 */

struct mca_base_modex_subscription_t {
    ompi_list_item_t item;
    orte_jobid_t jobid;
};
typedef struct mca_base_modex_subscription_t mca_base_modex_subscription_t;

OBJ_CLASS_INSTANCE(
    mca_base_modex_subscription_t,
    ompi_list_item_t,
    NULL,
    NULL);

/**
 * Globals to track the list of subscriptions.
 */

static ompi_list_t  mca_base_modex_subscriptions;
static ompi_mutex_t mca_base_modex_lock;


/**
 * Initialize global state.
 */
int mca_base_modex_init(void)
{
    OBJ_CONSTRUCT(&mca_base_modex_subscriptions, ompi_list_t);
    OBJ_CONSTRUCT(&mca_base_modex_lock, ompi_mutex_t);
    return OMPI_SUCCESS;
}

/**
 * Cleanup global state.
 */
int mca_base_modex_finalize(void)
{
    ompi_list_item_t *item;
    while(NULL != (item = ompi_list_remove_first(&mca_base_modex_subscriptions)))
        OBJ_RELEASE(item);
    OBJ_DESTRUCT(&mca_base_modex_subscriptions);
    return OMPI_SUCCESS;
}


/**
 *  Look to see if there is any data associated with a specified module.
 */

static mca_base_modex_module_t* mca_base_modex_lookup_module(
    mca_base_modex_t* modex,
    mca_base_component_t* component)
{
    mca_base_modex_module_t* modex_module;
    for(modex_module =  (mca_base_modex_module_t*)ompi_list_get_first(&modex->modex_modules);
        modex_module != (mca_base_modex_module_t*)ompi_list_get_end(&modex->modex_modules);
        modex_module =  (mca_base_modex_module_t*)ompi_list_get_next(modex_module)) {
        if(mca_base_component_compatible(&modex_module->component, component) == 0) {
            return modex_module;
        }
    }
    return NULL;
}


/**
 *  Create a placeholder for data associated with the specified module.
 */

static mca_base_modex_module_t* mca_base_modex_create_module(
    mca_base_modex_t* modex,
    mca_base_component_t* component)
{
    mca_base_modex_module_t* modex_module;
    if(NULL == (modex_module = mca_base_modex_lookup_module(modex, component))) {
        modex_module = OBJ_NEW(mca_base_modex_module_t);
        if(NULL != modex_module) {
            modex_module->component = *component;
            ompi_list_append(&modex->modex_modules, (ompi_list_item_t*)modex_module);
        }
    }
    return modex_module;
}


/**
 *  Callback for registry notifications.
 */

static void mca_base_modex_registry_callback(
    orte_gpr_notify_message_t* msg,
    void* cbdata)
{
    ompi_proc_t **new_procs = NULL;
    size_t new_proc_count = 0;
    int32_t i, j;
    orte_gpr_keyval_t **keyval;
    orte_gpr_value_t **value;
    ompi_proc_t *proc;
    char **token;
    orte_process_name_t *proc_name;
    mca_base_modex_t *modex;
    mca_base_modex_module_t *modex_module;
    mca_base_component_t component;
    bool isnew = false;
 

    /* process the callback */
    value = msg->values;
    for (i=0; i < msg->cnt; i++) {

        if (0 < value[i]->cnt) {  /* needs to be at least one value */
            new_procs = malloc(sizeof(ompi_proc_t*) * value[i]->cnt);

            /*
             * Token for the value should be the process name - look it up
             */
            token = value[i]->tokens;
            if (ORTE_SUCCESS == orte_ns.convert_string_to_process_name(&proc_name, token[0])) {
                proc = ompi_proc_find_and_add(proc_name, &isnew);
    
                if(NULL == proc)
                    continue;
                if(isnew) {
                    new_procs[new_proc_count] = proc;
                    new_proc_count++;
                }
    
                /*
                 * Lookup the modex data structure.
                 */
        
                OMPI_THREAD_LOCK(&proc->proc_lock);
                if(NULL == (modex = (mca_base_modex_t*)proc->proc_modex)) {
                    modex = OBJ_NEW(mca_base_modex_t);
                    if(NULL == modex) {
                        ompi_output(0, "mca_base_modex_registry_callback: unable to allocate mca_base_modex_t\n");
                        OMPI_THREAD_UNLOCK(&proc->proc_lock);
                        return;
                    }
                    proc->proc_modex = &modex->super;
                }
                
                /*
                 * Extract the component name and version from the keyval object's key
                 * Could be multiple keyvals returned since there is one for each
                 * component type/name/version - process them all
                 */
                keyval = value[i]->keyvals;
                for (j=0; j < value[i]->cnt; j++) {
                    if(sscanf(keyval[j]->key, "modex-%[^-]-%[^-]-%d-%d", 
                        component.mca_type_name,
                        component.mca_component_name,
                        &component.mca_component_major_version,
                        &component.mca_component_minor_version) != 4) {
                        ompi_output(0, "mca_base_modex_registry_callback: invalid component name %s\n", 
                            keyval[j]->key);
                        OMPI_THREAD_UNLOCK(&proc->proc_lock);
                        continue;
                    }
        
                    /*
                     * Lookup the corresponding modex structure
                     */
                    if(NULL == (modex_module = mca_base_modex_create_module(modex, &component))) {
                        ompi_output(0, "mca_base_modex_registry_callback: mca_base_modex_create_module failed\n");
                        OBJ_RELEASE(msg);
                        OMPI_THREAD_UNLOCK(&proc->proc_lock);
                        return;
                    }
        
                    /* 
                     * Create a copy of the data.
                     */
                    modex_module->module_data = (void*)keyval[j]->value.byteobject.bytes;
                    keyval[j]->value.byteobject.bytes = NULL;  /* dereference this pointer to avoid free'ng space */
                    modex_module->module_data_size = keyval[j]->value.byteobject.size;
                    modex_module->module_data_avail = true;
                    ompi_condition_signal(&modex_module->module_data_cond);
                }
            }  /* convert string to process name */
        }  /* if value[i]->cnt > 0 */
        
        /* update the pml/ptls with new proc */
        OMPI_THREAD_UNLOCK(&proc->proc_lock);
        
        if(NULL != new_procs) {
            mca_pml.pml_add_procs(new_procs, new_proc_count);
            free(new_procs);
        }
        
        /* relock the thread */
        OMPI_THREAD_LOCK(&proc->proc_lock);
    }
    
    /* unlock the thread to exit */
    OMPI_THREAD_UNLOCK(&proc->proc_lock);
}

/**
 * Make sure we have subscribed to this segment.
 */

static int mca_base_modex_subscribe(orte_process_name_t* name)
{
    orte_gpr_notify_id_t rctag;
    orte_gpr_value_t value;
    orte_jobid_t jobid;
    ompi_list_item_t* item;
    mca_base_modex_subscription_t* subscription;
    int rc;

    /* check for an existing subscription */
    OMPI_LOCK(&mca_base_modex_lock);
    for(item =  ompi_list_get_first(&mca_base_modex_subscriptions);
        item != ompi_list_get_end(&mca_base_modex_subscriptions);
        item = ompi_list_get_next(item)) {
        subscription = (mca_base_modex_subscription_t*)item;
        if(subscription->jobid == name->jobid) {
            OMPI_UNLOCK(&mca_base_modex_lock);
            return OMPI_SUCCESS;
        }
    }
    OMPI_UNLOCK(&mca_base_modex_lock);

    /* otherwise - subscribe */
    OBJ_CONSTRUCT(&value, orte_gpr_value_t);
    if (ORTE_SUCCESS != (rc = orte_ns.get_jobid(&jobid, name))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    
    if (ORTE_SUCCESS != (rc = orte_schema.get_job_segment_name(&(value.segment), jobid))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    
    value.cnt = 1;
    value.keyvals[0] = OBJ_NEW(orte_gpr_keyval_t);
    if (NULL == value.keyvals[0]) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    value.keyvals[0]->key = strdup("modex-*");
    value.keyvals[0]->type = ORTE_NULL;
    
    rc = orte_gpr.subscribe(
        ORTE_GPR_KEYS_OR | ORTE_GPR_TOKENS_OR,
        	ORTE_GPR_NOTIFY_ADD_ENTRY|ORTE_GPR_NOTIFY_DELETE_ENTRY|
        	ORTE_GPR_NOTIFY_MODIFICATION|
		ORTE_GPR_NOTIFY_ON_STARTUP|ORTE_GPR_NOTIFY_INCLUDE_STARTUP_DATA|
		ORTE_GPR_NOTIFY_PRE_EXISTING,
        	&value,
         &rctag,
        	mca_base_modex_registry_callback,
        	NULL);
    if(ORTE_SUCCESS != rc) {
        ompi_output(0, "mca_base_modex_exchange: "
		    "ompi_gpr.subscribe failed with return code %d\n", rc);
        OBJ_DESTRUCT(&value);
	    return OMPI_ERROR;
    }

    /* add this jobid to our list of subscriptions */
    OMPI_LOCK(&mca_base_modex_lock);
    subscription = OBJ_NEW(mca_base_modex_subscription_t);
    subscription->jobid = name->jobid;
    ompi_list_append(&mca_base_modex_subscriptions, &subscription->item);
    OMPI_UNLOCK(&mca_base_modex_lock);
    OBJ_DESTRUCT(&value);
    return OMPI_SUCCESS;
}


/**
 *  Store the data associated with the specified module in the
 *  gpr. Note that the gpr is in a mode where it caches
 *  individual puts during startup and sends them as an aggregate
 *  command.
 */

int mca_base_modex_send(
    mca_base_component_t *source_component, 
    const void *data, 
    size_t size)
{
    char *jobidstring;
    orte_gpr_value_t *value;
    int rc;

    value = OBJ_NEW(orte_gpr_value_t);
    if (NULL == value) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }

    if (ORTE_SUCCESS != (rc = orte_ns.get_jobid_string(&jobidstring, orte_process_info.my_name))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }

    if (0 > asprintf(&(value->segment), "%s-%s", ORTE_JOB_SEGMENT, jobidstring)) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }

    value->tokens = (char**)malloc(2*sizeof(char*));
    if (NULL == value->tokens) {
       ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    if (ORTE_SUCCESS != (rc = orte_ns.get_proc_name_string(&(value->tokens[0]), orte_process_info.my_name))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    value->tokens[1] = NULL;

    value->keyvals[0] = OBJ_NEW(orte_gpr_keyval_t);
    if (NULL == value->keyvals[0]) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    (value->keyvals[0])->type = ORTE_BYTE_OBJECT;
    (value->keyvals[0])->value.byteobject.size = size;
    (value->keyvals[0])->value.byteobject.bytes = (void *)malloc(size); 
    if(NULL == (value->keyvals[0])->value.byteobject.bytes) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return OMPI_ERR_OUT_OF_RESOURCE;
    }
    memcpy((value->keyvals[0])->value.byteobject.bytes, data, size);
    
    asprintf(&((value->keyvals[0])->key), "modex-%s-%s-%d-%d", 
        source_component->mca_type_name,
        source_component->mca_component_name,
        source_component->mca_component_major_version,
        source_component->mca_component_minor_version);
    
    rc = orte_gpr.put(
        ORTE_GPR_TOKENS_AND | ORTE_GPR_OVERWRITE, 
        1,
        &value);
        
    OBJ_RELEASE(value);

    return rc;
}


/**
 *  Retreive the data for the specified module from the source process.
 */

int mca_base_modex_recv(
    mca_base_component_t *component,
    ompi_proc_t *proc, 
    void **buffer, 
    size_t *size)
{
    mca_base_modex_t* modex;
    mca_base_modex_module_t* modex_module;

    /* check the proc for cached data */
    OMPI_THREAD_LOCK(&proc->proc_lock);
    if(NULL == (modex = (mca_base_modex_t*)proc->proc_modex)) {
        modex = OBJ_NEW(mca_base_modex_t);
        if(modex == NULL) {
            OMPI_THREAD_UNLOCK(&proc->proc_lock);
            return OMPI_ERR_OUT_OF_RESOURCE;
        }
        proc->proc_modex = &modex->super;

        /* verify that we have subscribed to this segment */
        OMPI_THREAD_UNLOCK(&proc->proc_lock);
        mca_base_modex_subscribe(&proc->proc_name);
        OMPI_THREAD_LOCK(&proc->proc_lock);
    }

    /* lookup/create the module */
    if(NULL == (modex_module = mca_base_modex_create_module(modex, component))) {
        OMPI_THREAD_UNLOCK(&proc->proc_lock);
        return OMPI_ERR_OUT_OF_RESOURCE;
    }

    /* wait until data is available */
    while(modex_module->module_data_avail == false) {
        ompi_condition_wait(&modex_module->module_data_cond, &proc->proc_lock);
    }

    /* copy the data out to the user */
    if(modex_module->module_data_size == 0) {
        *buffer = NULL;
        *size = 0;
    } else {
        void *copy = malloc(modex_module->module_data_size);
        if(copy == NULL) {
            return OMPI_ERR_OUT_OF_RESOURCE;
        }
        memcpy(copy, modex_module->module_data, modex_module->module_data_size);
        *buffer = copy;
        *size = modex_module->module_data_size;
    }
    OMPI_THREAD_UNLOCK(&proc->proc_lock);
    return OMPI_SUCCESS;
}


/**
 * Subscribe to the segment corresponding
 * to this job.
 */

int mca_base_modex_exchange(void)
{
    return mca_base_modex_subscribe(orte_process_info.my_name);
}



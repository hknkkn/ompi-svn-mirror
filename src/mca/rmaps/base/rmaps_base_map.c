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

#include "orte_config.h"
#include "include/orte_constants.h"
#include "include/orte_types.h"
#include "include/orte_schema.h"

#include "util/output.h"
#include "mca/mca.h"
#include "mca/gpr/gpr.h"
#include "mca/ns/ns.h"
#include "mca/rmgr/base/base.h"
#include "mca/base/mca_base_param.h"
#include "mca/rmaps/base/rmaps_base_map.h"
#include "mca/soh/soh_types.h"


/**
 * orte_rmaps_base_node_t
 */

static void orte_rmaps_base_node_construct(orte_rmaps_base_node_t* node)
{
    node->node_name = NULL;
    OBJ_CONSTRUCT(&node->node_procs, ompi_list_t);
}

static void orte_rmaps_base_node_destruct(orte_rmaps_base_node_t* node)
{
    if(NULL != node->node_name)
        free(node->node_name);
    OBJ_DESTRUCT(&node->node_procs);
}

OBJ_CLASS_INSTANCE(
    orte_rmaps_base_node_t,
    ompi_list_item_t,
    orte_rmaps_base_node_construct,
    orte_rmaps_base_node_destruct);

/**
 * orte_rmaps_base_proc_t
 */

static void orte_rmaps_base_proc_construct(orte_rmaps_base_proc_t* proc)
{
    proc->proc_node = NULL;
}

static void orte_rmaps_base_proc_destruct(orte_rmaps_base_proc_t* proc)
{
}

OBJ_CLASS_INSTANCE(
    orte_rmaps_base_proc_t,
    ompi_list_item_t,
    orte_rmaps_base_proc_construct,
    orte_rmaps_base_proc_destruct);


/**
 * orte_rmaps_base_map_t
 */

static void orte_rmaps_base_map_construct(orte_rmaps_base_map_t* map)
{
    map->app = NULL;
    map->procs = NULL;
    map->num_procs = 0;
    OBJ_CONSTRUCT(&map->nodes, ompi_list_t);
}

static void orte_rmaps_base_map_destruct(orte_rmaps_base_map_t* map)
{
    size_t i=0;
    ompi_list_item_t* item;
    for(i=0; i<map->num_procs; i++) {
        OBJ_RELEASE(map->procs[i]);
    }
    while(NULL != (item = ompi_list_remove_first(&map->nodes)))
        OBJ_RELEASE(item);
    if(NULL != map->procs) {
        free(map->procs);
    }
    if(NULL != map->app) {
        OBJ_RELEASE(map->app);
    }
    OBJ_DESTRUCT(&map->nodes);
}

OBJ_CLASS_INSTANCE(
    orte_rmaps_base_map_t,
    ompi_list_item_t,
    orte_rmaps_base_map_construct,
    orte_rmaps_base_map_destruct);


/*
 * Compare two proc entries
 */

static int orte_rmaps_value_compare(orte_gpr_value_t** val1, orte_gpr_value_t** val2)
{
    int32_t i;
    int32_t app1 = -1;
    int32_t app2 = -1;
    int32_t rank1 = -1;
    int32_t rank2 = -1;
    orte_gpr_value_t* value;

    for(i=0, value=*val1; i<value->cnt; i++) {
        orte_gpr_keyval_t* keyval = value->keyvals[i];
        if(strcmp(keyval->key, ORTE_PROC_RANK_KEY) == 0) {
            rank1 = keyval->value.i32;
            continue;
        }
        if(strcmp(keyval->key, ORTE_PROC_APP_CONTEXT_KEY) == 0) {
            app1 = keyval->value.i32;
            continue;
        }
    }
    for(i=0, value=*val2; i<value->cnt; i++) {
        orte_gpr_keyval_t* keyval = value->keyvals[i];
        if(strcmp(keyval->key, ORTE_PROC_RANK_KEY) == 0) {
            rank1 = keyval->value.i32;
            continue;
        }
        if(strcmp(keyval->key, ORTE_PROC_APP_CONTEXT_KEY) == 0) {
            app1 = keyval->value.i32;
            continue;
        }
    }
    if(app1 < app2)
        return -1;
    if(app1 > app2)
        return +1;
    if(rank1 < rank2)
        return -1;
    if(rank1 > rank2)
        return +1;
    return 0;
}


/**
 * Lookup node (if it exists) in the list. If it doesn't exist, create a new
 * node and append to the table.
 */

static orte_rmaps_base_node_t* 
orte_rmaps_lookup_node(ompi_list_t* nodes, char* node_name, orte_rmaps_base_proc_t* proc)
{
    ompi_list_item_t* item;
    orte_rmaps_base_node_t *node;
    for(item =  ompi_list_get_first(nodes);
        item != ompi_list_get_end(nodes);
        item =  ompi_list_get_next(item)) {
        node = (orte_rmaps_base_node_t*)item;
        if(strcmp(node->node_name, node_name) == 0)
            return node;
    }
    node = OBJ_NEW(orte_rmaps_base_node_t);
    node->node_name = strdup(node_name);
    ompi_list_append(&node->node_procs, &proc->super);
    ompi_list_prepend(nodes, &node->super);
    return NULL;
}


/**
 *  Query the process mapping from the registry.
 */

int orte_rmaps_base_get_map(orte_jobid_t jobid, ompi_list_t* mapping_list)
{
    orte_app_context_t** app_context = NULL;
    orte_rmaps_base_map_t** mapping = NULL;
    size_t i, num_context = 0;
    char* segment = NULL;
    char* jobid_str = NULL;
    orte_gpr_value_t** values;
    int32_t v, num_values;
    int rc;
    char* keys[] = {
        ORTE_PROC_RANK_KEY,
        ORTE_PROC_NAME_KEY,
        ORTE_PROC_APP_CONTEXT_KEY,
        ORTE_NODE_NAME_KEY,
        NULL
    };

    /* query the application context */
    if(ORTE_SUCCESS != (rc = orte_rmgr_base_get_app_context(jobid, &app_context, &num_context))) {
        return rc;
    }
    if(NULL == (mapping = malloc(sizeof(orte_rmaps_base_map_t*) * num_context))) {
        rc = ORTE_ERR_OUT_OF_RESOURCE;
        goto cleanup;
    }

    if(ORTE_SUCCESS != (rc = orte_ns.convert_jobid_to_string(&jobid_str, jobid))) {
        goto cleanup;
    }

    for(i=0; i<num_context; i++) {
        orte_rmaps_base_map_t* map = OBJ_NEW(orte_rmaps_base_map_t);
        orte_app_context_t* app = app_context[i];
        map->app = app;
        map->procs = malloc(sizeof(orte_rmaps_base_proc_t*) * app->num_procs);
        if(NULL == map->procs) {
            OBJ_RELEASE(map);
            rc = ORTE_ERR_OUT_OF_RESOURCE;
            goto cleanup;
        }
        map->num_procs = 0;
        mapping[i] = map;
    }
    asprintf(&segment, "%s-%s", ORTE_JOB_SEGMENT, jobid_str);

    /* query the process list from the registry */
    rc = orte_gpr.get(
        ORTE_GPR_OR,
        segment,
        NULL,
        keys,
        &num_values,
        &values);
    if(ORTE_SUCCESS != rc)
        goto cleanup;

    /* sort the response */
    qsort(values, num_values, sizeof(orte_gpr_value_t*), 
        (int (*)(const void*,const void*))orte_rmaps_value_compare);

    /* build the proc list */
    for(v=0; v<num_values; v++) {
        orte_gpr_value_t* value = values[v];
        orte_rmaps_base_map_t* map = NULL;
        orte_rmaps_base_proc_t* proc;
        char* node_name = NULL;
        int32_t kv;

        proc = OBJ_NEW(orte_rmaps_base_proc_t);
        if(NULL == proc) {
            rc = ORTE_ERR_OUT_OF_RESOURCE;
            goto cleanup;
        }
    
        for(kv = 0; kv<value->cnt; kv++) {
            orte_gpr_keyval_t* keyval = value->keyvals[kv];
            if(strcmp(keyval->key, ORTE_PROC_RANK_KEY) == 0) {
                proc->proc_rank = keyval->value.i32;
                continue;
            }
            if(strcmp(keyval->key, ORTE_PROC_NAME_KEY) == 0) {
                proc->proc_name  = keyval->value.proc;
                continue;
            }
            if(strcmp(keyval->key, ORTE_PROC_APP_CONTEXT_KEY) == 0) {
                int32_t app_index = keyval->value.i32;
                if(app_index >= (int32_t)num_context) {
                    ompi_output(0, "orte_rmaps_base_get_map: invalid context\n");
                    rc = ORTE_ERR_BAD_PARAM;
                    goto cleanup;
                }
                map = mapping[app_index];
                continue;
            }
            if(strcmp(keyval->key, ORTE_NODE_NAME_KEY) == 0) {
                node_name = keyval->value.strptr;
                continue;
            }
        }
        /* global record */
        if(NULL == map) {
            OBJ_RELEASE(proc);
            continue;
        }
        map->procs[map->num_procs++] = proc;
        proc->proc_node = orte_rmaps_lookup_node(&map->nodes, node_name, proc); 
    }

    /* release temporary variables */
    for(i=0; i<num_context; i++) {
        ompi_list_append(mapping_list, &mapping[i]->super);
    }
    free(segment);
    free(jobid_str);
    free(app_context);
    free(mapping);
    return ORTE_SUCCESS;

cleanup:
    if(NULL != segment)
        free(segment);
    if(NULL != jobid_str)
        free(jobid_str);
    if(NULL != app_context) {
        for(i=0; i<num_context; i++) {
            OBJ_RELEASE(app_context[i]);
        }
        free(app_context);
    }
    if(NULL != mapping) {
        for(i=0; i<num_context; i++) {
            if(NULL != mapping[i]) 
                OBJ_RELEASE(mapping[i]);
        }
        free(mapping);
    }
    return rc;
}

/**
 * Set the process mapping in the registry.
 */

int orte_rmaps_base_set_map(orte_jobid_t jobid, ompi_list_t* mapping_list)
{
    size_t i;
    size_t index=0;
    size_t num_procs = 0;
    size_t size;
    int rc = ORTE_SUCCESS;
    ompi_list_item_t* item;
    orte_gpr_value_t** values;

    for(item =  ompi_list_get_first(mapping_list);
        item != ompi_list_get_end(mapping_list);
        item =  ompi_list_get_next(item)) {
        orte_rmaps_base_map_t* map = (orte_rmaps_base_map_t*)item;
        num_procs += map->num_procs;
    }
    if(num_procs == 0)
        return ORTE_ERR_BAD_PARAM;

    /* allocate value array */
    size = sizeof(orte_gpr_value_t*) * num_procs;
    values = (orte_gpr_value_t**)malloc(size);
    if(NULL == values) {
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    for(i=0; i<num_procs; i++) {
         values[i] = OBJ_NEW(orte_gpr_value_t);
         if(NULL == values[i]) {
             size_t j;
             for(j=0; j<i; j++) {
                 OBJ_RELEASE(values[j]);
             }
             free(values);
             return ORTE_ERR_OUT_OF_RESOURCE;
         }
    }


    /* iterate through all processes and initialize value array */
    for(item =  ompi_list_get_first(mapping_list);
        item != ompi_list_get_end(mapping_list);
        item =  ompi_list_get_next(item)) {
        orte_rmaps_base_map_t* map = (orte_rmaps_base_map_t*)item;
        size_t p;
        for(p=0; p<map->num_procs; p++) {
            orte_rmaps_base_proc_t* proc = map->procs[p];
            orte_gpr_value_t* value = values[index++];
            orte_gpr_keyval_t** keyvals;
            size_t kv;

            /* allocate keyval array */
            size = sizeof(orte_gpr_keyval_t*) * 5;
            keyvals = (orte_gpr_keyval_t**)malloc(size);
            if(NULL == keyvals) {
                rc = ORTE_ERR_OUT_OF_RESOURCE;
                goto cleanup;
            }

            /* allocate keyvals */
            for(kv=0; kv < 5; kv++) {
                orte_gpr_keyval_t* value = OBJ_NEW(orte_gpr_keyval_t);
                if(value == NULL) {
                    rc = ORTE_ERR_OUT_OF_RESOURCE;
                    goto cleanup;
                }
                keyvals[kv] = value;
            } 

            /* initialize keyvals */
            keyvals[0]->key = strdup(ORTE_PROC_RANK_KEY);
            keyvals[0]->type = ORTE_INT32;
            keyvals[0]->value.i32 = proc->proc_rank;

            keyvals[1]->key = strdup(ORTE_PROC_NAME_KEY);
            keyvals[1]->type = ORTE_NAME;
            keyvals[1]->value.proc = proc->proc_name;

            keyvals[2]->key = strdup(ORTE_NODE_NAME_KEY);
            keyvals[2]->type = ORTE_INT32;
            keyvals[2]->value.strptr = strdup(proc->proc_node->node_name);

            keyvals[3]->key = strdup(ORTE_PROC_APP_CONTEXT_KEY);
            keyvals[3]->type = ORTE_INT32;
            keyvals[3]->value.i32 = map->app->idx;

            keyvals[4]->key = strdup(ORTE_PROC_STATE_KEY);
            keyvals[4]->type = ORTE_PROC_STATE;
            keyvals[4]->value.proc_state = ORTE_PROC_STATE_INIT;

            value->cnt = 5;
            value->keyvals = keyvals;
            if(ORTE_SUCCESS != (rc = orte_schema.get_job_segment_name(&value->segment,jobid))) {
                goto cleanup;
            }
            if(ORTE_SUCCESS != (rc = orte_schema.get_proc_tokens(&value->tokens,&value->num_tokens,&proc->proc_name))) {
                goto cleanup;
            }
        }
    }

    /* insert all values in one call */
    rc = orte_gpr.put(ORTE_GPR_AND, num_procs, values);
    orte_gpr.dump(0);

cleanup:
    for(i=0; i<num_procs; i++) {
        if(NULL != values[i]) {
            OBJ_RELEASE(values[i]);
        }
    }
    if(NULL != values)
        free(values);
    return rc;
}



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
#include "include/orte_names.h"

#include "util/output.h"
#include "mca/mca.h"
#include "mca/gpr/gpr.h"
#include "mca/ns/ns.h"
#include "mca/rmgr/base/base.h"
#include "mca/base/mca_base_param.h"
#include "mca/rmaps/base/rmaps_base_map.h"


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
    orte_rmaps_base_proc_t,
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
    orte_rmaps_base_node_t,
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
    map->nodes = NULL;
    map->num_procs = 0;
    map->num_nodes = 0;
}

static void orte_rmaps_base_map_destruct(orte_rmaps_base_map_t* map)
{
    size_t i=0;
    for(i=0; i<map->num_procs; i++) {
        OBJ_RELEASE(map->procs[i]);
    }
    for(i=0; i<map->num_nodes; i++) {
        OBJ_RELEASE(map->nodes[i]);
    }
    if(NULL != map->procs) {
        free(map->procs);
    }
    if(NULL != map->nodes) {
        free(map->nodes);
    }
    if(NULL != map->app) {
        OBJ_RELEASE(map->app);
    }
}

OBJ_CLASS_INSTANCE(
    orte_rmaps_base_map_t,
    ompi_list_item_t,
    orte_rmaps_base_map_construct,
    orte_rmaps_base_map_destruct);


/*
 * Compare two proc entries
 */

static int orte_rmaps_value_compare(orte_gpr_value_t* val1, orte_gpr_value_t* val2)
{
    int32_t i;
    int32_t app1 = -1;
    int32_t app2 = -1;
    int32_t rank1 = -1;
    int32_t rank2 = -1;

    for(i=0; i<val1->cnt; i++) {
        orte_gpr_keyval_t* keyval = val1->keyvals[i];
        if(strcmp(keyval->key, ORTE_PROC_RANK_KEY) == 0) {
            rank1 = keyval->value.i32;
            continue;
        }
        if(strcmp(keyval->key, ORTE_APP_CONTEXT_KEY) == 0) {
            app1 = keyval->value.i32;
            continue;
        }
    }
    for(i=0; i<val2->cnt; i++) {
        orte_gpr_keyval_t* keyval = val2->keyvals[i];
        if(strcmp(keyval->key, ORTE_PROC_RANK_KEY) == 0) {
            rank1 = keyval->value.i32;
            continue;
        }
        if(strcmp(keyval->key, ORTE_APP_CONTEXT_KEY) == 0) {
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
 * Lookup node (if it exists) in the hash table. If it doesn't exist, create a new
 * node and append to the table.
 */

static orte_rmaps_base_node_t* 
orte_rmaps_lookup_node(ompi_hash_table_t* hash, char* node_name, orte_rmaps_base_proc_t* proc)
{
    return NULL;
}


/**
 *  Query the process mapping from the registry.
 */

int orte_rmaps_base_get_map(orte_jobid_t jobid, orte_rmaps_base_map_t*** mapping_out, size_t* num_mapping_out)
{
    orte_app_context_t** app_context = NULL;
    orte_rmaps_base_map_t** mapping = NULL;
    ompi_hash_table_t nodes;
    size_t i, num_context = 0, num_procs = 0;
    char* segment = NULL;
    char* jobid_str = NULL;
    orte_gpr_value_t** values;
    int32_t v, num_values;
    int rc;
    char* keys[] = {
        ORTE_PROC_RANK_KEY,
        ORTE_PROC_NAME_KEY,
        ORTE_APP_CONTEXT_KEY,
        ORTE_NODE_NAME_KEY,
        NULL
    };
   
    OBJ_CONSTRUCT(&nodes, ompi_hash_table_t);

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
        num_procs += app->num_procs;
    }
    asprintf(&segment, "%s-%s", ORTE_JOB_SEGMENT, jobid_str);

    /* initialize hash table */
    ompi_hash_table_init(&nodes, (num_procs>>1)+1);

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
    qsort(values, num_values, sizeof(orte_gpr_value_t*), (int (*)(const void*,const void*))orte_rmaps_value_compare);

    /* build the proc list */
    for(v=0; v<num_values; v++) {
        orte_gpr_value_t* value = values[v];
        orte_rmaps_base_map_t* map;
        orte_rmaps_base_proc_t* proc;
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
            if(strcmp(keyval->key, ORTE_APP_CONTEXT_KEY) == 0) {
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
                proc->proc_node = orte_rmaps_lookup_node(&nodes, keyval->value.strptr, proc); 
                continue;
            }
        }
        /* global record */
        if(NULL == map) {
            OBJ_RELEASE(proc);
            continue;
        }
        map->procs[map->num_procs++] = proc;
    }

    /* build the node list */
    

    /* release temporary variables */
    free(segment);
    free(jobid_str);
    free(app_context);
    OBJ_DESTRUCT(&nodes);
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
    OBJ_DESTRUCT(&nodes);
    return rc;
}

/**
 * Set the process mapping in the registry.
 */

int orte_rmaps_base_set_map(orte_jobid_t jobid, orte_rmaps_base_map_t** map, size_t num_maps)
{
    return ORTE_SUCCESS;
}



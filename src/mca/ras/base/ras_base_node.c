#include "include/orte_names.h"
#include "include/orte_constants.h"
#include "mca/errmgr/errmgr.h"
#include "mca/gpr/gpr.h"
#include "mca/ns/ns.h"
#include "ras_base_node.h"

static void orte_ras_base_node_construct(orte_ras_base_node_t* node)
{
    node->node_name = NULL;
    node->node_cellid = 0;
    node->node_state = ORTE_NODE_STATE_UNKNOWN;
    node->node_slots = 0;
    node->node_slots_alloc = 0;
    node->node_slots_inuse = 0;
    node->node_slots_max = 0;
}

static void orte_ras_base_node_destruct(orte_ras_base_node_t* node)
{
    if(node->node_name != NULL)
        free(node->node_name);
}


OBJ_CLASS_INSTANCE(
    orte_ras_base_node_t,
    ompi_list_item_t,
    orte_ras_base_node_construct,
    orte_ras_base_node_destruct);


/*
 * Query the registry for all available nodes 
 */
int orte_ras_base_node_query(ompi_list_t* nodes)
{
    int i, cnt;
    orte_gpr_value_t** values;
    int rc;
    
    /* query all node entries */
    rc = orte_gpr.get(
        ORTE_GPR_OR,
        ORTE_NODE_SEGMENT,
        NULL,
        NULL,
        &cnt,
        &values);
    if(ORTE_SUCCESS != rc)
        return rc;

    /* parse the response */
    for(i=0; i<cnt; i++) {
        orte_gpr_value_t* value = values[i];
        orte_ras_base_node_t* node = OBJ_NEW(orte_ras_base_node_t);
        int k;

        for(k=0; k<value->cnt; k++) {
            orte_gpr_keyval_t* keyval = value->keyvals[k];
            if(strcmp(keyval->key, ORTE_NODE_NAME_KEY) == 0) {
                node->node_name = strdup(keyval->key);
                continue;
            }
            if(strcmp(keyval->key, ORTE_NODE_STATE_KEY) == 0) {
                node->node_state = keyval->value.node_state;
                continue;
            }
            if(strcmp(keyval->key, ORTE_NODE_SLOTS_KEY) == 0) {
                node->node_slots = keyval->value.ui32;
                continue;
            }
            if(strncmp(keyval->key, ORTE_NODE_SLOTS_ALLOC_KEY, strlen(ORTE_NODE_SLOTS_ALLOC_KEY)) == 0) {
                node->node_slots_inuse += keyval->value.ui32;
                continue;
            }
            if(strcmp(keyval->key, ORTE_NODE_SLOTS_MAX_KEY) == 0) {
                node->node_slots_max = keyval->value.ui32;
                continue;
            }
            if(strcmp(keyval->key, ORTE_CELLID_KEY) == 0) {
                node->node_cellid = keyval->value.cellid;
                continue;
            }
        }
        ompi_list_append(nodes, &node->super);
    }
    return ORTE_SUCCESS;
}

/*
 * Add the specified node definitions to the registry
 */
int orte_ras_base_node_insert(ompi_list_t* nodes)
{
    ompi_list_item_t* item;
    orte_gpr_value_t **values, *val;
    int rc, num_values, i, j;
    orte_ras_base_node_t* node;
    char* cellid;
    
    num_values = ompi_list_get_size(nodes);
    if (0 >= num_values) {
        return ORTE_ERR_BAD_PARAM;
    }
    
    values = (orte_gpr_value_t**)malloc(num_values * sizeof(orte_gpr_value_t*));
    if (NULL == values) {
       ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
       return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    for (i=0; i < num_values; i++) {
        values[i] = OBJ_NEW(orte_gpr_value_t);
        if (NULL == values[i]) {
            for (j=0; j < i; j++) {
                OBJ_RELEASE(values[j]);
            }
            free(values);
            ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
            return ORTE_ERR_OUT_OF_RESOURCE;
        }
        
        values[i]->segment = strdup(ORTE_NODE_SEGMENT);
        
        values[i]->cnt = 5;
        values[i]->keyvals = (orte_gpr_keyval_t**)malloc(5*sizeof(orte_gpr_keyval_t*));
        if (NULL == values[i]->keyvals) {
            for (j=0; j < i; j++) {
                OBJ_RELEASE(values[j]);
            }
            free(values);
            ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
            return ORTE_ERR_OUT_OF_RESOURCE;
        }
        
        for (j=0; j < 5; j++) {
            val->keyvals[j] = OBJ_NEW(orte_gpr_keyval_t);
            if (NULL == val->keyvals[j]) {
                for (j=0; j <= i; j++) {
                OBJ_RELEASE(values[j]);
                }
                free(values);
                ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                return ORTE_ERR_OUT_OF_RESOURCE;
            }
        }
        
        values[i]->num_tokens = 2;
        values[i]->tokens = (char**)malloc(2*sizeof(char*));
        if (NULL == values[i]->tokens) {
           for (j=0; j < i; j++) {
                OBJ_RELEASE(values[j]);
            }
            free(values);
            ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
            return ORTE_ERR_OUT_OF_RESOURCE;
        }
    }
    
    for(i=0, item =  ompi_list_get_first(nodes);
        i < num_values && item != ompi_list_get_end(nodes);
        i++, item =  ompi_list_get_next(nodes)) {

        node = (orte_ras_base_node_t*)item;

        val = values[i];
        
        (val->keyvals[0])->key = strdup(ORTE_NODE_NAME_KEY);
        (val->keyvals[0])->type = ORTE_STRING;
        (val->keyvals[0])->value.strptr = strdup(node->node_name);
        
        (val->keyvals[1])->key = strdup(ORTE_NODE_STATE_KEY);
        (val->keyvals[1])->type = ORTE_NODE_STATE;
        (val->keyvals[1])->value.node_state = node->node_state;
        
        (val->keyvals[2])->key = strdup(ORTE_CELLID_KEY);
        (val->keyvals[2])->type = ORTE_CELLID;
        (val->keyvals[2])->value.cellid = node->node_cellid;
        
        (val->keyvals[3])->key = strdup(ORTE_NODE_SLOTS_KEY);
        (val->keyvals[3])->type = ORTE_UINT32;
        (val->keyvals[3])->value.ui32 = node->node_slots;
        
        (val->keyvals[4])->key = strdup(ORTE_NODE_SLOTS_MAX_KEY);
        (val->keyvals[4])->type = ORTE_UINT32;
        (val->keyvals[4])->value.ui32 = node->node_slots_max;

        /* setup index/keys for this node */
        val->tokens[0] = strdup(node->node_name);
        if (ORTE_SUCCESS != (rc = orte_ns.convert_cellid_to_string(&(val->tokens[1]),
                                        node->node_cellid))) {
            for (j=0; j <= i; j++) {
                OBJ_RELEASE(values[j]);
            }
            free(values);
            return rc;
        }

        val->tokens[1] = cellid;
    }
    
    /* try the insert */
    rc = orte_gpr.put(
        ORTE_GPR_OVERWRITE|ORTE_GPR_AND,
        num_values,
        values);

    for (j=0; j < num_values; j++) {
          OBJ_RELEASE(values[j]);
    }
    free(values);
    
    if(ORTE_SUCCESS != rc)
        return rc;

    return ORTE_SUCCESS;
}

/*
 * Delete the specified nodes from the registry
 */
int orte_ras_base_node_delete(ompi_list_t* nodes)
{
    ompi_list_item_t* item;
    int rc;
    
    for(item =  ompi_list_get_first(nodes);
        item != ompi_list_get_end(nodes);
        item =  ompi_list_get_next(nodes)) {
        orte_ras_base_node_t* node = (orte_ras_base_node_t*)item;
        char* cellid;
        char* tokens[3];

        if(ORTE_SUCCESS != (rc = orte_ns.convert_cellid_to_string(&cellid, node->node_cellid)))
            return rc;

        /* setup index/keys for this node */
        tokens[0] = node->node_name;
        tokens[1] = cellid;
        tokens[2] = NULL;

        rc = orte_gpr.delete_entries(
            ORTE_GPR_AND,
            ORTE_NODE_SEGMENT,
            tokens,
            NULL);
        if(ORTE_SUCCESS != rc)
            return rc;
    }
    return ORTE_SUCCESS;
}

/*
 * Assign the allocated slots on the specified nodes to the  
 * indicated jobid.
 */
int orte_ras_base_node_assign(ompi_list_t* nodes, orte_jobid_t jobid)
{
    ompi_list_item_t* item;
    orte_gpr_value_t **values;
    int rc, num_values, i, j;
    orte_ras_base_node_t* node;
    char* jobid_str;
    
    num_values = ompi_list_get_size(nodes);
    if (0 >= num_values) {
        return ORTE_ERR_BAD_PARAM;
    }
    
    values = (orte_gpr_value_t**)malloc(num_values * sizeof(orte_gpr_value_t*));
    if (NULL == values) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    for (i=0; i < num_values; i++) {
        values[i] = OBJ_NEW(orte_gpr_value_t);
        if (NULL == values[i]) {
            for (j=0; j < i; j++) {
                OBJ_RELEASE(values[j]);
            }
            free(values);
            ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
            return ORTE_ERR_OUT_OF_RESOURCE;
        }
        
        values[i]->segment = strdup(ORTE_NODE_SEGMENT);
        
        values[i]->cnt = 1;
        values[i]->keyvals = (orte_gpr_keyval_t**)malloc(sizeof(orte_gpr_keyval_t*));
        if (NULL == values[i]->keyvals) {
            for (j=0; j < i; j++) {
                OBJ_RELEASE(values[j]);
            }
            free(values);
            ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
            return ORTE_ERR_OUT_OF_RESOURCE;
        }
        
        values[i]->keyvals[0] = OBJ_NEW(orte_gpr_keyval_t);
        if (NULL == values[i]->keyvals[0]) {
           for (j=0; j < i; j++) {
                OBJ_RELEASE(values[j]);
            }
            free(values);
            ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
            return ORTE_ERR_OUT_OF_RESOURCE;
        }
        
        values[i]->num_tokens = 2;
        values[i]->tokens = (char**)malloc(2*sizeof(char*));
        if (NULL == values[i]->tokens) {
           for (j=0; j < i; j++) {
                OBJ_RELEASE(values[j]);
            }
            free(values);
            ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
            return ORTE_ERR_OUT_OF_RESOURCE;
        }
    }
    
    for(i=0, item =  ompi_list_get_first(nodes);
        i < num_values && item != ompi_list_get_end(nodes);
        i++, item =  ompi_list_get_next(nodes)) {
          
        node = (orte_ras_base_node_t*)item;

        if(node->node_slots_alloc == 0)
            continue;
        if(ORTE_SUCCESS != (rc = orte_ns.convert_jobid_to_string(&jobid_str, jobid)))
            return rc;

        /* setup index/keys for this node */
        values[i]->tokens[0] = strdup(node->node_name);
        if (ORTE_SUCCESS != (rc = orte_ns.convert_cellid_to_string(
                                    &(values[i]->tokens[1]), node->node_cellid))) {
           for (j=0; j < num_values; j++) {
                OBJ_RELEASE(values[j]);
            }
            free(values);
            return rc;
        }

        /* setup node key/value pairs */
        asprintf(&((values[i]->keyvals[0])->key), "%s-%s", ORTE_NODE_SLOTS_ALLOC_KEY, jobid_str);
        free(jobid_str);
        
        (values[i]->keyvals[0])->type = ORTE_UINT32; 
        (values[i]->keyvals[0])->value.ui32 = node->node_slots_alloc;
    }
    
    /* try the insert */
    rc = orte_gpr.put(
        ORTE_GPR_OVERWRITE|ORTE_GPR_AND,
        num_values,
        values);
    
    for (j=0; j < num_values; j++) {
        OBJ_RELEASE(values[j]);
    }
    free(values);

    return rc;
}


#include "include/orte_names.h"
#include "include/orte_constants.h"
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
    orte_gpr_keyval_t node_name   = {{OBJ_CLASS(ompi_object_t), 0}, ORTE_NODE_NAME_KEY, ORTE_STRING };
    orte_gpr_keyval_t node_state  = {{OBJ_CLASS(ompi_object_t), 0}, ORTE_NODE_STATE_KEY, ORTE_NODE_STATE };
    orte_gpr_keyval_t node_cellid = {{OBJ_CLASS(ompi_object_t), 0}, ORTE_CELLID_KEY, ORTE_CELLID };
    orte_gpr_keyval_t node_slots  = {{OBJ_CLASS(ompi_object_t), 0}, ORTE_NODE_SLOTS_KEY, ORTE_UINT32 };
    orte_gpr_keyval_t node_slots_max = {{OBJ_CLASS(ompi_object_t), 0}, ORTE_NODE_SLOTS_MAX_KEY, ORTE_UINT32 };
    orte_gpr_keyval_t* keyvals[5];
    int rc;
    
    keyvals[0] = &node_name;
    keyvals[1] = &node_state;
    keyvals[2] = &node_cellid;
    keyvals[3] = &node_slots;
    keyvals[4] = &node_slots_max;

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

        /* setup node key/value pairs */
        node_name.value.strptr = node->node_name;
        node_state.value.node_state = node->node_state;
        node_cellid.value.cellid = node->node_cellid;
        node_slots.value.ui32 = node->node_slots;
        node_slots_max.value.ui32 = node->node_slots_max;

        /* try the insert */
        rc = orte_gpr.put(
            ORTE_GPR_OVERWRITE|ORTE_GPR_AND,
            ORTE_NODE_SEGMENT,
            tokens,
            sizeof(keyvals)/sizeof(keyvals[0]),
            keyvals);
        free(cellid);
        if(ORTE_SUCCESS != rc)
            return rc;
    }
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
    orte_gpr_keyval_t node_slots_inuse = {{OBJ_CLASS(ompi_object_t), 0}, ORTE_NODE_SLOTS_ALLOC_KEY, ORTE_UINT32 };
    orte_gpr_keyval_t* keyvals[1];
    int rc;
    
    keyvals[0] = &node_slots_inuse;
    for(item =  ompi_list_get_first(nodes);
        item != ompi_list_get_end(nodes);
        item =  ompi_list_get_next(nodes)) {
        orte_ras_base_node_t* node = (orte_ras_base_node_t*)item;
        char key[64];
        char* cellid_str;
        char* jobid_str;
        char* tokens[3];

        if(node->node_slots_alloc == 0)
            continue;
        if(ORTE_SUCCESS != (rc = orte_ns.convert_cellid_to_string(&cellid_str, node->node_cellid)))
            return rc;
        if(ORTE_SUCCESS != (rc = orte_ns.convert_jobid_to_string(&jobid_str, jobid)))
            return rc;

        /* setup index/keys for this node */
        tokens[0] = node->node_name;
        tokens[1] = cellid_str;
        tokens[2] = NULL;

        /* setup node key/value pairs */
        sprintf(key, "%s-%s", ORTE_NODE_SLOTS_ALLOC_KEY, jobid_str);
        free(jobid_str);
 
        node_slots_inuse.key = key;
        node_slots_inuse.value.ui32 = node->node_slots_alloc;

        /* try the insert */
        rc = orte_gpr.put(
            ORTE_GPR_OVERWRITE|ORTE_GPR_AND,
            ORTE_NODE_SEGMENT,
            tokens,
            sizeof(keyvals)/sizeof(keyvals[0]),
            keyvals);
        free(cellid_str);
        if(ORTE_SUCCESS != rc)
            return rc;
    }
    return ORTE_SUCCESS;
}


#include "include/orte_names.h"
#include "include/orte_constants.h"
#include "mca/gpr/gpr.h"
#include "ras_base_node.h"


/*
 * Query the registry for all available nodes 
 */
int orte_ras_base_node_query(ompi_list_t* list)
{
    int rc;
    int cnt;
    orte_gpr_value_t** values;
    
    /* query all node entries */
    rc = orte_gpr.get(
        ORTE_GPR_OR,
        ORTE_NODE_STATUS_SEGMENT,
        NULL,
        NULL,
        &cnt,
        &values);
    if(ORTE_SUCCESS != rc)
        return rc;

    /* parse the response */
    return ORTE_SUCCESS;
}

/*
 * Add the specified node definitions to the registry
 */
int orte_ras_base_node_insert(ompi_list_t* list)
{
    return ORTE_SUCCESS;
}

/*
 * Delete the specified nodes from the registry
 */
int orte_ras_base_node_delete(ompi_list_t* list)
{
    return ORTE_SUCCESS;
}

/*
 * Assign the allocated slots on the specified nodes to the  
 * indicated jobid.
 */
int orte_ras_base_node_assign(ompi_list_t* list, orte_jobid_t jobid)
{
    return ORTE_SUCCESS;
}


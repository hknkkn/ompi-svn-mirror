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
/**
 * @file
 *
 */
#ifndef ORTE_RAS_BASE_NODE_H
#define ORTE_RAS_BASE_NODE_H

#include "include/orte_types.h"
#include "mca/ras/ras.h"
#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

/*
 * Convience routines to query/set node state in the registry
 */

struct orte_ras_base_node_t {
    ompi_list_item_t super;
    char *node_name;
    orte_cellid_t node_cellid;
    orte_node_state_t node_state;
    size_t node_slots_inuse;        /* number of slots already assigned to existing jobs */
    size_t node_slots;              /* number of process slots */
    size_t node_slots_allocated;    /* number of slots that are being allocated to a new job */
    size_t node_slots_max;          /* maximum number of slots that can be allocated on this node */
};
typedef struct orte_ras_base_node_t orte_ras_base_node_t;


ORTE_DECLSPEC OBJ_CLASS_DECLARATION(orte_ras_base_node_t);


/*
 * Query the registry for all available nodes 
 */
int orte_ras_base_node_query(ompi_list_t*);

/*
 * Add the specified node definitions to the registry
 */
int orte_ras_base_node_insert(ompi_list_t*);

/*
 * Delete the specified nodes from the registry
 */
int orte_ras_base_node_delete(ompi_list_t*);

/*
 * Assign the allocated slots on the specified nodes to the  
 * indicated jobid.
 */
int orte_ras_base_node_assign(ompi_list_t*, orte_jobid_t);


#if defined(c_plusplus) || defined(__cplusplus)
}
#endif
#endif

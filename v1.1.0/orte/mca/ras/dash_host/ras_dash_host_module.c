/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

#include "orte_config.h"

#include "opal/util/output.h"
#include "opal/util/argv.h"
#include "orte/orte_constants.h"
#include "orte/orte_types.h"
#include "orte/mca/ras/base/base.h"
#include "orte/mca/ras/base/ras_base_node.h"
#include "orte/mca/rmgr/base/base.h"
#include "orte/mca/ras/base/ras_base_node.h"
#include "orte/mca/errmgr/errmgr.h"

#include "orte/mca/ras/dash_host/ras_dash_host.h"


/*
 * Local functions
 */
static int orte_ras_dash_host_allocate(orte_jobid_t jobid);
static int orte_ras_dash_host_deallocate(orte_jobid_t jobid);
static int orte_ras_dash_host_finalize(void);


/*
 * Local variables
 */
orte_ras_base_module_t orte_ras_dash_host_module = {
    orte_ras_dash_host_allocate,
    orte_ras_base_node_insert,
    orte_ras_base_node_query,
    orte_ras_dash_host_deallocate,
    orte_ras_dash_host_finalize
};


orte_ras_base_module_t *orte_ras_dash_host_init(int* priority)
{
    *priority = mca_ras_dash_host_component.priority;
    return &orte_ras_dash_host_module;
}


static int orte_ras_dash_host_allocate(orte_jobid_t jobid)
{
    opal_list_t nodes;
    opal_list_item_t* item;
    orte_app_context_t **context;
    size_t i, j, k, num_context;
    int rc;
    char **mapped_nodes = NULL, **mini_map;
    orte_ras_node_t *node;
    bool empty;

    /* If the node segment is not empty, do nothing */

    if (ORTE_SUCCESS != (rc = orte_ras_base_node_segment_empty(&empty))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    if (!empty) {
        opal_output(orte_ras_base.ras_output,
                    "orte:ras:dash_host: node segment not empty; not doing anything");
        return ORTE_SUCCESS;
    }

    /* Otherwise, get the context */

    rc = orte_rmgr_base_get_app_context(jobid, &context, &num_context);
    if (ORTE_SUCCESS != rc) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    OBJ_CONSTRUCT(&nodes, opal_list_t);

    /* If there's nothing to do, skip to the end */

    if (0 == num_context) {
        rc = ORTE_SUCCESS;
        goto cleanup;
    }

    /* Otherwise, go through the contexts */

    for (i = 0; i < num_context; ++i) {
        if (context[i]->num_map > 0) {
            orte_app_context_map_t** map = context[i]->map_data;

            /* Accumulate all of the host name mappings */
            for (j = 0; j < context[i]->num_map; ++j) {
                if (ORTE_APP_CONTEXT_MAP_HOSTNAME == map[j]->map_type) {
                    mini_map = opal_argv_split(map[j]->map_data, ',');

                    if (mapped_nodes == NULL) {
                        mapped_nodes = mini_map;
                    } else {
                        for (k = 0; NULL != mini_map[k]; ++k) {
                            rc = opal_argv_append_nosize(&mapped_nodes, 
                                                         mini_map[k]);
                            if (OPAL_SUCCESS != rc) {
                                goto cleanup;
                            }
                        }
                    }
                }
            }
        }
    }

    /* Did we find anything? */

    if (NULL != mapped_nodes) {

        /* Go through the names found and add them to the host list.
           If they're not unique, then bump the slots count for each
           duplicate */

        for (i = 0; NULL != mapped_nodes[i]; ++i) {
            for (item = opal_list_get_first(&nodes); 
                 item != opal_list_get_end(&nodes);
                 item = opal_list_get_next(item)) {
                node = (orte_ras_node_t*) item;
                if (0 == strcmp(node->node_name, mapped_nodes[i])) {
                    ++node->node_slots;
                    break;
                }
            }

            /* If we didn't find it, add it to the list */

            if (item == opal_list_get_end(&nodes)) {
                node = OBJ_NEW(orte_ras_node_t);
                if (NULL == node) {
                    return ORTE_ERR_OUT_OF_RESOURCE;
                }
                node->node_name = strdup(mapped_nodes[i]);
                node->node_arch = NULL;
                node->node_state = ORTE_NODE_STATE_UP;
                /* JMS: this should not be hard-wired to 0, but there's no
                   other value to put it to [yet]... */
                node->node_cellid = 0;
                node->node_slots_inuse = 0;
                node->node_slots_max = 0;
                node->node_slots = 1;
                opal_list_append(&nodes, &node->super);
            }
        }

        /* Put them on the segment and allocate them */

        if (ORTE_SUCCESS != 
            (rc = orte_ras_base_node_insert(&nodes)) ||
            ORTE_SUCCESS != 
            (rc = orte_ras_base_allocate_nodes(jobid, &nodes))) {
            goto cleanup;
        }
    }

cleanup:
    if (NULL != mapped_nodes) {
        opal_argv_free(mapped_nodes);
    }

    while (NULL != (item = opal_list_remove_first(&nodes))) {
        OBJ_RELEASE(item);
    }
    OBJ_DESTRUCT(&nodes);
    for (i = 0; i < num_context; i++) {
        OBJ_RELEASE(context[i]);
    }
    free(context);
    return rc;
}


static int orte_ras_dash_host_deallocate(orte_jobid_t jobid)
{
    /* Nothing to do */

    opal_output(orte_ras_base.ras_output, 
                "ras:dash_host:deallocate: success (nothing to do)");
    return ORTE_SUCCESS;
}


static int orte_ras_dash_host_finalize(void)
{
    /* Nothing to do */

    opal_output(orte_ras_base.ras_output, 
                "ras:dash_host:finalize: success (nothing to do)");
    return ORTE_SUCCESS;
}

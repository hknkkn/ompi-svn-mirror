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
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>
#include <string.h>

#include "include/orte_constants.h"
#include "class/ompi_list.h"
#include "util/output.h"
#include "mca/mca.h"
#include "mca/base/base.h"
#include "mca/ras/base/ras_base_node.h"
#include "rds_hostfile.h"
#include "rds_hostfile_lex.h"

#include "runtime/runtime_types.h"


static void orte_rds_hostfile_parse_error(void)
{
    ompi_output(0, "Error reading hostfile at line %d: %s\n",
        orte_rds_hostfile_line, orte_rds_hostfile_value.sval);
}


static int orte_rds_hostfile_parse_int(void)
{
    if (ORTE_RDS_HOSTFILE_EQUAL != orte_rds_hostfile_lex()) 
        return -1;
    if (ORTE_RDS_HOSTFILE_INT != orte_rds_hostfile_lex()) 
        return -1;
    return orte_rds_hostfile_value.ival;
}


static int orte_rds_hostfile_parse_line(int token, orte_ras_base_node_t* node)
{
    int rc;

    if (ORTE_RDS_HOSTFILE_STRING == token) {
        node->node_name = strdup(orte_rds_hostfile_value.sval);
        node->node_slots = 1;
    } else {
        orte_rds_hostfile_parse_error();
        return OMPI_ERROR;
    }

    while (!orte_rds_hostfile_done) {
        token = orte_rds_hostfile_lex();
        switch (token) {
        case ORTE_RDS_HOSTFILE_DONE:
            return OMPI_SUCCESS;

        case ORTE_RDS_HOSTFILE_NEWLINE:
            return OMPI_SUCCESS;

        case ORTE_RDS_HOSTFILE_COUNT:
        case ORTE_RDS_HOSTFILE_CPU:
        case ORTE_RDS_HOSTFILE_SLOTS:
            rc = orte_rds_hostfile_parse_int();
            if (rc < 0) 
                return OMPI_ERROR;
            node->node_slots = rc;
            break;

        case ORTE_RDS_HOSTFILE_SLOTS_MAX:
            rc = orte_rds_hostfile_parse_int();
            if (rc < 0) 
                return OMPI_ERROR;
            node->node_slots_max = rc;
            break;

        default:
            orte_rds_hostfile_parse_error();
            return OMPI_ERROR;
            break;
        }
    }
    return OMPI_SUCCESS;
}


/**
 * Parse the specified file into a node list.
 */

int orte_rds_hostfile_parse(const char *hostfile, ompi_list_t* list)
{
    orte_ras_base_node_t *node;
    int token, rc = ORTE_SUCCESS;

    OMPI_LOCK(&mca_rds_hostfile_component.lock);

    orte_rds_hostfile_done = false;
    orte_rds_hostfile_in = fopen(hostfile, "r");
    if (NULL == orte_rds_hostfile_in) {
        ompi_output(0, "orte_rds_hostfile_parse: could not open %s (%s)\n", hostfile, strerror(errno));
        rc = ORTE_ERR_BAD_PARAM;
        goto unlock;
    }

    while (!orte_rds_hostfile_done) {
        token = orte_rds_hostfile_lex();
        switch (token) {
        case ORTE_RDS_HOSTFILE_DONE:
            orte_rds_hostfile_done = true;
            break;

        case ORTE_RDS_HOSTFILE_NEWLINE:
            break;

        case ORTE_RDS_HOSTFILE_STRING:
            node = OBJ_NEW(orte_ras_base_node_t);
            rc = orte_rds_hostfile_parse_line(token, node);
            if (ORTE_SUCCESS != rc) {
                OBJ_RELEASE(node);
                goto unlock;
            }
            ompi_list_append(list, &node->super);
            break;

        default:
            orte_rds_hostfile_parse_error();
            goto unlock;
        }
    }
    fclose(orte_rds_hostfile_in);
    orte_rds_hostfile_in = NULL;

unlock:
    OMPI_UNLOCK(&mca_rds_hostfile_component.lock);
    return rc;
}


/**
 * Parse the default file as specified by the MCA parameter,
 * rds_hostfile_path, and add the nodes to the registry.
 */

static int orte_rds_hostfile_query(void)
{
    ompi_list_t nodes;
    ompi_list_item_t *item;
    int rc;

    OBJ_CONSTRUCT(&nodes, ompi_list_t);
    rc = orte_rds_hostfile_parse(mca_rds_hostfile_component.path, &nodes);
    if(ORTE_SUCCESS != rc) {
        goto cleanup;
    }
    rc = orte_ras_base_node_insert(&nodes);
   
cleanup:
    while(NULL != (item = ompi_list_remove_first(&nodes))) {
        OBJ_RELEASE(item);
    }
    OBJ_DESTRUCT(&nodes);
    return rc;
}


static int orte_rds_hostfile_finalize(void)
{
    return ORTE_SUCCESS;
}


orte_rds_base_module_t orte_rds_hostfile_module = {
    orte_rds_hostfile_query,
    orte_rds_hostfile_finalize
};


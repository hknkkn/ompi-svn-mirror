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
/** @file:
 *
 * The Open MPI general purpose registry - implementation.
 *
 */

/*
 * includes
 */

#include "orte_config.h"

#include "opal/util/trace.h"

#include "orte/orte_constants.h"
#include "orte/orte_types.h"
#include "orte/dss/dss.h"
#include "orte/mca/errmgr/errmgr.h"

#include "orte/mca/gpr/base/base.h"

int orte_gpr_base_pack_subscribe(orte_buffer_t *cmd,
                 size_t num_subs,
                 orte_gpr_subscription_t **subscriptions,
                 size_t num_trigs,
                 orte_gpr_trigger_t **trigs)
{
    orte_gpr_cmd_flag_t command;
    int rc;
    size_t zero=0;

    OPAL_TRACE(3);

    command = ORTE_GPR_SUBSCRIBE_CMD;

    /* can't be both NULL */
    if (NULL == subscriptions && NULL == trigs) {
        ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
        return ORTE_ERR_BAD_PARAM;
    }

    if (ORTE_SUCCESS != (rc = orte_dss.pack(cmd, &command, 1, ORTE_GPR_CMD))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }

    /* see if there are subscriptions - if so, pack them */
    if (NULL != subscriptions) {
        if (ORTE_SUCCESS != (rc = orte_dss.pack(cmd, subscriptions, num_subs, ORTE_GPR_SUBSCRIPTION))) {
            ORTE_ERROR_LOG(rc);
            return rc;
        }
    } else {
        if (ORTE_SUCCESS != (rc = orte_dss.pack(cmd, &zero, 1, ORTE_SIZE))) {
            ORTE_ERROR_LOG(rc);
            return rc;
        }
    }

    /* the API DOES allow there to be no triggers - if that happens, then trigs will be NULL and num_trigs
     * should be set to zero. we can't send that to the DPS though as it is an error condition over there,
     * so check for it here and record a "zero" if nothing is there
     */
    if (NULL != trigs && 0 < num_trigs) {
        if (ORTE_SUCCESS != (rc = orte_dss.pack(cmd, trigs, num_trigs, ORTE_GPR_TRIGGER))) {
            ORTE_ERROR_LOG(rc);
            return rc;
        }
    } else {
        if (ORTE_SUCCESS != (rc = orte_dss.pack(cmd, &zero, 1, ORTE_SIZE))) {
            ORTE_ERROR_LOG(rc);
            return rc;
        }
    }

    return ORTE_SUCCESS;
}


int orte_gpr_base_pack_unsubscribe(orte_buffer_t *cmd,
                  orte_gpr_subscription_id_t id)
{
    orte_gpr_cmd_flag_t command;
    int rc;

    OPAL_TRACE(3);

    command = ORTE_GPR_UNSUBSCRIBE_CMD;

    if (ORTE_SUCCESS != (rc = orte_dss.pack(cmd, &command, 1, ORTE_GPR_CMD))) {
       return rc;
    }

    if (ORTE_SUCCESS != (rc = orte_dss.pack(cmd, &id, 1, ORTE_GPR_SUBSCRIPTION_ID))) {
       return rc;
    }

    return ORTE_SUCCESS;
}

int orte_gpr_base_pack_cancel_trigger(orte_buffer_t *cmd, orte_gpr_trigger_id_t id)
{
    orte_gpr_cmd_flag_t command;
    int rc;

    OPAL_TRACE(3);

    command = ORTE_GPR_CANCEL_TRIGGER_CMD;

    if (ORTE_SUCCESS != (rc = orte_dss.pack(cmd, &command, 1, ORTE_GPR_CMD))) {
     return rc;
    }

    if (ORTE_SUCCESS != (rc = orte_dss.pack(cmd, &id, 1, ORTE_GPR_TRIGGER_ID))) {
       return rc;
    }

    return ORTE_SUCCESS;
}

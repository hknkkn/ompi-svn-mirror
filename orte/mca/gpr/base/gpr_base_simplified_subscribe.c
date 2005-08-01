/* -*- C -*-
 *
 * Copyright (c) 2004-2005 The Trustees of Indiana University.
 *                         All rights reserved.
 * Copyright (c) 2004-2005 The Trustees of the University of Tennessee.
 *                         All rights reserved.
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
/** @file 
 *
 */

#include "orte_config.h"
#include "orte/include/orte_constants.h"

#include "opal/util/output.h"

#include "orte/dps/dps.h"
#include "orte/mca/errmgr/errmgr.h"

#include "orte/mca/gpr/base/base.h"

int orte_gpr_base_subscribe_1(orte_gpr_subscription_id_t *id,
                              char *trig_name,
                              char *sub_name,
                              orte_gpr_notify_action_t action,
                              orte_gpr_addr_mode_t addr_mode,
                              char *segment,
                              char **tokens,
                              char *key,
                              orte_gpr_notify_cb_fn_t cbfunc,
                              void *user_tag)
{
    orte_gpr_value_t *values;
    orte_gpr_keyval_t *keyvals;
    orte_gpr_keyval_t keyval = { {OBJ_CLASS(opal_object_t),0},
                                  NULL, ORTE_NULL };
    orte_gpr_value_t value = { {OBJ_CLASS(opal_object_t),0},
                                ORTE_GPR_TOKENS_AND,
                                NULL, 0, NULL, 0, NULL };
    orte_gpr_subscription_t *subs;
    orte_gpr_subscription_t sub = { {OBJ_CLASS(opal_object_t),0},
                                     NULL, 0, 0, 0, NULL, 0, NULL };
    orte_gpr_trigger_t *trigs;
    orte_gpr_trigger_t trig = { {OBJ_CLASS(opal_object_t),0},
                                 NULL, 0, 0, 0, NULL, 0, NULL };
    int rc;
    
    /* assemble the subscription object */
    subs = &sub;
    sub.action = action;
    sub.cnt = 1;
    values = &value;
    sub.values = &values;
    sub.cbfunc = cbfunc;
    sub.user_tag = user_tag;
    
    value.addr_mode = addr_mode;
    value.segment = segment;
    value.cnt = 1;
    keyvals = &keyval;
    value.keyvals = &keyvals;
    value.tokens = tokens;

    keyval.key = key;

    /* assemble the trigger object - all we have here is the name*/
    trigs = &trig;
    trig.name = trig_name;
    
    /* send the subscription */
    if (ORTE_SUCCESS != (rc = orte_gpr.subscribe(1, &subs, 1, &trigs))) {
        ORTE_ERROR_LOG(rc);
    }
    
    /* no memory to cleanup since we didn't allocate anything */
    
    /* return the subscription id */
    *id = sub.id;
    
    return rc;
}


int orte_gpr_base_subscribe_N(orte_gpr_subscription_id_t *id,
                              char *trig_name,
                              char *sub_name,
                              orte_gpr_notify_action_t action,
                              orte_gpr_addr_mode_t addr_mode,
                              char *segment,
                              char **tokens,
                              size_t n,
                              char **keys,
                              orte_gpr_notify_cb_fn_t cbfunc,
                              void *user_tag)
{
    orte_gpr_value_t *values;
    orte_gpr_value_t value = { {OBJ_CLASS(opal_object_t),0},
                                ORTE_GPR_TOKENS_AND,
                                NULL, 0, NULL, 0, NULL };
    orte_gpr_subscription_t *subs;
    orte_gpr_subscription_t sub = { {OBJ_CLASS(opal_object_t),0},
                                     NULL, 0, 0, 0, NULL, 0, NULL };
    orte_gpr_trigger_t *trigs;
    orte_gpr_trigger_t trig = { {OBJ_CLASS(opal_object_t),0},
                                 NULL, 0, 0, 0, NULL, 0, NULL };
    size_t i, j;
    int rc;
    
    /* assemble the subscription object */
    subs = &sub;
    sub.name = sub_name;
    sub.action = action;
    sub.cnt = 1;
    values = &value;
    sub.values = &values;
    sub.cbfunc = cbfunc;
    sub.user_tag = user_tag;
    
    value.addr_mode = addr_mode;
    value.segment = segment;
    value.cnt = n;

    value.keyvals = (orte_gpr_keyval_t**)malloc(n * sizeof(orte_gpr_keyval_t*));
    if (NULL == value.keyvals) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    for (i=0; i < n; i++) {
        value.keyvals[i] = OBJ_NEW(orte_gpr_keyval_t);
        if (NULL == value.keyvals[i]) {
            ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
            for (j=0; j < i; j++) OBJ_RELEASE(value.keyvals[j]);
            free(value.keyvals);
            return ORTE_ERR_OUT_OF_RESOURCE;
        }
        value.keyvals[i]->key = keys[i];
    }

    value.tokens = tokens;

    /* assemble the trigger object - all we have here is the name */
    trigs = &trig;
    trig.name = trig_name;
    
    /* send the subscription */
    if (ORTE_SUCCESS != (rc = orte_gpr.subscribe(1, &subs, 1, &trigs))) {
        ORTE_ERROR_LOG(rc);
    }
    
    /* clean up memory - very carefully!
     * We can't use the object destructors because we didn't
     * copy input data fields into the objects. Thus, only
     * release the data that we explicitly allocated
     */
    for (i=0; i < n; i++) free(value.keyvals[i]);
    free(value.keyvals);

    /* return the subscription id */
    *id = sub.id;
    
    return rc;
}


int orte_gpr_base_define_trigger(orte_gpr_trigger_id_t *id,
                                 char *trig_name,
                                 orte_gpr_trigger_action_t action,
                                 orte_gpr_addr_mode_t addr_mode,
                                 char *segment,
                                 char **tokens,
                                 size_t n,
                                 char **keys,
                                 orte_gpr_trigger_cb_fn_t cbfunc,
                                 void *user_tag)
{
    orte_gpr_value_t *values;
    orte_gpr_value_t value = { {OBJ_CLASS(opal_object_t),0},
                                ORTE_GPR_TOKENS_AND,
                                NULL, 0, NULL, 0, NULL };
    orte_gpr_trigger_t *trigs;
    orte_gpr_trigger_t trig = { {OBJ_CLASS(opal_object_t),0},
                                 NULL, 0, 0, 0, NULL, 0, NULL };
    size_t i, j;
    int rc;
    
    /* assemble the trigger object */
    trig.name = trig_name;
    trig.action = action;
    trig.cnt = 1;
    values = &value;
    trig.values = &values;
    trig.cbfunc = cbfunc;
    trig.user_tag = user_tag;
    
    value.addr_mode = addr_mode;
    value.segment = segment;
    value.cnt = n;

    value.keyvals = (orte_gpr_keyval_t**)malloc(n * sizeof(orte_gpr_keyval_t*));
    if (NULL == value.keyvals) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    for (i=0; i < n; i++) {
        value.keyvals[i] = OBJ_NEW(orte_gpr_keyval_t);
        if (NULL == value.keyvals[i]) {
            ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
            for (j=0; j < i; j++) OBJ_RELEASE(value.keyvals[j]);
            free(value.keyvals);
            return ORTE_ERR_OUT_OF_RESOURCE;
        }
        value.keyvals[i]->key = keys[i];
    }

    value.tokens = tokens;

    /* send the subscription */
    if (ORTE_SUCCESS != (rc = orte_gpr.subscribe(0, NULL, 1, &trigs))) {
        ORTE_ERROR_LOG(rc);
    }
    
    /* clean up memory - very carefully!
     * We can't use the object destructors because we didn't
     * copy input data fields into the objects. Thus, only
     * release the data that we explicitly allocated
     */
    for (i=0; i < n; i++) free(value.keyvals[i]);
    free(value.keyvals);

    /* return the subscription id */
    *id = trig.id;
    
    return rc;
}


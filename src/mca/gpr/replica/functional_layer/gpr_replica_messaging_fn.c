/* -*- C -*-
 *
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
/** @file:
 *
 * The Open MPI General Purpose Registry - Replica component
 *
 */

/*
 * includes
 */
#include "orte_config.h"

#include "include/orte_constants.h"
#include "include/orte_schema.h"

#include "util/output.h"
#include "util/proc_info.h"

#include "mca/ns/ns.h"
#include "mca/errmgr/errmgr.h"

#include "mca/gpr/replica/communications/gpr_replica_comm.h"
#include "gpr_replica_fn.h"


int orte_gpr_replica_process_callbacks(void)
{
    orte_gpr_replica_callbacks_t *cb;

    /* aggregate messages for identical recipient - local messages just get called */

    /* send messages to de-aggregator - that function unpacks them and issues callbacks */
    if (orte_gpr_replica_globals.debug) {
	   ompi_output(0, "gpr replica: process_callbacks entered");
    }


    while (NULL != (cb = (orte_gpr_replica_callbacks_t*)ompi_list_remove_first(&orte_gpr_replica.callbacks))) {
	if (NULL == cb->requestor) {  /* local callback */
	    if (orte_gpr_replica_globals.debug) {
		    ompi_output(0, "process_callbacks: local");
	    }
	    cb->cb_func(cb->message, cb->user_tag);
	} else {  /* remote request - send message back */
	    if (orte_gpr_replica_globals.debug) {
		    ompi_output(0, "process_callbacks: remote to [%d,%d,%d]",
                    ORTE_NAME_ARGS(cb->requestor));
	    }
	    orte_gpr_replica_remote_notify(cb->requestor, cb->remote_idtag, cb->message);
	}
	OBJ_RELEASE(cb);
    }

    return ORTE_SUCCESS;
}


int
orte_gpr_replica_get_startup_msg_fn(orte_jobid_t jobid,
                                    orte_buffer_t *msg)
{
#if 0
    char *segment=NULL, *jobidstring=NULL;
    int rc, cnt, i;
    orte_gpr_value_t **values=NULL;
    orte_gpr_replica_segment_t *seg=NULL;
    orte_gpr_replica_itag_t toktag, keytag1, keytag2;
    orte_vpid_t vpid_start=ORTE_VPID_MAX, vpid_range=ORTE_VPID_MAX;
    orte_gpr_keyval_t **keyvals;
    orte_process_name_t *procs;
    
    int32_t size;
    ompi_buffer_t msg;
    ompi_list_t *returned_list;
    ompi_registry_value_t *value;
    bool found, include_data, done;
    size_t bufsize;

    if (orte_gpr_replica_globals.debug) {
	ompi_output(0, "[%d,%d,%d] entered construct_startup_msg for job %d",
		    ORTE_NAME_ARGS(*(orte_process_info.my_name)), (int)jobid);
    }

    /* setup tokens and segments for this job */
    if (ORTE_SUCCESS != (rc = orte_schema.get_job_segment_name(&segment, jobid))) {
        ORTE_ERROR_LOG(rc);
        return NULL;
    }

    /* find the specified segment */
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_find_seg(&seg, false, segment))) {
        ORTE_ERROR_LOG(rc);
        free(segment);
        return rc;
    }
    
    /* get vpid start and range from "global" container */
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_dict_lookup(&toktag, seg, ORTE_JOB_GLOBALS))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_dict_lookup(&keytags[0], seg, ORTE_JOB_VPID_START))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_dict_lookup(&keytags[1], seg, ORTE_JOB_VPID_RANGE))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_get_fn(ORTE_GPR_XAND,
                                    seg, &toktag, 1, &keytags, 2, &cnt, &values))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    if (0 > cnt || 0 > values[0]->cnt) {
        ORTE_ERROR_LOG(ORTE_ERR_NOT_FOUND);
        return ORTE_ERR_NOT_FOUND;
    }
    keyvals = values[0]->keyvals;
    for (i=0; i < values[0]->cnt; i++) {
        if (ORTE_VPID != keyvals[i]->type) {
            ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
            rc = ORTE_ERR_BAD_PARAM;
            goto CLEANUP;
        }
        if (0 == strcmp(keyvals[i]->key, ORTE_JOB_VPID_START)) {
            vpid_start = keyvals[i]->value.vpid;
        } else if (0 == strcmp(keyvals[i]->key, ORTE_JOB_VPID_RANGE)) {
            vpid_start = keyvals[i]->value.vpid;
        }
    }
    OBJ_RELEASE(values[0]);
    free(values);
    values = NULL;
    if (ORTE_VPID_MAX == vpid_start || ORTE_VPID_MAX == vpid_range) {
        ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
        rc = ORTE_ERR_BAD_PARAM;
        goto CLEANUP;
    }
    
    /* generate process recipient names */
    procs = (orte_gpr_process_name_t*)malloc(vpid_range * sizeof(orte_gpr_process_name_t));
    if (NULL == procs) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        rc = ORTE_ERR_OUT_OF_RESOURCE;
        goto CLEANUP;
    }
    
    for (i=0; i < vpid_range; i++) {
        if (ORTE_SUCCESS != (rc = orte_ns.create_process_name(&procs[i], 0, jobid, i+vpid_start))) {
            ORTE_ERROR_LOG(rc);
            goto CLEANUP;
        }
    }
    
    /* store the names in the msg buffer */
    if (ORTE_SUCCESS != (rc = orte_dps.pack(msg, &procs, (size_t)vpid_range, ORTE_NAME))) {
        ORTE_ERROR_LOG(rc);
        goto CLEANUP;
    }
    
    /* and release them */
    free(procs);
    procs = NULL;
    
    /* check list of subscriptions to find those on this segment */
    trig = (orte_gpr_replica_triggers_t**)(orte_gpr_replica.triggers)->addr;
    if (NULL == trig) {
        ORTE_ERROR_LOG(ORTE_ERR_NOT_FOUND);
        rc = ORTE_ERR_NOT_FOUND;
        goto CLEANUP;
    }
    for (i=0; i < (orte_gpr_replica.triggers)->size; i++) {
         if (NULL != trig[i] && seg == trig[i]->seg &&
             ORTE_GPR_SUBSCRIBE_CMD == trig[i]->cmd &&
             ((ORTE_GPR_NOTIFY_INCLUDE_STARTUP_DATA & trig[i]->flag.trig_action) ||/* see if they want data */
                (ORTE_GPR_NOTIFY_INCLUDE_SHUTDOWN_DATA & trig[i]->flag.trig_action ))) {
             /* ok, trigger is on specified segment and wants startup/shutdown data
              * use array of target pointers to collect data for return
              */
             /* create notify message for delivery later */
             notify = OBJ_NEW(orte_gpr_notify_message_t);
             if (NULL == notify) {
                 ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                 rc = ORTE_ERR_OUT_OF_RESOURCE;
                 goto CLEANUP;
             }
             notify->segment = strdup(segment);
             
             targets = (orte_gpr_replica_target_t**)(trig[i]->targets)->addr;
             for (j=0; j < (trig[i]->targets)->size; j++) {
                 if (NULL != targets[i]) {
                     


#endif
    return ORTE_ERR_NOT_IMPLEMENTED;
}



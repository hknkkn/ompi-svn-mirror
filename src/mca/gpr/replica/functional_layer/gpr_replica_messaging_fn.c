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

#include "gpr_replica_fn.h"


int orte_gpr_replica_process_callbacks(void)
{
#if 0
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
                    ORTE_NAME_ARGS(*(cb->requestor));
	    }
	    orte_gpr_replica_remote_notify(cb->requestor, cb->remote_idtag, cb->message);
	}
	OBJ_RELEASE(cb);
    }
    return ORTE_SUCCESS;

#endif
    return ORTE_ERR_NOT_IMPLEMENTED;
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
    free(segment);  /* done with this string */
    
    /* get vpid start and range from "global" container */
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_dict_lookup(&toktag, seg, "global"))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_dict_lookup(&keytags[0], seg, "vpid-start"))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_dict_lookup(&keytags[1], seg, "vpid-range"))) {
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
        if (0 == strcmp(keyvals[i]->key, "vpid-start")) {
            vpid_start = keyvals[i]->value.vpid;
        } else if (0 == strcmp(keyvals[i]->key, "vpid-range")) {
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
             ((ORTE_GPR_NOTIFY_INCLUDE_STARTUP_DATA & ) ||/* see if they want data */
                (ORTE_GPR_NOTIFY_INCLUDE_SHUTDOWN_DATA || ))) {
             /* ok, trigger is on specified segment and wants startup data */
             
	    include_data = false;

	    /* construct the list of recipients and find out if data is desired */
	    for (trig = (orte_gpr_replica_trigger_list_t*)ompi_list_get_first(&seg->triggers);
		 trig != (orte_gpr_replica_trigger_list_t*)ompi_list_get_end(&seg->triggers);
		 ) {
		next_trig = (orte_gpr_replica_trigger_list_t*)ompi_list_get_next(trig);

		if (OMPI_REGISTRY_NOTIFY_ON_STARTUP & trig->action) {

		    /* see if data is requested - only one trig has to ask for it */
		    if (OMPI_REGISTRY_NOTIFY_INCLUDE_STARTUP_DATA & trig->action) {
				include_data = true;
		    }

		    /***** if notify_one_shot is set, need to remove subscription from system */

		    /* find subscription on notify tracker */
		    done = false;
		    for (trackptr = (orte_gpr_replica_notify_request_tracker_t*)ompi_list_get_first(&orte_gpr_replica_notify_request_tracker);
			 trackptr != (orte_gpr_replica_notify_request_tracker_t*)ompi_list_get_end(&orte_gpr_replica_notify_request_tracker)
			     && !done;
			 trackptr = (orte_gpr_replica_notify_request_tracker_t*)ompi_list_get_next(trackptr)) {
			if (trackptr->local_idtag == trig->local_idtag) {
			    done = true;
			    if (NULL != trackptr->requestor) {
				name = trackptr->requestor;
			    } else {  /* local requestor */
				name = ompi_rte_get_self();
			    }
			    /* see if process already on list of recipients */
			    found = false;
			    for (ptr = (orte_name_services_namelist_t*)ompi_list_get_first(recipients);
				 ptr != (orte_name_services_namelist_t*)ompi_list_get_end(recipients) && !found;
				 ptr = (orte_name_services_namelist_t*)ompi_list_get_next(ptr)) {
                    if (ORTE_SUCCESS != orte_name_services.compare(&cmpval, ORTE_NS_CMP_ALL, name, ptr->name)) {
                        return NULL;
                    }
				   if (0 == cmpval) {
				       found = true;
				   }
			    }

			    if (!found) {
				/* check job status segment to verify recipient still alive */
                    if (ORTE_SUCCESS != orte_name_services.get_proc_name_string(tokens[0], name)) {
                        return NULL;
                    }
				   tokens[1] = NULL;

				/* convert tokens to array of keys */
				keys = orte_gpr_replica_get_key_list(proc_stat_seg, tokens, &num_keys);

				returned_list = OBJ_NEW(ompi_list_t);
				orte_gpr_replica_get_nl(returned_list,
						       OMPI_REGISTRY_XAND,
						       proc_stat_seg, keys, num_keys);

				free(tokens[0]);
				free(keys);

				if (NULL != (value = (ompi_registry_value_t*)ompi_list_remove_first(returned_list))) {
				    proc_status = ompi_rte_unpack_process_status(value);
				    if ((OMPI_PROC_KILLED != proc_status->status_key) &&
					(OMPI_PROC_STOPPED != proc_status->status_key)) {
					/* add process to list of recipients */
					peer = OBJ_NEW(orte_name_services_namelist_t);
                     if (ORTE_SUCCESS != orte_name_services.copy_process_name(peer->name, name)) {
                         return NULL;
                     }
					ompi_list_append(recipients, &peer->item);
				    }
				}
			    }
			}
		    }
		}
		trig = next_trig;
	    }

	    if (include_data) {  /* add in the data from all the registry entries on this segment */

		size = (int32_t)ompi_list_get_size(&seg->registry_entries); /* and number of data objects */
		ompi_pack(msg, &size, 1, OMPI_INT32);


		for (reg = (orte_gpr_replica_core_t*)ompi_list_get_first(&seg->registry_entries);
		     reg != (orte_gpr_replica_core_t*)ompi_list_get_end(&seg->registry_entries);
		     reg = (orte_gpr_replica_core_t*)ompi_list_get_next(reg)) {

		    /* add info to msg payload */
		    size = (int32_t)reg->object_size;
		    ompi_pack(msg, &size, 1, MCA_GPR_OOB_PACK_OBJECT_SIZE);
		    ompi_pack(msg, reg->object, reg->object_size, OMPI_BYTE);
		}
	    } else {
		size = 0;
		ompi_pack(msg, &size, 1, OMPI_INT32);
	    }
	}

	if (orte_gpr_replica_globals.debug) {
	    ompi_buffer_size(msg, &bufsize);
	    ompi_output(0, "[%d,%d,%d] built startup_msg of length %d with %d recipients",
			ORTE_NAME_ARGS(*ompi_rte_get_self()), bufsize, (int)ompi_list_get_size(recipients));
	    for (peer = (orte_name_services_namelist_t*)ompi_list_get_first(recipients);
		 peer != (orte_name_services_namelist_t*)ompi_list_get_end(recipients);
		 peer = (orte_name_services_namelist_t*)ompi_list_get_next(peer)) {
          if (ORTE_SUCCESS == orte_name_services.get_proc_name_string(procstring, peer->name)) {
		  ompi_output(0, "\trecipient: %s", procstring);
          }
	    }
	}
    }
#endif
    return ORTE_ERR_NOT_IMPLEMENTED;
}



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
    orte_gpr_notify_data_t **data;
    orte_gpr_replica_subscribed_data_t **sdata;
    orte_gpr_replica_triggers_t *trig;
    bool processed;
    int i, k;

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
            /* get this request off of the local notify request tracker */
            trig = (orte_gpr_replica_triggers_t*)((orte_gpr_replica.triggers)->addr[(cb->message)->idtag]);
            if (NULL == trig) {
                ORTE_ERROR_LOG(ORTE_ERR_GPR_DATA_CORRUPT);
                goto CLEANUP;
            }
            data = (cb->message)->data;
            sdata = (orte_gpr_replica_subscribed_data_t**)((trig->subscribed_data)->addr);
            for (i=0; i < (cb->message)->cnt; i++) {
                processed = false;
                for (k=0; k < (trig->subscribed_data)->size && !processed; k++) {
                    if (NULL != sdata[k] && sdata[k]->index == data[i]->cb_num) {
                        sdata[k]->callback(data[i], sdata[k]->user_tag);
                        processed = true;
                    }
                }
            }
    	   } else {  /* remote request - send message back */
    	       if (orte_gpr_replica_globals.debug) {
    		      ompi_output(0, "process_callbacks: remote to [%d,%d,%d]",
                        ORTE_NAME_ARGS(cb->requestor));
    	       }
    	       orte_gpr_replica_remote_notify(cb->requestor, cb->remote_idtag, cb->message);
    	   }
CLEANUP:
	   OBJ_RELEASE(cb);
    }

    return ORTE_SUCCESS;
}



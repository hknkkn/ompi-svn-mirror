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
 *
 */
/** @file 
 */

#include "orte_config.h"

#include "gpr_proxy.h"


int orte_gpr_proxy_deliver_notify_msg(orte_gpr_notify_action_t state,
				      orte_gpr_notify_message_t *message)
{
    int namelen;
    orte_gpr_proxy_notify_request_tracker_t *trackptr;

	/* don't deliver messages with zero data in them */
	if (0 < message->cnt) {
		
	    /* protect system from threadlock */
	    if (ORTE_GPR_NOTIFY_ON_STARTUP & state) {
	
			OMPI_THREAD_LOCK(&orte_gpr_proxy_mutex);
		
			namelen = strlen(message->segment);
		
			/* find the request corresponding to this notify */
			for (trackptr = (orte_gpr_proxy_notify_request_tracker_t*)ompi_list_get_first(&orte_gpr_proxy_notify_request_tracker);
			     trackptr != (orte_gpr_proxy_notify_request_tracker_t*)ompi_list_get_end(&orte_gpr_proxy_notify_request_tracker);
			     trackptr = (orte_gpr_proxy_notify_request_tracker_t*)ompi_list_get_next(trackptr)) {
			    if ((trackptr->action & state) &&
					(0 == strcmp(message->segment, trackptr->segment))) {
					OMPI_THREAD_UNLOCK(&orte_gpr_proxy_mutex);
					/* process request - callback function responsible for releasing memory */
					trackptr->callback(message, trackptr->user_tag);
					return ORTE_SUCCESS;
			    }
			}
             OMPI_THREAD_UNLOCK(&orte_gpr_proxy_mutex);
    		}
	}
    OBJ_RELEASE(message);
    return ORTE_ERROR;
}

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


int orte_gpr_proxy_deliver_notify_msg(orte_gpr_notify_message_t *message)
{
    orte_gpr_proxy_notify_tracker_t *trackptr;

	/* don't deliver messages with zero data in them */
	if (0 >= message->cnt) {
        OBJ_RELEASE(message);
        return ORTE_SUCCESS;
    }
		
	    /* protect system from threadlock */
	    OMPI_THREAD_LOCK(&orte_gpr_proxy_globals.mutex);
		
		/* locate the request corresponding to this notify */
        trackptr = (orte_gpr_proxy_notify_tracker_t*)
                    ((orte_gpr_proxy_globals.notify_tracker)->addr[message->idtag]);
        if (NULL == trackptr) {
            OMPI_THREAD_UNLOCK(&orte_gpr_proxy_globals.mutex);
            OBJ_RELEASE(message);
            return ORTE_ERR_BAD_PARAM;
        }
        
		/* process request - callback function responsible for releasing memory */
		trackptr->callback(message, trackptr->user_tag);

    OBJ_RELEASE(message);
    return ORTE_SUCCESS;
}

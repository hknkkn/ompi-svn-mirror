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
/** @file:
 *
 * The Open MPI general purpose registry - implementation.
 *
 */

/*
 * includes
 */

#include "orte_config.h"

#include "include/orte_constants.h"
#include "util/output.h"
#include "util/proc_info.h"

#include "mca/ns/ns_types.h"
#include "mca/oob/oob_types.h"
#include "mca/rml/rml.h"

#include "gpr_proxy.h"

int
orte_gpr_proxy_enter_notify_request(orte_gpr_notify_id_t *idtag, char *segment,
                    orte_gpr_notify_action_t action,
                    orte_gpr_notify_cb_fn_t cb_func,
                    void *user_tag)
{
    orte_gpr_proxy_notify_request_tracker_t *trackptr;

    trackptr = OBJ_NEW(orte_gpr_proxy_notify_request_tracker_t);
    if (NULL == trackptr) {
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    trackptr->segment = strdup(segment);
    trackptr->action = action;
    trackptr->callback = cb_func;
    trackptr->user_tag = user_tag;
    trackptr->remote_idtag = ORTE_GPR_NOTIFY_ID_MAX;
	trackptr->local_idtag = orte_gpr_proxy_next_notify_id_tag;
	orte_gpr_proxy_next_notify_id_tag++;

    if (orte_gpr_proxy_debug) {
        ompi_output(0, "[%d,%d,%d] enter_notify_request: tracker created for segment %s action %X idtag %d",
                    ORTE_NAME_ARGS(*(orte_process_info.my_name)), segment, action, trackptr->local_idtag);
    }
    
    *idtag = trackptr->local_idtag;
    return ORTE_SUCCESS;
}


int
orte_gpr_proxy_remove_notify_request(orte_gpr_notify_id_t local_idtag,
                                     orte_gpr_notify_id_t *remote_idtag)
{
    orte_gpr_proxy_notify_request_tracker_t *trackptr;

    /* locate corresponding entry on proxy tracker list and remove it */
    for (trackptr = (orte_gpr_proxy_notify_request_tracker_t*)ompi_list_get_first(&orte_gpr_proxy_notify_request_tracker);
	     trackptr != (orte_gpr_proxy_notify_request_tracker_t*)ompi_list_get_end(&orte_gpr_proxy_notify_request_tracker);
	     trackptr = (orte_gpr_proxy_notify_request_tracker_t*)ompi_list_get_next(trackptr)) {
         
         if (local_idtag == trackptr->local_idtag) {  /* found it */
             *remote_idtag = trackptr->remote_idtag;
             ompi_list_remove_item(&orte_gpr_proxy_notify_request_tracker, &trackptr->item);
        
             if (orte_gpr_proxy_debug) {
                 ompi_output(0, "[%d,%d,%d] remove_notify_request: tracker removed for segment %s action %X idtag %d",
                        ORTE_NAME_ARGS(*(orte_process_info.my_name)), trackptr->segment, trackptr->action, local_idtag);
             }
        
             /* release tracker item */
             OBJ_RELEASE(trackptr);
             return ORTE_SUCCESS;
         }
    }

    return ORTE_ERR_BAD_PARAM;
}


int orte_gpr_proxy_set_remote_idtag(orte_gpr_notify_id_t local_idtag,
                 orte_gpr_notify_id_t remote_idtag)
{
    orte_gpr_proxy_notify_request_tracker_t *trackptr;

    /* locate corresponding entry on proxy tracker list  */
    for (trackptr = (orte_gpr_proxy_notify_request_tracker_t*)ompi_list_get_first(&orte_gpr_proxy_notify_request_tracker);
	     trackptr != (orte_gpr_proxy_notify_request_tracker_t*)ompi_list_get_end(&orte_gpr_proxy_notify_request_tracker);
	     trackptr = (orte_gpr_proxy_notify_request_tracker_t*)ompi_list_get_next(trackptr)) {
     
         if (trackptr->local_idtag == local_idtag) {  /* found it */
             trackptr->remote_idtag = remote_idtag;
             return ORTE_SUCCESS;
         }
    }
    return ORTE_ERR_BAD_PARAM;
}


int orte_gpr_proxy_test_internals(int level, ompi_list_t *test_results)
{
    orte_buffer_t *cmd, *answer;
    int rc;

    test_results = OBJ_NEW(ompi_list_t);
    if (NULL == test_results) {
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    if (orte_gpr_proxy_compound_cmd_mode) {
	   return orte_gpr_base_pack_test_internals(orte_gpr_proxy_compound_cmd, level);
    }

    cmd = OBJ_NEW(orte_buffer_t);
    if (NULL == cmd) { /* got a problem */
	   return ORTE_ERR_OUT_OF_RESOURCE;
    }

    if (ORTE_SUCCESS != (rc = orte_gpr_base_pack_test_internals(cmd, level))) {
        OBJ_RELEASE(cmd);
	    return rc;
    }

    if (0 > orte_rml.send_buffer(orte_gpr_my_replica, cmd, MCA_OOB_TAG_GPR, 0)) {
	    return ORTE_ERR_COMM_FAILURE;
    }

    answer = OBJ_NEW(orte_buffer_t);
    if (NULL == answer) { /* got a problem */
        return ORTE_ERR_OUT_OF_RESOURCE;
    }

    if (0 > orte_rml.recv_buffer(orte_gpr_my_replica, answer, MCA_OOB_TAG_GPR)) {
        OBJ_RELEASE(answer);
	    return ORTE_ERR_COMM_FAILURE;
    }

    rc = orte_gpr_base_unpack_test_internals(answer, test_results);
    OBJ_RELEASE(answer);
    return rc;
}

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
#include "dps/dps.h"

#include "mca/errmgr/errmgr.h"
#include "mca/rml/rml.h"

#include "gpr_proxy.h"

int
orte_gpr_proxy_synchro(orte_gpr_addr_mode_t addr_mode,
                            orte_gpr_synchro_mode_t synchro_mode,
                            orte_gpr_value_t *value, int trigger,
                            orte_gpr_notify_id_t *synch_number,
                            orte_gpr_notify_cb_fn_t cb_func, void *user_tag)
{
    orte_buffer_t *cmd;
    orte_buffer_t *answer;
    int rc;
    orte_gpr_notify_id_t idtag, remote_idtag;
    orte_gpr_proxy_act_sync_t flag;
    
    *synch_number = ORTE_GPR_NOTIFY_ID_MAX;
    flag.trig_synchro = synchro_mode;
    
    /* need to protect against error */
    if (NULL == value || NULL == value->segment) {
        ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
        return ORTE_ERR_BAD_PARAM;
    }
    
    if (orte_gpr_proxy_globals.compound_cmd_mode) {
        	if (ORTE_SUCCESS != (rc = orte_gpr_base_pack_synchro(orte_gpr_proxy_globals.compound_cmd,
        						      synchro_mode, addr_mode, value, trigger))) {
            ORTE_ERROR_LOG(rc);
        	    return rc;
        	}
        
        	OMPI_THREAD_LOCK(&orte_gpr_proxy_globals.mutex);
        	/* store callback function and user_tag in local list for lookup */
        	/* generate id_tag to send to replica to identify lookup entry */
        
        if (ORTE_SUCCESS != (rc = orte_gpr_proxy_enter_notify_request(&idtag,
                                            value->segment, ORTE_GPR_SYNCHRO_CMD,
                                            &flag, cb_func, user_tag))) {
            ORTE_ERROR_LOG(rc);
            OMPI_THREAD_UNLOCK(&orte_gpr_proxy_globals.mutex);
            return rc;
        }
        
       	OMPI_THREAD_UNLOCK(&orte_gpr_proxy_globals.mutex);
        
        if (ORTE_SUCCESS != (rc = orte_dps.pack(orte_gpr_proxy_globals.compound_cmd,
                                    &idtag, 1, ORTE_GPR_NOTIFY_ID))) {
            ORTE_ERROR_LOG(rc);
            return rc;
        }
        
        *synch_number = idtag;
        	return ORTE_SUCCESS;
    }

    cmd = OBJ_NEW(orte_buffer_t);
    if (NULL == cmd) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    if (ORTE_SUCCESS != (rc = orte_gpr_base_pack_synchro(cmd, synchro_mode, addr_mode,
                                        value, trigger))) {
        OBJ_RELEASE(cmd);
        return rc;
    }

    OMPI_THREAD_LOCK(&orte_gpr_proxy_globals.mutex);
    /* store callback function and user_tag in local list for lookup */
    /* generate id_tag to send to replica to identify lookup entry */
    if (ORTE_SUCCESS != (rc = orte_gpr_proxy_enter_notify_request(&idtag,
                                                    value->segment, ORTE_GPR_SYNCHRO_CMD,
                                                    &flag, cb_func, user_tag))) {
        OMPI_THREAD_UNLOCK(&orte_gpr_proxy_globals.mutex);
        OBJ_RELEASE(cmd);
        return rc;
    }

    OMPI_THREAD_UNLOCK(&orte_gpr_proxy_globals.mutex);
    
    if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, &idtag, 1, ORTE_GPR_NOTIFY_ID))) {
       OBJ_RELEASE(cmd);
        return rc;
    }

    if (0 > orte_rml.send_buffer(orte_process_info.gpr_replica, cmd, ORTE_RML_TAG_GPR, 0)) {
         OMPI_THREAD_LOCK(&orte_gpr_proxy_globals.mutex);
         orte_gpr_proxy_remove_notify_request(idtag, &remote_idtag);
         OMPI_THREAD_UNLOCK(&orte_gpr_proxy_globals.mutex);
        return ORTE_ERR_COMM_FAILURE;
    }

    answer = OBJ_NEW(orte_buffer_t);
    if (NULL == answer) {
        OMPI_THREAD_LOCK(&orte_gpr_proxy_globals.mutex);
        orte_gpr_proxy_remove_notify_request(idtag, &remote_idtag);
        OMPI_THREAD_UNLOCK(&orte_gpr_proxy_globals.mutex);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    if (0 > orte_rml.recv_buffer(orte_process_info.gpr_replica, answer, ORTE_RML_TAG_GPR)) {
        OMPI_THREAD_LOCK(&orte_gpr_proxy_globals.mutex);
        orte_gpr_proxy_remove_notify_request(idtag, &remote_idtag);
        OMPI_THREAD_UNLOCK(&orte_gpr_proxy_globals.mutex);
        OBJ_RELEASE(answer);
        return ORTE_ERR_COMM_FAILURE;
    }

    if (ORTE_SUCCESS != (rc = orte_gpr_base_unpack_synchro(answer, &remote_idtag))) {
        OMPI_THREAD_LOCK(&orte_gpr_proxy_globals.mutex);
        orte_gpr_proxy_remove_notify_request(idtag, &remote_idtag);
        OMPI_THREAD_UNLOCK(&orte_gpr_proxy_globals.mutex);
        return rc;
    }

    /* set the remote id tag field */
    if (ORTE_SUCCESS != (rc = orte_gpr_proxy_set_remote_idtag(idtag, remote_idtag))) {
        OMPI_THREAD_LOCK(&orte_gpr_proxy_globals.mutex);
        orte_gpr_proxy_remove_notify_request(idtag, &remote_idtag);
        OMPI_THREAD_UNLOCK(&orte_gpr_proxy_globals.mutex);
        return rc;
    }

    *synch_number = idtag;
    return ORTE_SUCCESS;
}

int orte_gpr_proxy_cancel_synchro(orte_gpr_notify_id_t synch_number)
{
    orte_buffer_t *cmd;
    orte_buffer_t *answer;
    int rc;
    orte_gpr_notify_id_t remote_idtag;

    if (orte_gpr_proxy_globals.compound_cmd_mode) {
        if (ORTE_SUCCESS != (rc = orte_gpr_base_pack_cancel_synchro(orte_gpr_proxy_globals.compound_cmd,
                                        synch_number))) {
            return rc;
        }

        OMPI_THREAD_LOCK(&orte_gpr_proxy_globals.mutex);

        /* remove the notify tag */
        if (ORTE_SUCCESS != (rc = orte_gpr_proxy_remove_notify_request(synch_number, &remote_idtag))) {
            OMPI_THREAD_UNLOCK(&orte_gpr_proxy_globals.mutex);
            return rc;
        }
        
        rc = orte_dps.pack(orte_gpr_proxy_globals.compound_cmd, &remote_idtag, 1, ORTE_GPR_NOTIFY_ID);
           
        OMPI_THREAD_UNLOCK(&orte_gpr_proxy_globals.mutex);
        return rc;
    }

    cmd = OBJ_NEW(orte_buffer_t);
    if (NULL == cmd) {
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    OMPI_THREAD_LOCK(&orte_gpr_proxy_globals.mutex);
    if (ORTE_SUCCESS != (rc = orte_gpr_proxy_remove_notify_request(synch_number, &remote_idtag))) {
        OBJ_RELEASE(cmd);
        OMPI_THREAD_UNLOCK(&orte_gpr_proxy_globals.mutex);
        return rc;
    }
    OMPI_THREAD_UNLOCK(&orte_gpr_proxy_globals.mutex);    

    if (ORTE_SUCCESS != (rc = orte_gpr_base_pack_cancel_synchro(cmd, remote_idtag))) {
        OBJ_RELEASE(cmd);
        return rc;
    }

    if (0 > orte_rml.send_buffer(orte_process_info.gpr_replica, cmd, ORTE_RML_TAG_GPR, 0)) {
       return ORTE_ERR_COMM_FAILURE;
    }

    answer = OBJ_NEW(orte_buffer_t);
    if (NULL == answer) {
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    if (0 > orte_rml.recv_buffer(orte_process_info.gpr_replica, answer, ORTE_RML_TAG_GPR)) {
        OBJ_RELEASE(answer);
        return ORTE_ERR_COMM_FAILURE;
    }

    rc = orte_gpr_base_unpack_cancel_synchro(answer);
    OBJ_RELEASE(answer);
    return rc;

}

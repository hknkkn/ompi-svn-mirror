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

#include "dps/dps.h"
#include "mca/errmgr/errmgr.h"
#include "util/proc_info.h"

#include "gpr_replica_comm.h"

int orte_gpr_replica_recv_subscribe_cmd(orte_process_name_t* sender,
                                        orte_buffer_t *input_buffer,
                                        orte_buffer_t *output_buffer)
{
    orte_gpr_cmd_flag_t command=ORTE_GPR_SUBSCRIBE_CMD;
    orte_gpr_addr_mode_t addr_mode;
    char *segment=NULL, **tokens=NULL, **keys=NULL;
    orte_gpr_notify_id_t local_idtag=0, idtag=0;
    orte_gpr_replica_segment_t *seg=NULL;
    orte_gpr_replica_itag_t *token_itags, *key_itags;
    int num_keys=0, num_tokens=0, rc;
    int i=0;
    size_t n;
    orte_gpr_replica_act_sync_t flag;
    
    if (ORTE_SUCCESS != (rc = orte_dps.pack(output_buffer, &command, 1, ORTE_GPR_CMD))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    
    n = 1;
    if (ORTE_SUCCESS != (rc = orte_dps.unpack(input_buffer, &addr_mode, &n, ORTE_GPR_ADDR_MODE))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }

    n = 1;
    if (ORTE_SUCCESS != (rc = orte_dps.unpack(input_buffer, &flag.trig_action, &n, ORTE_NOTIFY_ACTION))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }

    n = 1;
    if (ORTE_SUCCESS != (rc = orte_dps.unpack(input_buffer, &segment, &n, ORTE_STRING))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }

    n = 1;
    if (ORTE_SUCCESS != (rc = orte_dps.unpack(input_buffer, &num_tokens, &n, ORTE_INT))) {
        ORTE_ERROR_LOG(rc);
        goto RETURN_ERROR;
    }

    if (0 < num_tokens) {  /* tokens provided */ 
        tokens = (char**)malloc(num_tokens*sizeof(char*));
        if (NULL == tokens) {
            ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
            goto RETURN_ERROR;
        }
        if (ORTE_SUCCESS != (rc = orte_dps.unpack(input_buffer, tokens, (size_t*)&num_tokens, ORTE_STRING))) {
            ORTE_ERROR_LOG(rc);
            goto RETURN_ERROR;
        }
    } else {  /* no tokens provided - wildcard case */
        tokens = NULL;
    }

    n = 1;
    if (ORTE_SUCCESS != (rc = orte_dps.unpack(input_buffer, &num_keys, &n, ORTE_INT))) {
        ORTE_ERROR_LOG(rc);
        goto RETURN_ERROR;
    }

    if (0 < num_keys) {  /* keys provided */ 
        keys = (char**)malloc(num_keys*sizeof(char*));
        if (NULL == keys) {
            ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
            goto RETURN_ERROR;
        }
        if (ORTE_SUCCESS != (rc = orte_dps.unpack(input_buffer, keys, (size_t*)&num_keys, ORTE_STRING))) {
            ORTE_ERROR_LOG(rc);
            goto RETURN_ERROR;
        }
    } else {  /* no tokens provided - wildcard case */
        tokens = NULL;
    }

    n = 1;
    if (ORTE_SUCCESS != orte_dps.unpack(input_buffer, &idtag, &n, ORTE_GPR_NOTIFY_ID)) {
        ORTE_ERROR_LOG(rc);
        goto RETURN_ERROR;
    }

    /*******   LOCK    *****/
    OMPI_THREAD_LOCK(&orte_gpr_replica_globals.mutex);

    /* locate the segment */
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_find_seg(&seg, true, segment))) {
        ORTE_ERROR_LOG(rc);
        OMPI_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);
        goto RETURN_ERROR;
    }

    /* convert tokens to itags */
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_get_itag_list(&token_itags,
                         seg, tokens, &num_tokens))) {
        ORTE_ERROR_LOG(rc);
        OMPI_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);
        goto RETURN_ERROR;
    }

    /* convert keys to itags */
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_get_itag_list(&key_itags,
                         seg, keys, &num_keys))) {
        ORTE_ERROR_LOG(rc);
        OMPI_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);
        goto RETURN_ERROR;
    }

    if (NULL != sender) {  /* remote sender */

      if (orte_gpr_replica_globals.debug) {
          ompi_output(0, "[%d,%d,%d] subscribe requested for remote sender [%d,%d,%d] on segment %s for idtag %d",
                       ORTE_NAME_ARGS(*orte_process_info.my_name), ORTE_NAME_ARGS(*sender), segment, idtag);
       }
      
        /* enter request on local notify tracking system */
        if (ORTE_SUCCESS != (rc = orte_gpr_replica_enter_notify_request(&local_idtag, seg,
                                        ORTE_GPR_SUBSCRIBE_CMD, &flag,
                                        sender, idtag, NULL, NULL))) {
            ORTE_ERROR_LOG(rc);
            OMPI_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);
            goto RETURN_ERROR;
        }

        /* register subscription */
        if (ORTE_SUCCESS != (rc = orte_gpr_replica_subscribe_fn(addr_mode, seg,
                          token_itags, num_tokens, key_itags, num_keys, local_idtag))) {
            orte_gpr_replica_remove_notify_request(local_idtag, &idtag);
            ORTE_ERROR_LOG(rc);
            OMPI_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);
            goto RETURN_ERROR;
        }

        /* pack the local idtag for return to sender */
        if (ORTE_SUCCESS != (rc = orte_dps.pack(output_buffer, &local_idtag, 1, ORTE_GPR_NOTIFY_ID))) {
            ORTE_ERROR_LOG(rc);
            OMPI_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);
            goto RETURN_ERROR;
        }

    } else {  /* local sender - idtag is for local notify tracking system */
        /* register subscription */
        if (ORTE_SUCCESS != (rc = orte_gpr_replica_subscribe_fn(addr_mode, seg,
                          token_itags, num_tokens, key_itags, num_keys, idtag))) {
            orte_gpr_replica_remove_notify_request(idtag, &idtag);
            ORTE_ERROR_LOG(rc);
            OMPI_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);
            goto RETURN_ERROR;
        }
        
        /* pack the local idtag for return to sender */
        if (ORTE_SUCCESS != (rc = orte_dps.pack(output_buffer, &idtag, 1, ORTE_GPR_NOTIFY_ID))) {
            ORTE_ERROR_LOG(rc);
            OMPI_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);
            goto RETURN_ERROR;
        }

    }

    orte_gpr_replica_check_subscriptions(seg, ORTE_GPR_REPLICA_SUBSCRIBER_ADDED);

    OMPI_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);
    /******     UNLOCK     ******/

 RETURN_ERROR:
    if (NULL != segment) {
        free(segment);
    }

    if (NULL != tokens) {
        for (i=0; i < num_tokens; i++) {
            free(tokens[i]);
        }
        free(tokens);
    }

    if (NULL != keys) {
        for (i=0; i < num_keys; i++) {
            free(keys[i]);
        }
        free(keys);
    }

    if (NULL != key_itags) {
      free(key_itags);
    }

    if (NULL != token_itags) {
      free(token_itags);
    }

    return rc;
}

int orte_gpr_replica_recv_unsubscribe_cmd(orte_buffer_t *input_buffer,
                                          orte_buffer_t *output_buffer)
{
    orte_gpr_cmd_flag_t command=ORTE_GPR_UNSUBSCRIBE_CMD;
    orte_gpr_notify_id_t sub_number=0;
    int rc, ret;
    size_t n;

    if (ORTE_SUCCESS != (rc = orte_dps.pack(output_buffer, &command, 1, ORTE_GPR_CMD))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    
    n = 1;
    if (ORTE_SUCCESS != (rc = orte_dps.unpack(input_buffer, &sub_number, &n, ORTE_GPR_NOTIFY_ID))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }

    /*******   LOCK    *****/
    OMPI_THREAD_LOCK(&orte_gpr_replica_globals.mutex);

    ret = orte_gpr_replica_unsubscribe_fn(sub_number);
    if (ORTE_SUCCESS != ret) {
        ORTE_ERROR_LOG(ret);
    }

    OMPI_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);
    /******     UNLOCK     ******/

    if (ORTE_SUCCESS != (rc = orte_dps.pack(output_buffer, &ret, 1, ORTE_INT))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    
    return ret;
}


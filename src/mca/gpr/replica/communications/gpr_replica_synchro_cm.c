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

#include "gpr_replica_comm.h"

int orte_gpr_replica_recv_synchro_cmd(orte_process_name_t* sender,
                                      orte_buffer_t *buffer,
                                      orte_buffer_t *output_buffer)
{
    orte_gpr_cmd_flag_t command=ORTE_GPR_SYNCHRO_CMD;
    orte_gpr_addr_mode_t addr_mode;
    orte_gpr_notify_id_t local_idtag=0, idtag=0;
    orte_gpr_replica_segment_t *seg=NULL;
    size_t n;
    int trigger=0, rc, ret;
    orte_gpr_replica_act_sync_t flag;
    orte_gpr_value_t *value;
    
    if (ORTE_SUCCESS != (rc = orte_dps.pack(output_buffer, &command, 1, ORTE_GPR_CMD))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    
    n = 1;
    if (ORTE_SUCCESS != (ret = orte_dps.unpack(buffer, &flag.trig_synchro, &n, ORTE_SYNCHRO_MODE))) {
        ORTE_ERROR_LOG(ret);
        goto RETURN_ERROR;
    }

    n = 1;
    if (ORTE_SUCCESS != (ret = orte_dps.unpack(buffer, &addr_mode, &n, ORTE_GPR_ADDR_MODE))) {
        ORTE_ERROR_LOG(ret);
        goto RETURN_ERROR;
    }

    n = 1;
    if (ORTE_SUCCESS != (ret = orte_dps.unpack(buffer, &value, &n, ORTE_GPR_VALUE))) {
        ORTE_ERROR_LOG(ret);
        goto RETURN_ERROR;
    }

    /* unpack the trigger info */
    n = 1;
    if (ORTE_SUCCESS != (ret = orte_dps.unpack(buffer, &trigger, &n, ORTE_INT))) {
        ORTE_ERROR_LOG(ret);
        goto RETURN_ERROR;
    }

    /* unpack the remote trigger id */
    n = 1;
    if (ORTE_SUCCESS != (ret = orte_dps.unpack(buffer, &idtag, &n, ORTE_GPR_NOTIFY_ID))) {
        ORTE_ERROR_LOG(ret);
        goto RETURN_ERROR;
    }

    /*******   LOCK    *****/
    OMPI_THREAD_LOCK(&orte_gpr_replica_globals.mutex);

    /* locate the segment */
    if (ORTE_SUCCESS != (ret = orte_gpr_replica_find_seg(&seg, true, value->segment))) {
        ORTE_ERROR_LOG(ret);
        OMPI_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);
        goto RETURN_ERROR;
    }

    
    if (NULL != sender) {  /* remote sender */

        /* enter request on notify tracking system */
        if (ORTE_SUCCESS != (ret = orte_gpr_replica_enter_notify_request(&local_idtag,
                                        seg, ORTE_GPR_SYNCHRO_CMD, &flag,
                                        sender, idtag, NULL, NULL))) {
            ORTE_ERROR_LOG(ret);
            OMPI_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);
            goto RETURN_ERROR;
        }

        /* process synchro request */
        if (ORTE_SUCCESS != (ret = orte_gpr_replica_synchro_fn(addr_mode,
                                    seg, value, trigger, local_idtag))) {
            ORTE_ERROR_LOG(ret);
            orte_gpr_replica_remove_notify_request(local_idtag, &idtag);
            OMPI_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);
            goto RETURN_ERROR;
        }

        /* pack the local idtag for return to sender */
        if (ORTE_SUCCESS != (ret = orte_dps.pack(output_buffer, &local_idtag, 1, ORTE_GPR_NOTIFY_ID))) {
            ORTE_ERROR_LOG(ret);
            OMPI_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);
            goto RETURN_ERROR;
        }

    } else {  /* local request - idtag is for local notify tracking system */

        /* process synchro request */
        if (ORTE_SUCCESS != (ret = orte_gpr_replica_synchro_fn(addr_mode,
                                    seg, value, trigger, idtag))) {
            ORTE_ERROR_LOG(ret);
            orte_gpr_replica_remove_notify_request(idtag, &idtag);
            OMPI_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);
            goto RETURN_ERROR;
        }
    }

    orte_gpr_replica_check_synchros(seg);

    OMPI_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);
    /******     UNLOCK     ******/

 RETURN_ERROR:
    if (ORTE_SUCCESS != (rc = orte_dps.pack(output_buffer, &ret, 1, ORTE_INT))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    
    return ret;
}

int orte_gpr_replica_recv_cancel_synchro_cmd(orte_buffer_t *buffer,
                                             orte_buffer_t *answer)
{
    orte_gpr_cmd_flag_t command=ORTE_GPR_CANCEL_SYNCHRO_CMD;
    orte_gpr_notify_id_t synch_number=0;
    size_t n;
    int rc, ret;

    if (ORTE_SUCCESS != (rc = orte_dps.pack(answer, &command, 1, ORTE_GPR_CMD))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    
    n = 1;
    if (ORTE_SUCCESS != (rc = orte_dps.unpack(buffer, &synch_number, &n, ORTE_GPR_NOTIFY_ID))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }

    /*******   LOCK    *****/
    OMPI_THREAD_LOCK(&orte_gpr_replica_globals.mutex);

    ret = orte_gpr_replica_cancel_synchro_fn(synch_number);

    OMPI_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);
    /******     UNLOCK     ******/

    if (ORTE_SUCCESS != ret)
        ORTE_ERROR_LOG(ret);
        
    if (ORTE_SUCCESS != (rc = orte_dps.pack(answer, &ret, 1, ORTE_INT))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    
    return ret;
}


/*
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

#include "opal/util/output.h"
#include "util/proc_info.h"
#include "mca/errmgr/errmgr.h"

#include "gpr_replica_api.h"

int orte_gpr_replica_delete_segment(char *segment)
{
    orte_gpr_replica_segment_t *seg=NULL;
    int rc;

    /* protect against errors */
    if (NULL == segment) {
	   return ORTE_ERROR;
    }

    OPAL_THREAD_LOCK(&orte_gpr_replica_globals.mutex);

    /* locate the segment */
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_find_seg(&seg, false, segment))) {
        OPAL_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);
	    return rc;
    }

    rc = orte_gpr_replica_release_segment(&seg);

    OPAL_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);
    return rc;
}


int orte_gpr_replica_delete_segment_nb(char *segment,
                                orte_gpr_notify_cb_fn_t cbfunc, void *user_tag)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}



int orte_gpr_replica_delete_entries(orte_gpr_addr_mode_t addr_mode,
			      char *segment, char **tokens, char **keys)
{
    int rc;
    orte_gpr_replica_segment_t *seg=NULL;
    orte_gpr_replica_itag_t *token_itags=NULL, *key_itags=NULL;
    size_t num_tokens, num_keys;

    /* protect against errors */
    if (NULL == segment) {
	   return ORTE_ERROR;
    }

    OPAL_THREAD_LOCK(&orte_gpr_replica_globals.mutex);

    /* locate the segment */
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_find_seg(&seg, false, segment))) {
        OPAL_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);
        return rc;
    }

    if (ORTE_SUCCESS != (rc = orte_gpr_replica_get_itag_list(&token_itags, seg, tokens, &num_tokens))) {
        OPAL_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);
        return rc;
    }

    if (ORTE_SUCCESS != (rc = orte_gpr_replica_get_itag_list(&key_itags, seg, keys, &num_keys))) {
        OPAL_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);
        return rc;
    }

    rc = orte_gpr_replica_delete_entries_fn(addr_mode, seg,
                                            token_itags, num_tokens,
                                            key_itags, num_keys);

    if (ORTE_SUCCESS == rc) {
        if (ORTE_SUCCESS != (rc = orte_gpr_replica_check_events())) {
            ORTE_ERROR_LOG(rc);
        }
    }
    
    if (NULL != token_itags) {
        free(token_itags);
    }

    if (NULL != key_itags) {
        free(key_itags);
    }

    if (ORTE_SUCCESS == rc) {
        if (ORTE_SUCCESS != (rc = orte_gpr_replica_process_callbacks())) {
            ORTE_ERROR_LOG(rc);
        }
    }
    
    OPAL_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);

    return rc;
}


int orte_gpr_replica_delete_entries_nb(
                            orte_gpr_addr_mode_t addr_mode,
                            char *segment, char **tokens, char **keys,
                            orte_gpr_notify_cb_fn_t cbfunc, void *user_tag)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}


int orte_gpr_replica_index(char *segment, size_t *cnt, char ***index)
{
    orte_gpr_replica_segment_t *seg=NULL;
    int rc;

    if (NULL == index || NULL == cnt) {
        ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
        return ORTE_ERR_BAD_PARAM;
    }
    
    OPAL_THREAD_LOCK(&orte_gpr_replica_globals.mutex);

    if (NULL == segment) {  /* want global level index */
	   seg = NULL;
    } else {
        /* locate the segment */
        if (ORTE_SUCCESS != (rc = orte_gpr_replica_find_seg(&seg, false, segment))) {
            OPAL_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);
            return rc;
        }
    }

    rc = orte_gpr_replica_index_fn(seg, cnt, index);

    OPAL_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);
    return rc;
}

int orte_gpr_replica_index_nb(char *segment,
                        orte_gpr_notify_cb_fn_t cbfunc, void *user_tag)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

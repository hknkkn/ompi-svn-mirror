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
 * The Open MPI general purpose registry - support functions.
 *
 */

/*
 * includes
 */

#include "orte_config.h"

#include "util/output.h"
#include "util/argv.h"
#include "mca/errmgr/errmgr.h"
#include "mca/gpr/replica/transition_layer/gpr_replica_tl.h"

#include "gpr_replica_fn.h"


int orte_gpr_replica_create_container(orte_gpr_replica_container_t **cptr,
                                      orte_gpr_replica_segment_t *seg,
                                      int num_itags,
                                      orte_gpr_replica_itag_t *itags)
{
    int rc;
    
    *cptr = OBJ_NEW(orte_gpr_replica_container_t);
    if (NULL == *cptr) {
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    if (ORTE_SUCCESS !=
          (rc = orte_gpr_replica_copy_itag_list(&((*cptr)->itags), itags, num_itags))) {
        ORTE_ERROR_LOG(rc);
        OBJ_RELEASE(*cptr);
        return rc;
    }
    
    (*cptr)->num_itags = num_itags;
    
    if (0 > ((*cptr)->index = orte_pointer_array_add(seg->containers, (void*)(*cptr)))) {
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    return ORTE_SUCCESS;
}


int orte_gpr_replica_release_container(orte_gpr_replica_segment_t *seg,
                                       orte_gpr_replica_container_t *cptr)
{
    orte_gpr_replica_triggers_t **trig;
    orte_gpr_replica_target_t **targets;
    orte_gpr_replica_itagval_t **iptr;
    int i, j, k, rc;
    
    /* clear any triggers attached to it, adjusting synchros and
     * registering callbacks as required
     */
    iptr = (orte_gpr_replica_itagval_t**)((cptr->itagvals)->addr);
    trig = (orte_gpr_replica_triggers_t**)((orte_gpr_replica.triggers)->addr);

    for (i=0; i < (orte_gpr_replica.triggers)->size; i++) {
        if (NULL != trig[i] && seg == trig[i]->seg) { /* valid trig on this segment */
            targets = (orte_gpr_replica_target_t**)((trig[i]->targets)->addr);
            for (j=0; j < (trig[i]->targets)->size; j++) {
                if (NULL != targets[j]) {
                    if (cptr == targets[j]->cptr) {
                        if (ORTE_SUCCESS != (rc = orte_gpr_replica_check_trigger(seg, trig[i], cptr, targets[j]->iptr,
                                            ORTE_GPR_REPLICA_ENTRY_DELETED))) {
                            ORTE_ERROR_LOG(rc);
                            return rc;
                        }
                    }
                    orte_pointer_array_set_item(trig[i]->targets, j, NULL);
                    free(targets[j]);
                    break;
                }
            }
        }
    }
    
    /* remove container from segment and release it */
    orte_pointer_array_set_item(seg->containers, cptr->index, NULL);
    OBJ_RELEASE(cptr);
    
    return ORTE_SUCCESS;
}


int orte_gpr_replica_add_keyval(orte_gpr_replica_segment_t *seg,
                                orte_gpr_replica_container_t *cptr,
                                orte_gpr_keyval_t **kptr)
{
    orte_gpr_replica_itagval_t *iptr;
    int rc;
    
    iptr = OBJ_NEW(orte_gpr_replica_itagval_t);
    if (NULL == iptr) {
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_create_itag(&(iptr->itag),
                                            seg, (*kptr)->key))) {
        OBJ_RELEASE(iptr);
        return rc;
    }
    
    iptr->type = (*kptr)->type;
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_xfer_payload(&(iptr->value),
                                               &((*kptr)->value), (*kptr)->type))) {
        OBJ_RELEASE(iptr);
        return rc;
    }
    
    if (0 > (iptr->index = orte_pointer_array_add(cptr->itagvals, (void*)iptr))) {
        OBJ_RELEASE(iptr);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }

    return ORTE_SUCCESS;
}


int orte_gpr_replica_update_keyval(orte_gpr_replica_segment_t *seg,
                                   orte_gpr_replica_container_t *cptr,
                                   orte_gpr_keyval_t **kptr)
{
    int i;
    orte_pointer_array_t *ptr;
    orte_gpr_replica_itagval_t *iptr;
    
    /* for each item in the search array, delete it */
    ptr = orte_gpr_replica_globals.search;
    
    for (i = 0; i < ptr->size; i++) {
        if (NULL != ptr->addr[i]) {
            iptr = (orte_gpr_replica_itagval_t*)ptr->addr[i];
            orte_pointer_array_set_item(cptr->itagvals, iptr->index, NULL);
            OBJ_RELEASE(iptr);
        }
    }
    
    /* now add new item in their place */
   return orte_gpr_replica_add_keyval(seg, cptr, kptr);
}


bool orte_gpr_replica_search_container(int *num_found,
                                       orte_gpr_replica_itag_t itag,
                                       orte_gpr_replica_container_t *cptr)
{
    orte_gpr_replica_itagval_t **ptr;
    int i, cnt;
    
    /* ensure the search array is clear */
    orte_pointer_array_clear(orte_gpr_replica_globals.search);
    *num_found = 0;
    cnt = 0;
    
    ptr = (orte_gpr_replica_itagval_t**)((cptr->itagvals)->addr);
    for (i=0; i < (cptr->itagvals)->size; i++) {
        if (NULL != ptr[i] && itag == ptr[i]->itag) { /* found it! */
            if (0 > orte_pointer_array_add(orte_gpr_replica_globals.search, ptr[i])) {
                orte_pointer_array_clear(orte_gpr_replica_globals.search);
                return false;
            }
            cnt++;
        }
    }
    
    *num_found = cnt;
    
    if (0 < cnt) {
        return true;
    }
    
    /* didn't find anything, so return false */
    return false;
}


int orte_gpr_replica_xfer_payload(orte_gpr_value_union_t *dest,
                                  orte_gpr_value_union_t *src,
                                  orte_data_type_t type)
{
    
    switch(type) {

        case ORTE_STRING:
            dest->strptr = strdup(src->strptr);
            break;
            
        case ORTE_UINT8:
            dest->ui8 = src->ui8;
            break;
            
        case ORTE_UINT16:
            dest->ui16 = src->ui16;
            break;
            
        case ORTE_UINT32:
            dest->ui32 = src->ui32;
            break;
            
#ifdef HAVE_I64
        case ORTE_UINT64:
            dest->ui64 = src->ui64;
            break;
#endif

        case ORTE_INT8:
            dest->i8 = src->i8;
            break;
        
        case ORTE_INT16:
            dest->i16 = src->i16;
            break;
        
        case ORTE_INT32:
            dest->i32 = src->i32;
            break;
        
#ifdef HAVE_I64
        case ORTE_INT64:
            dest->i64 = src->i64;
            break;
#endif

        case ORTE_NAME:
            dest->proc = src->proc;;
            break;
            
        case ORTE_JOBID:
            dest->jobid = src->jobid;
            break;
            
        case ORTE_CELLID:
            dest->cellid = src->cellid;
            break;
            
        case ORTE_VPID:
            dest->vpid = src->vpid;
            break;
            
        case ORTE_NODE_STATE:
            dest->node_state = src->node_state;
            break;
            
        case ORTE_PROC_STATE:
            dest->proc_state = src->proc_state;
            break;
            
        case ORTE_EXIT_CODE:
            dest->exit_code = src->exit_code;
            break;
            
        case ORTE_BYTE_OBJECT:
            (dest->byteobject).size = (src->byteobject).size;
            (dest->byteobject).bytes = (src->byteobject).bytes;
            (src->byteobject).bytes = NULL;
            break;

        case ORTE_APP_CONTEXT:
            if(NULL == src->app_context) {
                dest->app_context = NULL;
                break;
            }
            dest->app_context = OBJ_NEW(orte_app_context_t);
            dest->app_context->idx = src->app_context->idx;
            if(NULL != src->app_context->app) {
                dest->app_context->app = strdup(src->app_context->app);
            }
            dest->app_context->num_procs = src->app_context->num_procs;
            dest->app_context->argc = src->app_context->argc;
            dest->app_context->argv = ompi_argv_copy(src->app_context->argv);
            dest->app_context->num_env = src->app_context->num_env;
            dest->app_context->env = ompi_argv_copy(src->app_context->env);
            if(NULL != src->app_context->cwd) {
                dest->app_context->cwd = strdup(src->app_context->cwd);
            }
            break;

        case ORTE_NULL:
            break;
            
        default:
            return ORTE_ERR_BAD_PARAM;
            break;
    }
    return ORTE_SUCCESS;
}


int orte_gpr_replica_release_segment(orte_gpr_replica_segment_t **seg)
{
    int rc;
    
    if (0 > (rc = orte_pointer_array_set_item(orte_gpr_replica.segments, (*seg)->itag, NULL))) {
        return rc;
    }
    OBJ_RELEASE(*seg);
    
    return ORTE_SUCCESS;
}

int orte_gpr_replica_purge_itag(orte_gpr_replica_segment_t *seg,
                                orte_gpr_replica_itag_t itag)
{
     /*
     * Begin by looping through the segment's containers and check
     * their descriptions first - if removing this name leaves that
     * list empty, then remove the container.
     * If the container isn't to be removed, then loop through all
     * the container's keyvalue pairs and check the "key" - if
     * it matches, then remove that pair. If all pairs are removed,
     * then remove the container
     * */

    return ORTE_SUCCESS;
}

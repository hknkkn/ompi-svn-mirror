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

#include "mca/gpr/replica/transition_layer/gpr_replica_tl.h"

#include "gpr_replica_fn.h"


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
    
    if (0 > orte_pointer_array_add(cptr->itagvals, (void*)iptr)) {
        OBJ_RELEASE(iptr);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    free(*kptr);
    *kptr = NULL;
    return ORTE_SUCCESS;
}


int orte_gpr_replica_update_keyval(orte_gpr_replica_segment_t *seg,
                                   orte_gpr_replica_itagval_t *iptr,
                                   orte_gpr_keyval_t *kptr)
{
    int rc;
    orte_gpr_replica_itag_t itag;
    
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_create_itag(&itag,
                                            seg, kptr->key))) {
        return rc;
    }
    
    iptr->itag = itag;
    iptr->type = kptr->type;
    
    return orte_gpr_replica_xfer_payload(&(iptr->value), &(kptr->value), iptr->type);
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

        case ORTE_BYTE_OBJECT:
            (dest->byteobject).size = (src->byteobject).size;
            (dest->byteobject).bytes = (src->byteobject).bytes;
            (src->byteobject).bytes = NULL;
            break;
            
        case ORTE_NAME:
            dest->proc = src->proc;;
            break;
            
        case ORTE_JOBID:
            dest->jobid = src->jobid;
            break;
            
        case ORTE_NODE_STATE:
            dest->node_state = src->node_state;
            break;
            
        case ORTE_STATUS_KEY:
            dest->proc_status = src->proc_status;
            break;
            
        case ORTE_EXIT_CODE:
            dest->exit_code = src->exit_code;
            break;
            
        default:
            return ORTE_ERR_BAD_PARAM;
            break;
    }
    return ORTE_SUCCESS;
}


bool orte_gpr_replica_search_container(orte_gpr_replica_itagval_t **iptr,
                                       orte_gpr_replica_segment_t *seg,
                                       orte_gpr_replica_container_t *cptr,
                                       orte_gpr_keyval_t *kptr)
{
    orte_gpr_replica_itagval_t **ptr;
    orte_gpr_replica_itag_t itag;
    int i;
    
    if (ORTE_SUCCESS != orte_gpr_replica_dict_lookup(&itag, seg, kptr->key)) {
        /* if the key isn't in the dictionary, then the keyval can't
         * possibly be in the container
         */
        return false;
    }
    
    ptr = (orte_gpr_replica_itagval_t**)((cptr->itagvals)->addr);
    for (i=0; i < (cptr->itagvals)->size; i++) {
        if (NULL != ptr[i] && itag == ptr[i]->itag) { /* found it! */
            *iptr = ptr[i];  /* send back the ptr to the itagval */
            return true;
        }
    }
    
    /* didn't find it, so return false */
    return false;
}


int orte_gpr_replica_release_segment(orte_gpr_replica_segment_t *seg)
{
    int rc;
    
    if (0 > (rc = orte_pointer_array_set_item(orte_gpr_replica.segments, seg->itag, NULL))) {
        return rc;
    }
    OBJ_RELEASE(seg);
    
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

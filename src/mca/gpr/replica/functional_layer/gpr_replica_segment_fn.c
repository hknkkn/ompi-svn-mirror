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
    
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_xfer_payload(iptr, *kptr))) {
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


int orte_gpr_replica_update_keyval(orte_gpr_replica_itagval_t *iptr,
                                   orte_gpr_keyval_t *kptr)
{
    return orte_gpr_replica_xfer_payload(iptr, kptr);
}


int orte_gpr_replica_xfer_payload(orte_gpr_replica_itagval_t *iptr,
                                  orte_gpr_keyval_t *kptr)
{
    iptr->type = kptr->type;
    
    switch(iptr->type) {

        case ORTE_STRING:
            (iptr->value).strptr = strdup((kptr->value).strptr);
            break;
            
        case ORTE_UINT8:
            (iptr->value).ui8 = (kptr->value).ui8;
            break;
            
        case ORTE_UINT16:
            (iptr->value).ui16 = (kptr->value).ui16;
            break;
            
        case ORTE_UINT32:
            (iptr->value).ui32 = (kptr->value).ui32;
            break;
            
#ifdef HAVE_I64
        case ORTE_UINT64:
            (iptr->value).ui64 = (kptr->value).ui64;
            break;
#endif

        case ORTE_INT8:
            (iptr->value).i8 = (kptr->value).i8;
            break;
        
        case ORTE_INT16:
            (iptr->value).i16 = (kptr->value).i16;
            break;
        
        case ORTE_INT32:
            (iptr->value).i32 = (kptr->value).i32;
            break;
        
#ifdef HAVE_I64
        case ORTE_INT64:
            (iptr->value).i64 = (kptr->value).i64;
            break;
#endif

        case ORTE_BYTE_OBJECT:
            ((iptr->value).byteobject).size = ((kptr->value).byteobject).size;
            ((iptr->value).byteobject).bytes = ((kptr->value).byteobject).bytes;
            ((kptr->value).byteobject).bytes = NULL;
            break;
            
        case ORTE_NAME:
            (iptr->value).proc = (kptr->value).proc;;
            break;
            
        case ORTE_JOBID:
            (iptr->value).jobid = (kptr->value).jobid;
            break;
            
        case ORTE_NODE_STATE:
            (iptr->value).node_state = (kptr->value).node_state;
            break;
            
        case ORTE_STATUS_KEY:
            (iptr->value).proc_status = (kptr->value).proc_status;
            break;
            
        case ORTE_EXIT_CODE:
            (iptr->value).exit_code = (kptr->value).exit_code;
            break;
            
        default:
            return ORTE_ERR_BAD_PARAM;
            break;
    }
}


bool orte_gpr_replica_search_container(orte_gpr_replica_itagval_t **iptr,
                                       orte_gpr_replica_container_t *cptr,
                                       orte_gpr_keyval_t *kptr)
{
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

    return ORTE_ERR_NOT_IMPLEMENTED;
}

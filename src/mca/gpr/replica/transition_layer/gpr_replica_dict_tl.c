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

#include "class/orte_pointer_array.h"
#include "util/output.h"

#include "mca/gpr/replica/functional_layer/gpr_replica_fn.h"

#include "gpr_replica_tl.h"

int
orte_gpr_replica_create_itag(orte_gpr_replica_itag_t *itag,
                             orte_gpr_replica_segment_t *seg, char *name)
{
    orte_gpr_replica_dict_t **ptr, *new;
    int i;

    /* default to illegal value */
    *itag = ORTE_GPR_REPLICA_ITAG_MAX;
    
    /* if name or seg is NULL, error */
    if (NULL == name || NULL == seg) {
        return ORTE_ERR_BAD_PARAM;
    }

    /* check seg's dictionary to ensure uniqueness */
    ptr = (orte_gpr_replica_dict_t**)(seg->dict)->addr;
    for (i=0; i < (seg->dict)->size; i++) {
        if (NULL != ptr[i]) {
            if (0 == strncmp(ptr[i]->entry, name, strlen(ptr[i]->entry))) { /* already present */
                *itag = ptr[i]->itag;
                return ORTE_SUCCESS;
            }
        }
    }

    /* okay, name is unique - create dictionary entry */
    new = (orte_gpr_replica_dict_t*)malloc(sizeof(orte_gpr_replica_dict_t));
    if (NULL == new) {
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    new->entry = strdup(name);
    if (0 > (i = orte_pointer_array_add(seg->dict, (void*)new))) {
        *itag = ORTE_GPR_REPLICA_ITAG_MAX;
        free(new);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    *itag = (orte_gpr_replica_itag_t)i;
    new->itag = *itag;
    return ORTE_SUCCESS;
}


int orte_gpr_replica_delete_itag(orte_gpr_replica_segment_t *seg, char *name)
{
    orte_gpr_replica_itag_t itag;
    int rc;

    /* check for errors */
    if (NULL == name || NULL == seg) {
        return ORTE_ERR_BAD_PARAM;
    }

    /* find dictionary element to delete */
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_dict_lookup(&itag, seg, name))) {
        return rc;
    }

    /* found name in dictionary */
    /* need to search this segment's registry to find all instances
     * that name & delete them
     */
     if (ORTE_SUCCESS != (rc = orte_gpr_replica_purge_itag(seg, itag))) {
        return rc;
     }

     /* remove itag from segment dictionary */
     return orte_pointer_array_set_item(seg->dict, itag, NULL);
}


int
orte_gpr_replica_dict_lookup(orte_gpr_replica_itag_t *itag,
                             orte_gpr_replica_segment_t *seg, char *name)
{
    orte_gpr_replica_dict_t **ptr;
    int i;
    
    /* initialize to illegal value */
    *itag = ORTE_GPR_REPLICA_ITAG_MAX;
    
    /* protect against error */
    if (NULL == seg) {
        return ORTE_ERR_BAD_PARAM;
    }
    
    if (NULL == name) { /* just want segment token-itag pair */
        *itag = seg->itag;
        return ORTE_SUCCESS;
	}

    /* want specified token-itag pair in that segment's dictionary */
    ptr = (orte_gpr_replica_dict_t**)((seg->dict)->addr);
    for (i=0; i < (seg->dict)->size; i++) {
        if (NULL != ptr[i]) {
    	       if (0 == strncmp(ptr[i]->entry, name, strlen(ptr[i]->entry))) {
                *itag = ptr[i]->itag;
                return ORTE_SUCCESS;
            }
        }
	}

    return ORTE_ERR_NOT_FOUND; /* couldn't find the specified entry */
}


int orte_gpr_replica_dict_reverse_lookup(char **name,
        orte_gpr_replica_segment_t *seg, orte_gpr_replica_itag_t itag)
{
    orte_gpr_replica_dict_t **ptr;
    orte_gpr_replica_segment_t **segptr;

    /* initialize to nothing */
    *name = NULL;
    
    if (NULL == seg) {
	   /* return the segment name
        * note that itag is the index of the segment in that array
        */
        segptr = (orte_gpr_replica_segment_t**)(orte_gpr_replica.segments->addr);
        if (NULL == segptr[itag]) { /* this segment is no longer alive */
            return ORTE_ERR_NOT_FOUND;
        }
	   *name = strdup(segptr[itag]->name);
	   return ORTE_SUCCESS;
    }

    /* seg is provided - find the matching token for this itag
     * note again that itag is the index into this segment's
     * dictionary array
     */
    ptr = (orte_gpr_replica_dict_t**)((seg->dict)->addr);
    if (NULL == ptr[itag]) {  /* dict element no longer valid */
        return ORTE_ERR_NOT_FOUND;
    }
    *name = strdup(ptr[itag]->entry);
    return ORTE_SUCCESS;

}

int
orte_gpr_replica_get_itag_list(orte_gpr_replica_itag_t **itaglist,
                    orte_gpr_replica_segment_t *seg, char **names,
                    int *num_names)
{
    char **namptr;
    int num_itags, rc, i;

    *num_names = 0;
    *itaglist = NULL;

    /* check for errors */
    if (NULL == seg) {
        return ORTE_ERR_BAD_PARAM;
    }
    
    /* check for wild-card case */
    if (NULL == names) {
	   return ORTE_SUCCESS;
    }

    /* count the number of names */
    namptr = names;
    num_itags = 0;
    while (NULL != *namptr) {
	   num_itags++;
	   namptr++;
    }

    *itaglist = (orte_gpr_replica_itag_t*)malloc(num_itags*sizeof(orte_gpr_replica_itag_t));
    if (NULL == *itaglist) {
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    *num_names = num_itags;

    namptr = names;

    i = 0;
    while (NULL != *namptr) {  /* traverse array of names until NULL */
        if (ORTE_SUCCESS != (rc = orte_gpr_replica_create_itag(&((*itaglist)[i]), seg, *namptr))) {
            return rc;
        }
	    namptr++; i++;
    }
    return ORTE_SUCCESS;
}


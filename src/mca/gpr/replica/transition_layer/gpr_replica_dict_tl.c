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

#include "mca/gpr/replica/functional_layer/gpr_replica_fn.h"

#include "gpr_replica_tl.h"

int
orte_gpr_replica_create_itag(orte_gpr_replica_itag_t *itag,
                             orte_gpr_replica_segment_t *seg, char *name)
{
    orte_gpr_replica_dict_t *ptr;
    int i;

    /* default to illegal value */
    *itag = ORTE_GPR_REPLICA_ITAG_MAX;
    
    /* if name or seg is NULL, error */
    if (NULL == name || NULL == seg) {
        return ORTE_ERR_BAD_PARAM;
    }

    /* check seg's dictionary to ensure uniqueness */
    ptr = (orte_gpr_replica_dict_t*)(seg->dict)->addr;
    for (i=0; i < (seg->dict)->size; i++) {
        if (NULL != ptr) {
            if (0 == strncmp(ptr->entry, name, strlen(ptr->entry))) { /* already present */
                *itag = ptr->itag;
                return ORTE_SUCCESS;
            }
        }
        ptr++;
    }

    /* okay, name is unique - create dictionary entry */
    ptr = (orte_gpr_replica_dict_t*)malloc(sizeof(orte_gpr_replica_dict_t));
    ptr->entry = strdup(name);
    if (0 < (i = orte_pointer_array_add(seg->dict, (void*)ptr))) {
        *itag = ORTE_GPR_REPLICA_ITAG_MAX;
        free(ptr);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    *itag = (orte_gpr_replica_itag_t)i;
    ptr->itag = *itag;
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
    orte_gpr_replica_dict_t *ptr;
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
    ptr = (orte_gpr_replica_dict_t*)((seg->dict)->addr);
    for (i=0; i < (seg->dict)->size; i++) {
        if (NULL != ptr) {
    	       if (0 == strncmp(ptr->entry, name, strlen(ptr->entry))) {
                *itag = ptr->itag;
                return ORTE_SUCCESS;
            }
        }
        ptr++;
	}

    return ORTE_ERR_BAD_PARAM; /* couldn't find the specified entry */
}


int orte_gpr_replica_dict_reverse_lookup(char **name,
        orte_gpr_replica_segment_t *seg, orte_gpr_replica_itag_t itag)
{
    orte_gpr_replica_dict_t *ptr;
    orte_gpr_replica_segment_t *segptr;
    int i;

    /* initialize to nothing */
    *name = NULL;
    
    if (NULL == seg) {
	   /* want to find a matching token for a segment name */
       segptr = (orte_gpr_replica_segment_t*)(orte_gpr_replica.segments->addr);
	   for (i=0; i < orte_gpr_replica.segments->size; i++) {
	       if (itag == segptr->itag) {
		   *name = strdup(segptr->name);
		   return ORTE_SUCCESS;
	       }
	   }
	   return ORTE_ERR_BAD_PARAM;  /* couldn't find the specified entry */
    }

    /* seg is provides - find the matching itag */
    ptr = (orte_gpr_replica_dict_t*)((seg->dict)->addr);
    for (i=0; i < (seg->dict)->size; i++) {
        if (itag == ptr->itag) {
            *name = strdup(ptr->entry);
            return ORTE_SUCCESS;
        }
        ptr++;
    }

    return ORTE_ERR_BAD_PARAM; /* couldn't find the specified entry */
}

int
orte_gpr_replica_get_itag_list(orte_gpr_replica_itag_t **itaglist,
                    orte_gpr_replica_segment_t *seg, char **names,
                    int *num_names)
{
    char **namptr;
    orte_gpr_replica_itag_t *itagptr;
    int num_itags, rc;

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
    itagptr = *itaglist;
    *num_names = num_itags;

    namptr = names;

    while (NULL != *namptr) {  /* traverse array of names until NULL */
        if (ORTE_SUCCESS != (rc = orte_gpr_replica_create_itag(itagptr, seg, *namptr))) {
            return rc;
        }
	    namptr++; itagptr++;
    }
    return ORTE_SUCCESS;
}


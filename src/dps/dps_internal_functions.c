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
 */
#include "ompi_config.h"

#include <sys/types.h>
#include <math.h>
#include <unistd.h>

#include "mca/ns/base/base.h"

#include "dps_internal.h"

/**
 * Internal-use only functions
 */
 
/**
 * Calculate the memory storage required for the requested operation
 */
size_t orte_dps_memory_required(bool packed, void *src, orte_pack_type_t type)
{
    orte_byte_object_t *ptr;
    char *strptr;
    uint8_t *d8;
    
    switch(type) {
        case ORTE_BYTE_OBJECT:
            if (NULL == src) {
                return 0;
            }
            ptr = (orte_byte_object_t *) src;
            return ptr->size;
            break;
            
        case ORTE_INT8:
            return 1;
            break;
            
        case ORTE_NODE_STATE:
            return sizeof(orte_node_state_t);
            break;
            
        case ORTE_PROCESS_STATUS:
            return sizeof(orte_process_status_t);
            break;
            
        case ORTE_EXIT_CODE:
            return sizeof(orte_exit_code_t);
            break;
            
        case ORTE_INT16:
            return 2;
            break;
            
        case ORTE_INT32:
            return 4;
            break;
            
        case ORTE_JOBID:
            return sizeof(orte_jobid_t);
            break;
            
        case ORTE_CELLID:
            return sizeof(orte_cellid_t);
            break;
            
        case ORTE_NAME:
            return sizeof(orte_process_name_t);
            break;
            
        case ORTE_STRING:
            if (packed) {  /* packed string */
                /* first byte is the length */
                d8 = (uint8_t *) src;
                return ((*d8) + 1);
            } else {  /* unpacked string */
                strptr = (char *) src;
                return (strlen(strptr) + 1);  /* reserve a spot for the string length */
            }
            break;
            
        default:
            return 0;  /* unrecognized type */
    }

}


/**
 * Internal function that resizes (expands) an inuse buffer...adds
 * requested memory in units of memory pages to the current buffer.
 */
int orte_dps_buffer_extend(orte_buffer_t *bptr, size_t mem_req)
{
    /* no buffer checking, we should know what we are doing in here */
    
    size_t newsize; 
    int pages;
    void*  newbaseptr;
    int num_pages;
    size_t mdiff;      /* difference in memory */
    size_t  sdiff;          /* difference (increase) in space */
    
    /* how many pages are required */
    num_pages = (int)ceil(((double)mem_req/(double)getpagesize()));

    /* push up page count */
    pages = bptr->pages + num_pages;
    
    newsize = (size_t)(pages*getpagesize());
    
    sdiff = newsize - bptr->size; /* actual increase in space */
    /* have to use relative change as no absolute without */
    /* doing pointer maths for some counts such as space */
    
    newbaseptr = realloc (bptr->base_ptr, newsize);
    
    if (!newbaseptr) { return (ORTE_ERR_OUT_OF_RESOURCE); }
    
    /* ok, we have new memory */
    
    /* update all the pointers in the buffer DT */
    /* first calc change in memory location */
    mdiff = ((char*)newbaseptr) - ((char*)bptr->base_ptr);
    
    bptr->base_ptr = newbaseptr;
    bptr->data_ptr = ((char*)bptr->data_ptr) + mdiff;
    bptr->from_ptr = ((char*)bptr->from_ptr) + mdiff;
    
    /* now update all pointers & counters */
    bptr->size = newsize;
    bptr->space += sdiff;
    bptr->pages = pages;
    
    return (ORTE_SUCCESS);
}

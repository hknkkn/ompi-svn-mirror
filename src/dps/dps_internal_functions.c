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

#include "mca/ns/ns_types.h"

#include "dps_internal.h"

/**
 * Internal-use only functions
 */
 
/**
 * Calculate the memory storage required for the requested operation
 */
size_t orte_dps_memory_required(void *src, size_t num_vals, orte_data_type_t type)
{
    char *strptr=NULL;
    size_t i=0, mem_req=0;
    orte_byte_object_t *sbyteptr=NULL;
    
    switch(type) {

        case ORTE_BYTE:
            return num_vals;

        case ORTE_BYTE_OBJECT:
            mem_req = 0;
            sbyteptr = (orte_byte_object_t *) src;
            for (i=0; i<num_vals; i++) {
                mem_req += sbyteptr->size + sizeof(sbyteptr->size);
            }
            return mem_req;
            
        case ORTE_INT8:
        case ORTE_UINT8:
            return num_vals;
            
        case ORTE_INT16:
        case ORTE_UINT16:
            return (size_t)(num_vals * 2);
            
        case ORTE_INT32:
        case ORTE_UINT32:
            return (size_t)(num_vals * 4);
            
        case ORTE_INT64:
        case ORTE_UINT64:
            return (size_t)(num_vals * 8);
            
        case ORTE_NAME:
            return (size_t)(num_vals * sizeof(orte_process_name_t));
            
        case ORTE_NULL:
        case ORTE_STRING:
            mem_req = 0;
            strptr = (char *) src;
            for (i=0; i<num_vals; i++) {
                mem_req += strlen(strptr);
                strptr++;
            }
            return mem_req;
            
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
    float frac_pages;
    ssize_t mdiff;
    size_t  sdiff;          /* difference (increase) in space */
    
    /* how many pages are required */
    frac_pages = (float)mem_req/(float)orte_dps_page_size;
    frac_pages = ceilf(frac_pages);
    num_pages = (int)frac_pages;

    /* push up page count */
    pages = bptr->pages + num_pages;
    
    newsize = (size_t)(pages*orte_dps_page_size);
    
    sdiff = newsize - bptr->size; /* actual increase in space */
    /* have to use relative change as no absolute without */
    /* doing pointer maths for some counts such as space */
    
    newbaseptr = realloc (bptr->base_ptr, newsize);
    
    if (!newbaseptr) { return (ORTE_ERR_OUT_OF_RESOURCE); }
    
    /* ok, we have new memory */
    
    /* update all the pointers in the buffer */
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

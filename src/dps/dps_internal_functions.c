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

#include <stdio.h>
#include <sys/types.h>
#include <math.h>
#include <unistd.h>

#include "util/output.h"

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

        case ORTE_BOOL:
        case ORTE_BYTE:
        case ORTE_INT8:
        case ORTE_UINT8:
            return num_vals;
            
        case ORTE_INT16:
        case ORTE_UINT16:
            return (size_t)(num_vals * sizeof(uint16_t));
            
        case ORTE_INT32:
        case ORTE_UINT32:
            return (size_t)(num_vals * sizeof(uint32_t));
            
        case ORTE_INT64:
        case ORTE_UINT64:
            return (size_t)(num_vals * sizeof(uint64_t));
            
        case ORTE_NAME:
            return (size_t)(num_vals * sizeof(orte_process_name_t));
            
        case ORTE_NULL:
        case ORTE_STRING:

            strptr = (char *) src;
            for (i=0; i<num_vals; i++) { 
                /* need to reserve sizeof(uint32_t) for length */
                mem_req += sizeof(uint32_t);
                mem_req += strlen(strptr);   /* string - null-terminator */
                strptr++;
            }
            return mem_req;
            
        case ORTE_BYTE_OBJECT:
            sbyteptr = (orte_byte_object_t *) src;
            for (i=0; i<num_vals; i++) {
                mem_req += sizeof(uint32_t);  /* length */
                mem_req += sbyteptr->size; /* bytes */
                sbyteptr++;
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

int orte_dps_dump_buffer(orte_buffer_t *buffer, int outid)
{
    void *src;
    uint32_t *s32, *d32;
    char *sstr;
    uint8_t *sptr;
    size_t num, nbytes, mem_left, i, len;
    orte_data_type_t type;
    
    src = buffer->from_ptr;
    mem_left = buffer->toend;
    
    /* output buffer's vitals */
    ompi_output(outid, "Buffer vitals:\n\tbase_ptr: %p\tdata_ptr %p\tfrom_ptr %p\n",
                        buffer->base_ptr, buffer->data_ptr, buffer->from_ptr);
    ompi_output(outid, "\tpages %d\tsize %d\tlen %d\tspace %d\ttoend %d\n\n",
                        buffer->pages, buffer->size, buffer->len,
                        buffer->space, buffer->toend);

    while (0 < mem_left) {
        /* got enough for type? */
        if (sizeof(uint32_t) > mem_left) {
            ompi_output(outid, "Not enough memory for type");
            return ORTE_ERR_UNPACK_FAILURE;
        }
        
        s32 = (uint32_t *) src;
        type = (orte_data_type_t)ntohl(*s32);
        s32++;
        src = (void *)s32;
        mem_left -= sizeof(uint32_t);
        
        /* got enough left for num_vals? */
        if (sizeof(uint32_t) > mem_left) { /* not enough memory  */
            ompi_output(outid, "Not enough memory for number of values");
            return ORTE_ERR_UNPACK_FAILURE;
        }
    
        /* unpack the number of values */
        s32 = (uint32_t *) src;
        num = (size_t)ntohl(*s32);
        s32++;
        src = (void *)s32;
        mem_left -= sizeof(uint32_t);

        ompi_output(outid, "Item: type %d number %d", (int)type, (int)num);

        switch(type) {
           
            case ORTE_BYTE:
            case ORTE_INT8:
            case ORTE_UINT8:
                mem_left -= num*sizeof(uint8_t);
                break;
                
            case ORTE_INT16:
            case ORTE_UINT16:
                mem_left -= num * sizeof(uint16_t);
                break;
                
            case ORTE_INT32:
            case ORTE_UINT32:
                mem_left -= num * sizeof(uint32_t);
                break;
            
            case ORTE_INT64:
            case ORTE_UINT64:
            case ORTE_FLOAT:
            case ORTE_FLOAT4:
            case ORTE_FLOAT8:
            case ORTE_FLOAT12:
            case ORTE_FLOAT16:
            case ORTE_DOUBLE:
            case ORTE_LONG_DOUBLE:
                ompi_output(outid, "Attempt to unpack unimplemented type");
                return ORTE_ERR_PACK_FAILURE;
                break;
    
            case ORTE_BOOL:
                mem_left -= num * sizeof(uint8_t);
                break;
    
            case ORTE_NAME:
                mem_left -= num * sizeof(orte_process_name_t);
                break;

        case ORTE_STRING:
            sstr = (char *) src;
            for(i=0; i<num; i++) {
                if(mem_left < sizeof(uint32_t)) {
                    ompi_output(outid, "Attempt to read past end of buffer");
                    return ORTE_UNPACK_READ_PAST_END_OF_BUFFER;
                }
                d32 = (uint32_t*)sstr;
                len = ntohl(*d32);
                d32++;
                sstr= (char*)d32;
                mem_left -= sizeof(uint32_t);
                if(mem_left < len) {
                    ompi_output(outid, "Attempt to read past end of buffer");
                    return ORTE_UNPACK_READ_PAST_END_OF_BUFFER;
                }
                sstr = (char*)(sstr + len);
                mem_left -= len;
            }
            break;

        case ORTE_BYTE_OBJECT:
 
            for(i=0; i<num; i++) {
                if(mem_left < sizeof(uint32_t)) {
                    ompi_output(outid, "Attempt to read past end of buffer");
                    return ORTE_UNPACK_READ_PAST_END_OF_BUFFER;
                }
                d32 = (uint32_t*)src;
                nbytes = (size_t)ntohl(*d32);
                d32++;
                sptr = (void*)d32;
                mem_left -= sizeof(uint32_t);
                if(mem_left < nbytes) {
                    ompi_output(outid, "Attempt to read past end of buffer");
                    return ORTE_UNPACK_READ_PAST_END_OF_BUFFER;
                }
                sptr = (void*)((uint8_t*)sptr + nbytes);
                mem_left -= nbytes;
            }
            break;

        case ORTE_NULL:
            break;

        default:
            ompi_output(outid, "Attempt to unpack unknown type");
            return ORTE_ERROR;
        }
        
        /* output buffer's vitals */
        ompi_output(outid, "Buffer vitals:\n\tbase_ptr: %p\tdata_ptr %p\tfrom_ptr %p\n",
                            buffer->base_ptr, buffer->data_ptr, buffer->from_ptr);
        ompi_output(outid, "\tpages %d\tsize %d\tlen %d\tspace %d\ttoend %d\n\n",
                            buffer->pages, buffer->size, buffer->len,
                            buffer->space, buffer->toend);
    }
    return ORTE_SUCCESS;
}

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
 
/*
 * DPS Buffer Operations
 */
 
/** @file:
 *
 */

#include "orte_config.h"

#include <sys/types.h>
#include <netinet/in.h>

#include "include/orte_constants.h"
#include "include/orte_types.h"
#include "mca/ns/ns_types.h"
#include "dps_internal.h"

/**
 * DPS_PACK_VALUE
 */

int orte_dps_pack(orte_buffer_t *buffer, void *src,
                  size_t num_vals,
                  orte_data_type_t type)
{
    int rc;
    void *dst;
    int32_t op_size=0;
    size_t num_bytes;
    uint32_t * d32;
    
    /* check for error */
    if (!buffer || !src || 0 >= num_vals) { return (ORTE_ERROR); }
    
    dst = buffer->data_ptr;    /* get location in buffer */

    /* calculate the required memory size for this operation */
    if (0 == (op_size = orte_dps_memory_required(src, num_vals, type))) {  /* got error */
        return ORTE_ERROR;
    }
    
    /* add in the space for the pack type */
    op_size += sizeof(uint32_t);
    
    /* add in the space to store the number of values */
    op_size += sizeof(uint32_t);

    /* check to see if current buffer has enough room */
    if (op_size > buffer->space) {  /* need to extend the buffer */
        if (ORTE_SUCCESS != (rc = orte_dps_buffer_extend(buffer, op_size))) { /* got an error */
            fprintf(stderr, "dps_pack: buffer extend failed\n");
            return rc;
        }
        dst = buffer->data_ptr;  /* need to reset the dst since it could have moved */
    }
    
    /* check for size of generic data types so they can be properly packed
     * NOTE we convert the generic data type flag to a hard type for storage
     * to handle heterogeneity
     */
    if (ORTE_INT == type || ORTE_UINT == type) {
        switch(sizeof(int)) {
            case 1:
                type = (type == ORTE_INT) ? ORTE_INT8 : ORTE_UINT8;
                break;
            case 2:
                type = (type == ORTE_INT) ? ORTE_INT16 : ORTE_UINT16;
                break;
            case 4:
                type = (type == ORTE_INT) ? ORTE_INT32 : ORTE_UINT32;
                break;
            case 8:
                type = (type == ORTE_INT) ? ORTE_INT64 : ORTE_UINT64;
                break;
            default:
                return ORTE_ERR_NOT_IMPLEMENTED;
        }
    }
    
    /* store the data type  as uint32_t */
    d32 = (uint32_t *) dst;
    *d32 = htonl(type);
    d32++;
    dst = (void *)d32;
    
    /* store the number of values as uint32_t */
    d32 = (uint32_t *) dst;
    *d32 = htonl((uint32_t)num_vals);
    d32++;
    dst = (void *)d32;

    /* pack the data */
    if (ORTE_SUCCESS != (rc = orte_dps_pack_nobuffer(dst, src, num_vals,
                                        type, &num_bytes))) {
        return rc;
    }
    
    /* ok, we managed to pack some more stuff, so update all ptrs/cnts */
    buffer->data_ptr = (void*)((char*)dst + num_bytes);
    buffer->len += op_size;
    buffer->toend += op_size;
    buffer->space -= op_size;

    return ORTE_SUCCESS;
}


int orte_dps_pack_nobuffer(void *dst, void *src, size_t num_vals,
                    orte_data_type_t type, size_t *num_bytes)
{
    size_t i, len;
    uint16_t * d16;
    uint16_t * s16;
    uint32_t * d32;
    uint32_t * s32;
    bool *bool_src;
    uint8_t *bool_dst;
    uint8_t *dbyte;
    char ** str;
    char * dstr;
    orte_process_name_t *dn, *sn;
    orte_byte_object_t *sbyteptr;

    /* initialize the number of bytes */
    *num_bytes = 0;
    
    /* pack the data */
    switch(type) {
        case ORTE_BYTE:
        case ORTE_INT8:
        case ORTE_UINT8:
            
            memcpy(dst, src, num_vals);
            *num_bytes = num_vals;
            break;
        
        case ORTE_INT16:
        case ORTE_UINT16:
            d16 = (uint16_t *) dst;
            s16 = (uint16_t *) src;
            for (i=0; i<num_vals; i++) {
                /* convert the host order to network order */
                *d16 = htons(*s16);
                d16++; s16++;
            }
            *num_bytes = num_vals * sizeof(uint16_t);
            break;
            
        case ORTE_INT32:
        case ORTE_UINT32:
            d32 = (uint32_t *) dst;
            s32 = (uint32_t *) src;
            for (i=0; i<num_vals; i++) {
                /* convert the host order to network order */
                *d32 = htonl(*s32);
                d32++; s32++;
            }
            *num_bytes = num_vals * sizeof(uint32_t);
            break;

        case ORTE_INT64:
        case ORTE_UINT64:
            return ORTE_ERR_NOT_IMPLEMENTED;
            break;
                        
        case ORTE_FLOAT:
        case ORTE_FLOAT4:
        case ORTE_FLOAT8:
        case ORTE_FLOAT12:
        case ORTE_FLOAT16:
        case ORTE_DOUBLE:
        case ORTE_LONG_DOUBLE:
            return ORTE_ERR_NOT_IMPLEMENTED;
            break;

        case ORTE_BOOL:
            bool_src = (bool *) src;
            bool_dst = (uint8_t *) dst;
            for (i=0; i<num_vals; i++) {
                /* pack native bool as uint8_t */
                *bool_dst = *bool_src ? (uint8_t)true : (uint8_t)false;
                bool_dst++; bool_src++;
            }
            *num_bytes = num_vals * sizeof(uint8_t);
            break;

        case ORTE_STRING:
            str = (char **) src;
            d32 = (uint32_t *) dst;
            for (i=0; i<num_vals; i++) {
                len = strlen(str[i]);  /* exclude the null terminator */
                *d32 = htonl(len);
                d32++;
                dstr = (char *) d32;
                memcpy(dstr, str[i], len);
                d32 = (uint32_t *)(dstr + len);
                *num_bytes += len + sizeof(uint32_t);
            }
            break;
            
        case ORTE_NAME:
            dn = (orte_process_name_t*) dst;
            sn = (orte_process_name_t*) src;
            for (i=0; i<num_vals; i++) {
                dn->cellid = htonl(sn->cellid);
                dn->jobid = htonl(sn->jobid);
                dn->vpid = htonl(sn->vpid);
                dn++; sn++;
            }
            *num_bytes = num_vals * sizeof(orte_process_name_t);
            break;
            
        case ORTE_BYTE_OBJECT:
            sbyteptr = (orte_byte_object_t *) src;
            dbyte = (uint8_t *) dst;
            for (i=0; i<num_vals; i++) {
                /* pack number of bytes */
                d32 = (uint32_t*)dbyte;
                *d32 = htonl(sbyteptr->size);
                d32++;
                dbyte = (void*)d32;
                /* pack actual bytes */
                memcpy(dbyte, sbyteptr->bytes, sbyteptr->size);
                dbyte = (uint8_t*)(dbyte + sbyteptr->size);
                sbyteptr++;
            }
            break;
            
        case ORTE_NULL:
            break;

        default:
            return ORTE_ERR_BAD_PARAM;
    }

    return ORTE_SUCCESS;
}


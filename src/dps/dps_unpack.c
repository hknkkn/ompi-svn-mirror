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

#include "mca/ns/base/base.h"

#include "dps_internal.h"

/**
 * DPS UNPACK VALUE
 */

int orte_dps_unpack(orte_buffer_t *buffer, void *dest,
                    size_t *max_num_vals,
                    orte_pack_type_t type)
{
    int rc=ORTE_SUCCESS;
    orte_pack_type_t stored_type;
    size_t pack_type_size, num_vals;
    int32_t mem_left, tst;
    void *src;
    size_t op_size=0;
    uint8_t  * s8;
    uint8_t  * d8;
    uint16_t * d16;
    uint32_t * d32;
    uint16_t * s16;
    uint32_t * s32;
    orte_node_state_t *node_state_src, *node_state_dest;
    orte_process_status_t *proc_status_src, *proc_status_dest;
    orte_exit_code_t *exit_code_src, *exit_code_dest;
    char * str;
    char * dstr;

    /* check for errors - if buffer is NULL -or- dest is NOT NULL */
    if (!buffer || dest) { return (ORTE_ERR_BAD_PARAM); }

    src = buffer->from_ptr;  /* get location in buffer */
    mem_left = buffer->toend;  /* how much data is left in buffer */

    /* calculate the pack type size */
    pack_type_size = sizeof(orte_pack_type_t);
    
    /* check to see if there is at least that much left in the buffer */
    if ((int32_t)pack_type_size > mem_left) {
        return ORTE_ERR_UNPACK_FAILURE;
    }

    /* first thing in the current buffer space must be the type
     */
    switch(pack_type_size) {
        case 1:
            s8 = (uint8_t *) src;
            stored_type = (orte_pack_type_t)*s8;
            s8++;
            src = (void *) s8;
            break;
        case 2:
            s16 = (uint16_t *) src;
            stored_type = (orte_pack_type_t)ntohs(*s16);
            s16 += 2;
            src = (void *) s16;
            break;
        case 4:
            s32 = (uint32_t *) src;
            stored_type = (orte_pack_type_t)ntohl(*s32);
            s32 += 4;
            src = (void *) s32;
            break;
        default:
            return ORTE_ERR_UNPACK_FAILURE;
    }
    
    /* check for type match */
    if (stored_type != type) {
        return ORTE_PACK_MISMATCH;
    }
    
    /* account for the memory used */
    mem_left = mem_left - pack_type_size;

    /* got enough left for num_values? */

    if (4 > mem_left) { /* not enough memory  */
        return ORTE_ERR_UNPACK_FAILURE;
    }

    /* unpack the number of values */
    s32 = (uint32_t *) src;
    num_vals = (size_t)ntohl(*s32);
    if (num_vals > *max_num_vals) {  /* not enough space provided */
        return ORTE_UNPACK_INADEQUATE_SPACE;
    }
    src = (void *)((char *)src + 4);
    mem_left = mem_left - 4;

    /* will check to see if adequate storage in buffer prior
     * to unpacking the item
     */
    
    /* default to ORTE_SUCCESS */
    rc = ORTE_SUCCESS;
    
    switch(type) {
        case ORTE_BYTE:
            if ((int)num_vals > mem_left) {
                num_vals = mem_left;
                rc = ORTE_UNPACK_READ_PAST_END_OF_BUFFER;
            }
            s8 = (uint8_t *) src;
            d8 = (uint8_t *) dest;
            memcpy(d8, s8, num_vals);
            dest = (void *)d8;
            break;
            
        case ORTE_INT8:
            d8 = (uint8_t *) malloc(sizeof(uint8_t));
            if (NULL == d8) {
                return ORTE_ERR_OUT_OF_RESOURCE;
            }
            s8 = (uint8_t *) src;
            *d8 = *s8;
            dest = (void *) d8;
            break;
            
        case ORTE_NODE_STATE:
            tst = sizeof(orte_node_state_t);
            node_state_dest = (orte_node_state_t *) malloc(tst);
            if (NULL == node_state_dest) {
                return ORTE_ERR_OUT_OF_RESOURCE;
            }
            node_state_src = (orte_node_state_t *) src;
            if (1 == tst) {  /* single byte field */
                *node_state_dest = *node_state_src;
            } else if (2 == tst) {  /* two byte field */
                *node_state_dest = ntohs(*node_state_src);
            } else if (4 == tst) { /* four byte field */
                *node_state_dest = ntohl(*node_state_src);
            } else {  /* no idea what this is */
                return ORTE_ERR_UNPACK_FAILURE;
            }
            dest = (void*) node_state_dest;
            break;

        case ORTE_PROCESS_STATUS:
            tst = sizeof(orte_process_status_t);
            proc_status_dest = (orte_process_status_t *) malloc(tst);
            if (NULL == proc_status_dest) {
                return ORTE_ERR_OUT_OF_RESOURCE;
            }
            proc_status_src = (orte_process_status_t *) src;
            if (1 == tst) {  /* single byte field */
                *proc_status_dest = *proc_status_src;
            } else if (2 == tst) {  /* two byte field */
                *proc_status_dest = ntohs(*proc_status_src);
            } else if (4 == tst) { /* four byte field */
                *proc_status_dest = ntohl(*proc_status_src);
            } else {  /* no idea what this is */
                return ORTE_ERR_UNPACK_FAILURE;
            }
            dest = (void*) proc_status_dest;
            break;

        case ORTE_EXIT_CODE:
            tst = sizeof(orte_exit_code_t);
            exit_code_dest = (orte_exit_code_t *) malloc(tst);
            if (NULL == exit_code_dest) {
                return ORTE_ERR_OUT_OF_RESOURCE;
            }
            exit_code_src = (orte_exit_code_t *) src;
            if (1 == tst) {  /* single byte field */
                *node_state_dest = *node_state_src;
            } else if (2 == tst) {  /* two byte field */
                *node_state_dest = ntohs(*node_state_src);
            } else if (4 == tst) { /* four byte field */
                *node_state_dest = ntohl(*node_state_src);
            } else {  /* no idea what this is */
                return ORTE_ERROR;
            }
            dest = (void *) exit_code_dest;
            break;
            
        case ORTE_INT16:
            d16 = (uint16_t *) malloc(sizeof(uint16_t));
            if (NULL == d16) {
                return ORTE_ERR_OUT_OF_RESOURCE;
            }
            s16 = (uint16_t *) src;
            /* convert the network order to host order */
            *d16 = ntohs(*s16);
            dest = (void *) d16;
            break;
            
        case ORTE_INT32:
            d32 = (uint32_t *) malloc(sizeof(uint32_t));
            if (NULL == d32) {
                return ORTE_ERR_OUT_OF_RESOURCE;
            }
            s32 = (uint32_t *) src;
            /* convert the network order to host order */
            *d32 = ntohl(*s32);
            dest = (void *) d32;
            break;
            
        case ORTE_JOBID:
            orte_name_services.unpack_jobid(dest, src);
            break;
        
        case ORTE_CELLID:
            orte_name_services.unpack_cellid(dest, src);
            break;
            
        case ORTE_NAME:
            orte_name_services.unpack_name(dest, src);
            break;
        
        case ORTE_STRING:
            str = (char *) src;
            dstr = (char *) dest;
            memcpy(dstr, str, 32);
        default:
            return ORTE_ERROR;
    }
    
    /* ok, we managed to unpack some stuff, so update all ptrs/cnts */
    buffer->from_ptr = ((char*)buffer->from_ptr) + op_size;

    buffer->toend -= op_size; /* closer to the end */
    buffer->len   -= op_size; /* and less data left */

    /* return the number of values unpacked */
    *max_num_vals = num_vals;
    
    return rc;

}

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
 * DPS_PACK_VALUE
 */

int orte_dps_pack(orte_buffer_t *buffer, void *src,
                  size_t num_vals,
                  orte_pack_type_t type)
{
    int rc;
    void *dest;
    orte_buffer_t* bptr;
    int32_t pack_type_size=0, desc_len=0;
    int32_t op_size=0, tst=0;
    uint8_t  * d8;
    uint8_t  * s8;
    uint16_t * d16;
    uint16_t * s16;
    uint32_t * d32;
    uint32_t * s32;
    char * str;
    char * dstr;
    orte_node_state_t *node_state_src, *node_state_dest;
    orte_process_status_t *proc_status_src, *proc_status_dest;
    orte_exit_code_t *exit_code_src, *exit_code_dest;
    
    /* check for error */
    if (!buffer || !src) { return (ORTE_ERROR); }
    
    dest = buffer->data_ptr;    /* get location in buffer */

    /* calculate the required memory size for this operation */
    if (0 == (op_size = orte_dps_memory_required(false, src, type))) {  /* got error */
        return ORTE_ERROR;
    }
    
    /* add in the space for the pack type */
    op_size += sizeof(orte_pack_type_t);
    
    /* check to see if current buffer has enough room */
    if (op_size > buffer->space) {  /* need to extend the buffer */
        if (ORTE_SUCCESS != (rc = orte_dps_buffer_extend(buffer, op_size))) { /* got an error */
            return rc;
        }
        dest = buffer->data_ptr;  /* need to reset the dest since it could have moved */
    }
    
    /* store the pack data type */
    pack_type_size = sizeof(orte_pack_type_t);
    switch(pack_type_size) {
        case 1:
            d8 = (uint8_t *) dest;
            *d8 = type;
            d8++;
            dest = (void *) d8;
            break;
        case 2:
            d16 = (uint16_t *) dest;
            *d16 = htons(type);
            d16 += 2;
            dest = (void *) d16;
            break;
        case 4:
            d32 = (uint32_t *) dest;
            *d32 = htonl(type);
            d32 += 4;
            dest = (void *) d32;
            break;
        default:
            return ORTE_ERROR;
    }

    /* store the number of values as uint32_t */
    d32 = (uint32_t *) dest;
    *d32 = htonl((uint32_t)num_vals);
    d32 += 4;
    dest = (void *) d32;
   
    /* pack the data */
    switch(type) {
        case ORTE_BYTE:
            s8 = (uint8_t *) src;
            memcpy(dest, s8, num_vals);
            dest = (void *)((char *)dest + num_vals);
            break;
            
        case ORTE_INT8:
            s8 = (uint8_t *) src;
            d8 = (uint8_t *) dest;
            *d8 = *s8;
            dest = (void *)((char *)dest + 1);
            break;
            
        case ORTE_NODE_STATE:
            node_state_src = (orte_node_state_t *) src;
            node_state_dest = (orte_node_state_t *) dest;
            tst = sizeof(orte_node_state_t);
            if (1 == tst) {  /* single byte field */
                *node_state_dest = *node_state_src;
            } else if (2 == tst) {  /* two byte field */
                *node_state_dest = htons(*node_state_src);
            } else if (4 == tst) { /* four byte field */
                *node_state_dest = htonl(*node_state_src);
            } else {  /* no idea what this is */
                return ORTE_ERROR;
            }
            dest = (void *)((char *)dest + tst);
            break;

        case ORTE_PROCESS_STATUS:
            proc_status_src = (orte_process_status_t *) src;
            proc_status_dest = (orte_process_status_t *) dest;
            tst = sizeof(orte_process_status_t);
            if (1 == tst) {  /* single byte field */
                *proc_status_dest = *proc_status_src;
            } else if (2 == tst) {  /* two byte field */
                *proc_status_dest = htons(*proc_status_src);
            } else if (4 == tst) { /* four byte field */
                *proc_status_dest = htonl(*proc_status_src);
            } else {  /* no idea what this is */
                return ORTE_ERROR;
            }
            dest = (void *)((char *)dest + tst);
            break;

        case ORTE_EXIT_CODE:
            exit_code_src = (orte_exit_code_t *) src;
            exit_code_dest = (orte_exit_code_t *) dest;
            tst = sizeof(orte_exit_code_t);
            if (1 == tst) {  /* single byte field */
                *exit_code_dest = *exit_code_src;
            } else if (2 == tst) {  /* two byte field */
                *exit_code_dest = htons(*exit_code_src);
            } else if (4 == tst) { /* four byte field */
                *exit_code_dest = htonl(*exit_code_src);
            } else {  /* no idea what this is */
                return ORTE_ERROR;
            }
            dest = (void *)((char *)dest + tst);
            break;
            
        case ORTE_INT16:
            d16 = (uint16_t *) dest;
            s16 = (uint16_t *) src;
            /* convert the host order to network order */
            *d16 = htons(*s16);
            break;
            
        case ORTE_INT32:
            d32 = (uint32_t *) dest;
            s32 = (uint32_t *) src;
            /* convert the host order to network order */
            *d32 = htonl(*s32);
            dest = (void *)((char *)dest + 4);
            break;
            
        case ORTE_JOBID:
            orte_name_services.pack_jobid(dest, src);
            break;
        
        case ORTE_CELLID:
            orte_name_services.pack_cellid(dest, src);
            break;
            
        case ORTE_NAME:
            orte_name_services.pack_name(dest, src);
            break;
            
        case ORTE_STRING:
            str = (char *) src;
            d32 = (uint32_t *) dest;
            *d32 = (uint32_t) strlen(str);
            dest = (void *) ((char *)dest + 4);
        
            dstr = (char *) dest;
            memcpy(dstr, str, *d32);
            dest = (void *)((char *)dest + (*d32));
            break;
            
        default:
            return ORTE_ERROR;
    }
    
    /* ok, we managed to pack some more stuff, so update all ptrs/cnts */
    buffer->data_ptr = dest;

    bptr->len += op_size;
    bptr->toend += op_size;

    bptr->space -= op_size;

    return ORTE_SUCCESS;
}

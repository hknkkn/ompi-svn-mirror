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
    void *dest;
    int32_t pack_type_size=0;
    int32_t op_size=0, tst=0;
    size_t i;
    uint8_t  * d8;
    uint8_t  * s8;
    uint16_t * d16;
    uint16_t * s16;
    uint32_t * d32;
    uint32_t * s32;
    bool *bool_src, *bool_dest;
    char * str;
    char * dstr;
    orte_process_name_t *dn, *sn;
    orte_vpid_t *dv, *sv;
    orte_jobid_t *dj, *sj;
    orte_cellid_t *dc, *sc;
    orte_node_state_t *node_state_src, *node_state_dest;
    orte_process_status_t *proc_status_src, *proc_status_dest;
    orte_exit_code_t *exit_code_src, *exit_code_dest;
    orte_byte_object_t *sbyteptr, *dbyteptr;
    
    /* check for error */
    if (!buffer || !src || 0 >= num_vals) { return (ORTE_ERROR); }
    
    dest = buffer->data_ptr;    /* get location in buffer */

    /* calculate the required memory size for this operation */
    if (0 == (op_size = orte_dps_memory_required(src, num_vals, type))) {  /* got error */
        return ORTE_ERROR;
    }
    
    /* add in the space for the pack type */
    op_size += sizeof(orte_data_type_t);
    
    /* add in the space to store the number of values */
    op_size += 4;

    /* check to see if current buffer has enough room */
    if (op_size > buffer->space) {  /* need to extend the buffer */
        if (ORTE_SUCCESS != (rc = orte_dps_buffer_extend(buffer, op_size))) { /* got an error */
            fprintf(stderr, "dps_pack: buffer extend failed\n");
            return rc;
        }
        dest = buffer->data_ptr;  /* need to reset the dest since it could have moved */
    }
    
    /* check for size of generic data types so they can be properly packed
     * NOTE we convert the generic data type flag to a hard type for storage
     * to handle heterogeneity
     */
    if (ORTE_INT == type || ORTE_UINT == type) {
        tst = sizeof(int);
        if (1 == tst) {
            type = ORTE_INT8;
        } else if (2 == tst) {
            type = ORTE_INT16;
        } else if (4 == tst) {
            type = ORTE_INT32;
        } else if (8 == tst) {
            type = ORTE_INT64;
        } else {
            return ORTE_ERR_NOT_IMPLEMENTED;
        }
    }
    
    /* store the pack data type */
    pack_type_size = sizeof(orte_data_type_t);
    switch(pack_type_size) {
        case 1:
            d8 = (uint8_t *) dest;
            *d8 = type;
            d8++;
            dest = (void *)d8;
            break;
        case 2:
            d16 = (uint16_t *) dest;
            *d16 = htons(type);
            d16++;
            dest = (void *)d16;
            break;
        case 4:
            d32 = (uint32_t *) dest;
            *d32 = htonl(type);
            d32++;
            dest = (void *)d32;
            break;
        default:
            return ORTE_ERROR;
    }
    
    /* store the number of values as uint32_t */
    d32 = (uint32_t *) dest;
    *d32 = htonl((uint32_t)num_vals);
    d32++;
    dest = (void *)d32;

    /* pack the data */
    switch(type) {
        case ORTE_BYTE:
        case ORTE_INT8:
        case ORTE_UINT8:
            s8 = (uint8_t *) src;
            memcpy(dest, s8, num_vals);
            dest = (void *)((char *)dest + num_vals);
            break;
        
        case ORTE_INT16:
        case ORTE_UINT16:
            d16 = (uint16_t *) dest;
            s16 = (uint16_t *) src;
            for (i=0; i<num_vals; i++) {
                /* convert the host order to network order */
                *d16 = htons(*s16);
                d16++; s16++;
            }
            dest = (void *)d16;
            break;
            
        case ORTE_INT32:
        case ORTE_UINT32:
            d32 = (uint32_t *) dest;
            s32 = (uint32_t *) src;
            for (i=0; i<num_vals; i++) {
                /* convert the host order to network order */
                *d32 = htonl(*s32);
                d32++; s32++;
            }
            dest = (void *)d32;
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
            bool_dest = (bool *) dest;
            tst = sizeof(bool);
            for (i=0; i<num_vals; i++) {
                if (1 == tst) {  /* single byte field */
                    *bool_dest = *bool_src;
                } else if (2 == tst) {  /* two byte field */
                    *bool_dest = htons(*bool_src);
                } else if (4 == tst) { /* four byte field */
                    *bool_dest = htonl(*bool_src);
                } else {  /* no idea what this is */
                    return ORTE_ERROR;
                }
                bool_dest++; bool_src++;
            }
            dest = (void *)bool_dest;
            break;

        case ORTE_STRING:
            str = (char *) src;
            d32 = (uint32_t *) dest;
            for (i=0; i<num_vals; i++) {
                /* store string length as uint32_t */
                *d32 = (uint32_t) strlen(str);
                dest = (void *) ((char *)dest + 4);
                /* move the char's over */
                dstr = (char *) dest;
                memcpy(dstr, str, *d32);
                dest = (void *)((char *)dest + (*d32)); /* update destination */
                str++;  /* move to next string */
            }
            break;
            
        case ORTE_NAME:
            dn = (orte_process_name_t*) dest;
            sn = (orte_process_name_t*) src;
            for (i=0; i<num_vals; i++) {
                dn->cellid = htonl(sn->cellid);
                dn->jobid = htonl(sn->jobid);
                dn->vpid = htonl(sn->vpid);
                dn++; sn++;
            }
            dest = (void *)dn;
            break;
            
        case ORTE_VPID:
            dv = (orte_vpid_t *) dest;
            sv = (orte_vpid_t *) src;
            for (i=0; i<num_vals; i++) {
                *dv = htonl(*sv);
                dv++; sv++;
            }
            dest = (void *)dv;
            break;
            
        case ORTE_JOBID:
            dj = (orte_jobid_t*) dest;
            sj = (orte_jobid_t*) src;
            for (i=0; i<num_vals; i++) {
                *dj = htonl(*sj);
                dj++; sj++;
            }
            dest = (void *)dj;
            break;
        
        case ORTE_CELLID:
            dc = (orte_cellid_t*) dest;
            sc = (orte_cellid_t*) src;
            for (i=0; i<num_vals; i++) {
                *dc = htonl(*sc);
                dc++; sc++;
            }
            dest = (void *)dc;
            break;
            
        case ORTE_NODE_STATE:
            node_state_src = (orte_node_state_t *) src;
            node_state_dest = (orte_node_state_t *) dest;
            tst = sizeof(orte_node_state_t);
            for (i=0; i<num_vals; i++) {
                if (1 == tst) {  /* single byte field */
                    *node_state_dest = *node_state_src;
                } else if (2 == tst) {  /* two byte field */
                    *node_state_dest = htons(*node_state_src);
                } else if (4 == tst) { /* four byte field */
                    *node_state_dest = htonl(*node_state_src);
                } else {  /* no idea what this is */
                    return ORTE_ERROR;
                }
                node_state_dest++; node_state_src++;
            }
            dest = (void *)node_state_dest;
            break;

        case ORTE_PROCESS_STATUS:
            proc_status_src = (orte_process_status_t *) src;
            proc_status_dest = (orte_process_status_t *) dest;
            tst = sizeof(orte_process_status_t);
            for (i=0; i<num_vals; i++) {
                if (1 == tst) {  /* single byte field */
                    *proc_status_dest = *proc_status_src;
                } else if (2 == tst) {  /* two byte field */
                    *proc_status_dest = htons(*proc_status_src);
                } else if (4 == tst) { /* four byte field */
                    *proc_status_dest = htonl(*proc_status_src);
                } else {  /* no idea what this is */
                    return ORTE_ERROR;
                }
                proc_status_dest++; proc_status_src++;
            }
            dest = (void *)proc_status_dest;
            break;

        case ORTE_EXIT_CODE:
            exit_code_src = (orte_exit_code_t *) src;
            exit_code_dest = (orte_exit_code_t *) dest;
            tst = sizeof(orte_exit_code_t);
            for (i=0; i<num_vals; i++) {
                if (1 == tst) {  /* single byte field */
                    *exit_code_dest = *exit_code_src;
                } else if (2 == tst) {  /* two byte field */
                    *exit_code_dest = htons(*exit_code_src);
                } else if (4 == tst) { /* four byte field */
                    *exit_code_dest = htonl(*exit_code_src);
                } else {  /* no idea what this is */
                    return ORTE_ERROR;
                }
                exit_code_dest++; exit_code_src++;
            }
            dest = (void *)exit_code_dest;
            break;
            
        case ORTE_BYTE_OBJECT:
            sbyteptr = (orte_byte_object_t *) src;
            for (i=0; i<num_vals; i++) {
                dbyteptr = (orte_byte_object_t *) dest;
                dbyteptr->size = sbyteptr->size;
                memcpy(dbyteptr->bytes, sbyteptr->bytes, dbyteptr->size);
                sbyteptr++;
                dest = (void *)((char *)dest + sizeof(dbyteptr->size) + dbyteptr->size);
            }
            break;
            
        default:
            return ORTE_ERR_BAD_PARAM;
    }
    
    /* ok, we managed to pack some more stuff, so update all ptrs/cnts */
    buffer->data_ptr = dest;

    buffer->len += op_size;
    buffer->toend += op_size;

    buffer->space -= op_size;

    return ORTE_SUCCESS;
}

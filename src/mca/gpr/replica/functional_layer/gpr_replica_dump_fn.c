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
 * The Open MPI general purpose registry - implementation.
 *
 */

/*
 * includes
 */

#include "orte_config.h"

#include "include/orte_constants.h"
#include "include/orte_types.h"

#include "dps/dps.h"
#include "util/output.h"

#include "mca/ns/ns_types.h"
#include "mca/soh/soh_types.h"

#include "mca/gpr/replica/transition_layer/gpr_replica_tl.h"
#include "gpr_replica_fn.h"

static void orte_gpr_replica_dump_load_string(orte_buffer_t *buffer, char **tmp);

static void orte_gpr_replica_dump_itagval_value(orte_buffer_t *buffer,
                                                orte_gpr_replica_itagval_t *iptr);

static void orte_gpr_replica_dump_trigger(orte_buffer_t *buffer, int cnt,
                                          orte_gpr_replica_triggers_t *trig);


int orte_gpr_replica_dump_fn(orte_buffer_t *buffer)
{
    orte_gpr_replica_segment_t **seg;
    orte_gpr_replica_container_t **cptr;
    orte_gpr_replica_itag_t *itaglist;
    orte_gpr_replica_itagval_t **iptr;
    orte_gpr_replica_triggers_t **trig;
    char *token;
    int num_objects;
    int i, j, k;
    char *tmp_out;

    asprintf(&tmp_out, "DUMP OF GENERAL PURPOSE REGISTRY\n\n");
    orte_gpr_replica_dump_load_string(buffer, &tmp_out);
    
    trig = (orte_gpr_replica_triggers_t**)((orte_gpr_replica.triggers)->addr);
    k = 0;
    for (j=0; j < (orte_gpr_replica.triggers)->size; j++) {
        if (NULL != trig[j]) k++;
    }
    
    asprintf(&tmp_out, "Number of triggers: %d\n", k);
    orte_gpr_replica_dump_load_string(buffer, &tmp_out);
    
    /* dump the trigger info for the registry */
    for (j=0, k=0; j < (orte_gpr_replica.triggers)->size; j++) {
        if (NULL != trig[j]) {
            orte_gpr_replica_dump_trigger(buffer, k, trig[j]);
            k++;
        }
    }

    /* loop through all segments */
    seg = (orte_gpr_replica_segment_t**)(orte_gpr_replica.segments)->addr;
    for (i=0; i < (orte_gpr_replica.segments)->size; i++) {
         if (NULL != seg[i]) {

            	asprintf(&tmp_out, "\nGPR Dump for Segment: %s", seg[i]->name);
            	orte_gpr_replica_dump_load_string(buffer, &tmp_out);
            
            	num_objects = (seg[i]->containers)->size - (seg[i]->containers)->number_free;
            
            	asprintf(&tmp_out, "\tNumber of containers: %d\n", num_objects);
            	orte_gpr_replica_dump_load_string(buffer, &tmp_out);
            
            	/* loop through all containers and print their info and contents */
             cptr = (orte_gpr_replica_container_t**)(seg[i]->containers)->addr;
            	for (j=0; j < (seg[i]->containers)->size; j++) {
                if (NULL != cptr[j]) {
                	    asprintf(&tmp_out, "\n\tInfo for container %d\tNumber of keyvals: %d\n\tTokens:\n",
                                 j, (cptr[j]->itagvals)->size - (cptr[j]->itagvals)->number_free);
                	    orte_gpr_replica_dump_load_string(buffer, &tmp_out);
                
                	    /* reverse lookup tokens and print them */
                     itaglist = cptr[j]->itags;
                	    for (k=0; k < cptr[j]->num_itags; k++) {
                		     if (ORTE_SUCCESS != orte_gpr_replica_dict_reverse_lookup(
                                                            &token, seg[i], itaglist[k])) {
                		          asprintf(&tmp_out, "\t\titag num %d: No entry found for itag %X",
                			             k, itaglist[k]);
                		     } else {
                		          asprintf(&tmp_out, "\t\titag num %d: itag %d\tToken: %s",
                			             k, itaglist[k], token);
                		          free(token);
                		     }
                		     orte_gpr_replica_dump_load_string(buffer, &tmp_out);
                	    }
                     
                     asprintf(&tmp_out, "\n\tKeyval info:");
                     orte_gpr_replica_dump_load_string(buffer, &tmp_out);
                
                     /* loop through all itagvals and print their info */
                     iptr = (orte_gpr_replica_itagval_t**)(cptr[j]->itagvals)->addr;
                     for (k=0; k < (cptr[j]->itagvals)->size; k++) {
                          if (NULL != iptr[k]) {
                              if (ORTE_SUCCESS != orte_gpr_replica_dict_reverse_lookup(
                                                        &token, seg[i], iptr[k]->itag)) {
                                   asprintf(&tmp_out, "\n\t\titag num %d: No entry found for itag %X",
                                       k, iptr[k]->itag);
                              } else {
                                   asprintf(&tmp_out, "\n\t\tEntry %d: itag %d\tKey: %s",
                                        k, iptr[k]->itag, token);
                                   free(token);
                              }
                              orte_gpr_replica_dump_load_string(buffer, &tmp_out);
                              orte_gpr_replica_dump_itagval_value(buffer, iptr[k]);
                          }
                     }
                }
            	}
         }
    }
    
    return ORTE_SUCCESS;
}

static void orte_gpr_replica_dump_trigger(orte_buffer_t *buffer, int cnt,
                                          orte_gpr_replica_triggers_t *trig)
{
    char *tmp_out, *token;
    int i, k;
    orte_gpr_replica_target_t **target;
    orte_gpr_replica_itagval_t **ival;
    
	asprintf(&tmp_out, "\nData for trigger %d on segment %s",
                        cnt, (trig->seg)->name);
	orte_gpr_replica_dump_load_string(buffer, &tmp_out);

	/* output recipient info */
		if (NULL == trig->requestor) {
		    asprintf(&tmp_out, "\tIntended recipient: LOCAL");
    	} else {
	    asprintf(&tmp_out, "\tIntended recipient: [%d,%d,%d]", ORTE_NAME_ARGS(trig->requestor));
	}
 
	orte_gpr_replica_dump_load_string(buffer, &tmp_out);
	tmp_out = strdup("\tActions:");
    orte_gpr_replica_dump_load_string(buffer, &tmp_out);
	if (ORTE_GPR_NOTIFY_VALUE_CHG & trig->action) {
        tmp_out = strdup("\t\tORTE_GPR_NOTIFY_VALUE_CHG");
        orte_gpr_replica_dump_load_string(buffer, &tmp_out);
	} else if (ORTE_GPR_NOTIFY_VALUE_CHG_TO & trig->action) {
        tmp_out = strdup("\t\tORTE_GPR_NOTIFY_VALUE_CHG_TO");
        orte_gpr_replica_dump_load_string(buffer, &tmp_out);
	} else if (ORTE_GPR_NOTIFY_VALUE_CHG_FRM & trig->action) {
        tmp_out = strdup("\t\tORTE_GPR_NOTIFY_VALUE_CHG_FRM");
        orte_gpr_replica_dump_load_string(buffer, &tmp_out);
    }
	if (ORTE_GPR_NOTIFY_DEL_ENTRY & trig->action) {
	    tmp_out = strdup("\t\tORTE_GPR_NOTIFY_DEL_ENTRY");
        orte_gpr_replica_dump_load_string(buffer, &tmp_out);
	}
	if (ORTE_GPR_NOTIFY_ADD_ENTRY & trig->action) {
	    tmp_out = strdup("\t\tORTE_GPR_NOTIFY_ADD_ENTRY");
        orte_gpr_replica_dump_load_string(buffer, &tmp_out);
	}
	if (ORTE_GPR_NOTIFY_PRE_EXISTING & trig->action) {
	    tmp_out = strdup("\t\tORTE_GPR_NOTIFY_PRE_EXISTING");
        orte_gpr_replica_dump_load_string(buffer, &tmp_out);
	}
	if (ORTE_GPR_TRIG_ONE_SHOT & trig->action) {
	    tmp_out = strdup("\t\tORTE_GPR_TRIG_ONE_SHOT");
        orte_gpr_replica_dump_load_string(buffer, &tmp_out);
	}
    if (ORTE_GPR_TRIG_CMP_LEVELS & trig->action) {
        tmp_out = strdup("\t\tORTE_GPR_TRIG_CMP_LEVELS");
        orte_gpr_replica_dump_load_string(buffer, &tmp_out);
    }
    if (ORTE_GPR_TRIG_MONITOR_ONLY & trig->action) {
        tmp_out = strdup("\t\tORTE_GPR_TRIG_MONITOR_ONLY");
        orte_gpr_replica_dump_load_string(buffer, &tmp_out);
    }
    if (ORTE_GPR_TRIG_NOTIFY_START & trig->action) {
        tmp_out = strdup("\t\tORTE_GPR_TRIG_NOTIFY_START");
        orte_gpr_replica_dump_load_string(buffer, &tmp_out);
    }
    if (ORTE_GPR_TRIG_INCLUDE_DATA & trig->action) {
        tmp_out = strdup("\t\tORTE_GPR_TRIG_INCLUDE_DATA");
        orte_gpr_replica_dump_load_string(buffer, &tmp_out);
    }

	asprintf(&tmp_out, "\tData covered by this subscription");
	orte_gpr_replica_dump_load_string(buffer, &tmp_out);

    k = (int)orte_value_array_get_size(&(trig->tokentags));
    if (0 == k) {
        asprintf(&tmp_out, "\t\tNULL token (wildcard)");
        orte_gpr_replica_dump_load_string(buffer, &tmp_out);
    } else {
        asprintf(&tmp_out, "\t\tNumber of tokens: %d", k);
        orte_gpr_replica_dump_load_string(buffer, &tmp_out);
    
        for (i=0; i < k; i++) {
            if (ORTE_SUCCESS == orte_gpr_replica_dict_reverse_lookup(&token, trig->seg,
                    ORTE_VALUE_ARRAY_GET_ITEM(&(trig->tokentags), orte_gpr_replica_itag_t, i))) {
                asprintf(&tmp_out, "\t\t\tToken: %s", token);
    		    orte_gpr_replica_dump_load_string(buffer, &tmp_out);
                free(token);
            }
    	   }
    }
    
    asprintf(&tmp_out, "\t\tToken addressing mode:\n");
    orte_gpr_replica_dump_load_string(buffer, &tmp_out);

    if (ORTE_GPR_REPLICA_NOT & trig->token_addr_mode) {
        asprintf(&tmp_out, "\t\t\tORTE_GPR_TOKENS_NOT\n");
        orte_gpr_replica_dump_load_string(buffer, &tmp_out);
    }
    if (ORTE_GPR_REPLICA_AND & trig->token_addr_mode) {
        asprintf(&tmp_out, "\t\t\tORTE_GPR_TOKENS_AND\n");
        orte_gpr_replica_dump_load_string(buffer, &tmp_out);
    }
    if (ORTE_GPR_REPLICA_OR & trig->token_addr_mode) {
        asprintf(&tmp_out, "\t\t\tORTE_GPR_TOKENS_OR\n");
        orte_gpr_replica_dump_load_string(buffer, &tmp_out);
    }
    if (ORTE_GPR_REPLICA_XAND & trig->token_addr_mode) {
        asprintf(&tmp_out, "\t\t\tORTE_GPR_TOKENS_XAND\n");
        orte_gpr_replica_dump_load_string(buffer, &tmp_out);
    }
    if (ORTE_GPR_REPLICA_XOR & trig->token_addr_mode) {
        asprintf(&tmp_out, "\t\t\tORTE_GPR_TOKENS_XOR\n");
        orte_gpr_replica_dump_load_string(buffer, &tmp_out);
    }
    
    k = (int)orte_value_array_get_size(&(trig->tokentags));
    if (0 == k) {
        asprintf(&tmp_out, "\t\tNULL key (wildcard)");
        orte_gpr_replica_dump_load_string(buffer, &tmp_out);
    } else {
        asprintf(&tmp_out, "\t\tNumber of keys: %d", k);
        orte_gpr_replica_dump_load_string(buffer, &tmp_out);

        for (i=0; i < k; i++) {
            if (ORTE_SUCCESS == orte_gpr_replica_dict_reverse_lookup(&token, trig->seg,
                    ORTE_VALUE_ARRAY_GET_ITEM(&(trig->keytags), orte_gpr_replica_itag_t, i))) {
                asprintf(&tmp_out, "\t\t\tKey: %s", token);
                orte_gpr_replica_dump_load_string(buffer, &tmp_out);
                free(token);
            }
        }
    }
    
    asprintf(&tmp_out, "\t\tKey addressing mode:\n");
    orte_gpr_replica_dump_load_string(buffer, &tmp_out);

    if (ORTE_GPR_REPLICA_NOT & trig->key_addr_mode) {
        asprintf(&tmp_out, "\t\t\tORTE_GPR_KEYS_NOT\n");
        orte_gpr_replica_dump_load_string(buffer, &tmp_out);
    }
    if (ORTE_GPR_REPLICA_AND & trig->key_addr_mode) {
        asprintf(&tmp_out, "\t\t\tORTE_GPR_KEYS_AND\n");
        orte_gpr_replica_dump_load_string(buffer, &tmp_out);
    }
    if (ORTE_GPR_REPLICA_OR & trig->key_addr_mode) {
        asprintf(&tmp_out, "\t\t\tORTE_GPR_KEYS_OR\n");
        orte_gpr_replica_dump_load_string(buffer, &tmp_out);
    }
    if (ORTE_GPR_REPLICA_XAND & trig->key_addr_mode) {
        asprintf(&tmp_out, "\t\t\tORTE_GPR_KEYS_XAND\n");
        orte_gpr_replica_dump_load_string(buffer, &tmp_out);
    }
    if (ORTE_GPR_REPLICA_XOR & trig->key_addr_mode) {
        asprintf(&tmp_out, "\t\t\tORTE_GPR_KEYS_XOR\n");
        orte_gpr_replica_dump_load_string(buffer, &tmp_out);
    }
    
    asprintf(&tmp_out, "\tIdentified targets:");
    orte_gpr_replica_dump_load_string(buffer, &tmp_out);

    target = (orte_gpr_replica_target_t**)((trig->targets)->addr);
    if (0 == trig->num_targets) {
        asprintf(&tmp_out, "\t\tNone");
        orte_gpr_replica_dump_load_string(buffer, &tmp_out);
    } else {
        for (i=0; i < (trig->targets)->size; i++) {
            if (NULL != target[i]) {
                if (NULL == target[i]->cptr) {
                    asprintf(&tmp_out, "\t\tContainer: NULL");
                } else {
                    asprintf(&tmp_out, "\t\tContainer: %d", (target[i]->cptr)->index);
                }
                orte_gpr_replica_dump_load_string(buffer, &tmp_out);
                if (NULL == target[i]->iptr) {
                    asprintf(&tmp_out, "\t\tKeyval: NULL");
                } else {
                    if (ORTE_SUCCESS == orte_gpr_replica_dict_reverse_lookup(&token,
                                            trig->seg, (target[i]->iptr)->itag)) {
                        asprintf(&tmp_out, "\t\tKeyval: %s", token);
                        free(token);
                    }
                }
                orte_gpr_replica_dump_load_string(buffer, &tmp_out);
            }
        }
    }
    
    if (0 < trig->num_counters) {
        asprintf(&tmp_out, "\tTrigger monitoring %d counters", trig->num_counters);
        orte_gpr_replica_dump_load_string(buffer, &tmp_out);
        ival = (orte_gpr_replica_itagval_t**)((trig->counters)->addr);
        for (i=0; i < trig->num_counters; i++) {
            if (NULL != ival[i] &&
                ORTE_SUCCESS == orte_gpr_replica_dict_reverse_lookup(&token, trig->seg,
                    ival[i]->itag)) {
                asprintf(&tmp_out, "\t\tCounter: %d\tName: %s", i, token);
                free(token);
                orte_gpr_replica_dump_load_string(buffer, &tmp_out);
                orte_gpr_replica_dump_itagval_value(buffer, ival[i]);
            }
        }
    }
    
    return;
}


static void orte_gpr_replica_dump_itagval_value(orte_buffer_t *buffer,
                                                orte_gpr_replica_itagval_t *iptr)
{
    char *tmp;
    
    switch(iptr->type) {

        case ORTE_STRING:
            asprintf(&tmp, "\t\tData type: ORTE_STRING\tValue: %s", iptr->value.strptr);
            break;
            
        case ORTE_UINT8:
            asprintf(&tmp, "\t\tData type: ORTE_UINT8\tValue: %d", (int)iptr->value.ui8);
            break;
            
        case ORTE_UINT16:
            asprintf(&tmp, "\t\tData type: ORTE_UINT16\tValue: %d", (int)iptr->value.ui16);
            break;
            
        case ORTE_UINT32:
            asprintf(&tmp, "\t\tData type: ORTE_UINT32\tValue: %d", (int)iptr->value.ui32);
            break;
            
#ifdef HAVE_I64
        case ORTE_UINT64:
            asprintf(&tmp, "\t\tData type: ORTE_UINT64\tValue: %d", (int)iptr->value.ui64);
            break;
#endif

        case ORTE_INT8:
            asprintf(&tmp, "\t\tData type: ORTE_INT8\tValue: %d", (int)iptr->value.i8);
            break;
        
        case ORTE_INT16:
            asprintf(&tmp, "\t\tData type: ORTE_INT16\tValue: %d", (int)iptr->value.i16);
            break;
        
        case ORTE_INT32:
            asprintf(&tmp, "\t\tData type: ORTE_INT32\tValue: %d", (int)iptr->value.i32);
            break;
        
#ifdef HAVE_I64
        case ORTE_INT64:
            asprintf(&tmp, "\t\tData type: ORTE_INT64\tValue: %d", (int)iptr->value.i64);
            break;
#endif

        case ORTE_BYTE_OBJECT:
            asprintf(&tmp, "\t\tData type: ORTE_BYTE_OBJECT\tSize: %d", (int)(iptr->value.byteobject).size);
            break;
            
        case ORTE_NAME:
            asprintf(&tmp, "\t\tData type: ORTE_NAME\tValue: UNIMPLEMENTED");
            break;
            
        case ORTE_JOBID:
            asprintf(&tmp, "\t\tData type: ORTE_JOBID\tValue: UNIMPLEMENTED");
            break;
            
        case ORTE_NODE_STATE:
            asprintf(&tmp, "\t\tData type: ORTE_NODE_STATE\tValue: %d", (int)iptr->value.node_state);
            break;
            
        case ORTE_PROC_STATE:
            asprintf(&tmp, "\t\tData type: ORTE_PROC_STATE\tValue: %d", (int)iptr->value.proc_state);
            break;
            
        case ORTE_EXIT_CODE:
            asprintf(&tmp, "\t\tData type: ORTE_EXIT_CODE\tValue: %d", (int)iptr->value.exit_code);
            break;
            
        default:
            asprintf(&tmp, "\t\tData type: UNKNOWN");
            break;
    }
    
    orte_gpr_replica_dump_load_string(buffer, &tmp);
}


static void orte_gpr_replica_dump_load_string(orte_buffer_t *buffer, char **tmp)
{
    orte_dps.pack(buffer, tmp, 1, ORTE_STRING);
    free(*tmp);

}

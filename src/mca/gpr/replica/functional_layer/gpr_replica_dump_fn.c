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

#include "mca/gpr/replica/transition_layer/gpr_replica_tl.h"
#include "gpr_replica_fn.h"

static void orte_gpr_replica_dump_load_string(orte_buffer_t *buffer, char **tmp);

static void orte_gpr_replica_dump_itagval_value(orte_buffer_t *buffer,
                                                orte_gpr_replica_itagval_t *iptr);

static void orte_gpr_replica_dump_trigger(orte_buffer_t *buffer, int cnt,
                                          orte_gpr_replica_notify_tracker_t *trig);


int orte_gpr_replica_dump_fn(orte_buffer_t *buffer)
{
    orte_gpr_replica_segment_t **seg;
    orte_gpr_replica_container_t **cptr;
    orte_gpr_replica_itag_t *itaglist;
    orte_gpr_replica_itagval_t **iptr;
    orte_gpr_replica_notify_tracker_t **trig;
    char *token;
    int num_objects;
    int i, j, k;
    char *tmp_out;

    orte_dps.pack(buffer, "DUMP OF GENERAL PURPOSE REGISTRY\n\nTriggers:\n", 1, ORTE_STRING);
    
    /* dump the trigger info for the registry */
    trig = (orte_gpr_replica_notify_tracker_t**)((orte_gpr_replica.triggers)->addr);
    for (i=0; i < (orte_gpr_replica.triggers)->size; i++) {
        if (NULL != trig[i]) {
            orte_gpr_replica_dump_trigger(buffer, i, trig[i]);
        }
    }

    /* loop through all segments */
    seg = (orte_gpr_replica_segment_t**)(orte_gpr_replica.segments)->addr;
    for (i=0; i < (orte_gpr_replica.segments)->size; i++) {
         if (NULL != seg[i]) {

            	asprintf(&tmp_out, "GPR Dump for Segment: %s", seg[i]->name);
            	orte_gpr_replica_dump_load_string(buffer, &tmp_out);
            
            	num_objects = (seg[i]->containers)->size - (seg[i]->containers)->number_free;
            
             k = (int)orte_value_array_get_size(&(seg[i]->triggers));
            	asprintf(&tmp_out, "\tNumber of objects: %d\tNumber of triggers on segment: %d\n",
                        num_objects, k);
            	orte_gpr_replica_dump_load_string(buffer, &tmp_out);
            
            if (0 < k) {
                /* output the number of the triggers */
            }
            
            	/* loop through all containers and print their info and contents */
             cptr = (orte_gpr_replica_container_t**)(seg[i]->containers)->addr;
            	for (j=0; j < (seg[i]->containers)->size; j++) {
                if (NULL != cptr[j]) {
                	    asprintf(&tmp_out, "\tInfo for container %d\tNumber of keyvals: %d\nTokens:\n",
                                 j, (cptr[j]->itagvals)->size - (cptr[j]->itagvals)->number_free);
                	    orte_gpr_replica_dump_load_string(buffer, &tmp_out);
                
                	    /* reverse lookup tokens and print them */
                     itaglist = cptr[j]->itags;
                	    for (k=0; k < cptr[j]->num_itags; k++) {
                		     if (ORTE_SUCCESS != orte_gpr_replica_dict_reverse_lookup(
                                                            &token, seg[i], itaglist[k])) {
                		          asprintf(&tmp_out, "\t\titag num: %d - No entry found for itag %X",
                			             k, itaglist[k]);
                		     } else {
                		          asprintf(&tmp_out, "\t\titag num: %d - itag %d Token: %s",
                			             k, itaglist[k], token);
                		          free(token);
                		     }
                		     orte_gpr_replica_dump_load_string(buffer, &tmp_out);
                	    }
                     
                     asprintf(&tmp_out, "\tKeyval info:\n");
                     orte_gpr_replica_dump_load_string(buffer, &tmp_out);
                
                     /* loop through all itagvals and print their info */
                     iptr = (orte_gpr_replica_itagval_t**)(cptr[j]->itagvals)->addr;
                     for (k=0; k < (cptr[j]->itagvals)->size; k++) {
                          if (NULL != iptr[k]) {
                              if (ORTE_SUCCESS != orte_gpr_replica_dict_reverse_lookup(
                                                        &token, seg[i], iptr[k]->itag)) {
                                   asprintf(&tmp_out, "\t\titag num: %d - No entry found for itag %X",
                                       k, iptr[k]->itag);
                              } else {
                                   asprintf(&tmp_out, "\t\tEntry: %d - itag %d Key: %s",
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
                                          orte_gpr_replica_notify_tracker_t *trig)
{
    char *tmp_out, *token;
    int i, k;
    
    if (ORTE_GPR_SYNCHRO_CMD == trig->cmd) {  /* subscription */
		asprintf(&tmp_out, "\tData for trigger %d\tType: SUBSCRIPTION", cnt);
		orte_gpr_replica_dump_load_string(buffer, &tmp_out);
		asprintf(&tmp_out, "\t\tAssociated with notify number: %d",trig->local_idtag);
		orte_gpr_replica_dump_load_string(buffer, &tmp_out);
		/* find recipient info from notify list */
    		if (NULL == trig->requestor) {
    		    asprintf(&tmp_out, "\tIntended recipient: LOCAL");
        	} else {
		    asprintf(&tmp_out, "\tIntended recipient: [%d,%d,%d]", ORTE_NAME_ARGS(*(trig->requestor)));
		}
		orte_gpr_replica_dump_load_string(buffer, &tmp_out);
		orte_dps.pack(buffer, "\tActions:", 1, ORTE_STRING);
		if (ORTE_GPR_NOTIFY_MODIFICATION & trig->flag.trig_action) {
		    orte_dps.pack(buffer, "\t\tORTE_GPR_NOTIFY_MODIFICATION", 1, ORTE_STRING);
		}
		if (ORTE_GPR_NOTIFY_ADD_SUBSCRIBER & trig->flag.trig_action) {
		    orte_dps.pack(buffer, "\t\tORTE_GPR_NOTIFY_ADD_SUBSCRIBER", 1, ORTE_STRING);
		}
		if (ORTE_GPR_NOTIFY_DELETE_ENTRY & trig->flag.trig_action) {
		    orte_dps.pack(buffer, "\t\tORTE_GPR_NOTIFY_DELETE_ENTRY", 1, ORTE_STRING);
		}
		if (ORTE_GPR_NOTIFY_ADD_ENTRY & trig->flag.trig_action) {
		    orte_dps.pack(buffer, "\t\tORTE_GPR_NOTIFY_ADD_ENTRY", 1, ORTE_STRING);
		}
		if (ORTE_GPR_NOTIFY_ON_STARTUP & trig->flag.trig_action) {
		    orte_dps.pack(buffer, "\t\tORTE_GPR_NOTIFY_ON_STARTUP", 1, ORTE_STRING);
		}
		if (ORTE_GPR_NOTIFY_ON_SHUTDOWN & trig->flag.trig_action) {
		    orte_dps.pack(buffer, "\t\tORTE_GPR_NOTIFY_ON_SHUTDOWN", 1, ORTE_STRING);
		}
		if (ORTE_GPR_NOTIFY_PRE_EXISTING & trig->flag.trig_action) {
		    orte_dps.pack(buffer, "\t\tORTE_GPR_NOTIFY_PRE_EXISTING", 1, ORTE_STRING);
		}
		if (ORTE_GPR_NOTIFY_INCLUDE_STARTUP_DATA & trig->flag.trig_action) {
		    orte_dps.pack(buffer, "\t\tORTE_GPR_NOTIFY_INCLUDE_STARTUP_DATA", 1, ORTE_STRING);
		}
		if (ORTE_GPR_NOTIFY_INCLUDE_SHUTDOWN_DATA & trig->flag.trig_action) {
		    orte_dps.pack(buffer, "\t\tORTE_GPR_NOTIFY_INCLUDE_SHUTDOWN_DATA", 1, ORTE_STRING);
		}
		if (ORTE_GPR_NOTIFY_ONE_SHOT & trig->flag.trig_action) {
		    orte_dps.pack(buffer, "\t\tORTE_GPR_NOTIFY_ONE_SHOT", 1, ORTE_STRING);
		}

    } else {  /* synchro */
		asprintf(&tmp_out, "\tData for trigger %d\tType: SYNCHRO", cnt);
		orte_gpr_replica_dump_load_string(buffer, &tmp_out);
		asprintf(&tmp_out, "\t\tAssociated with notify number: %d",trig->local_idtag);
		orte_gpr_replica_dump_load_string(buffer, &tmp_out);

		if (NULL == trig->requestor) {
		    asprintf(&tmp_out, "\tIntended recipient: LOCAL");
		} else {
		    asprintf(&tmp_out, "\tIntended recipient: [%d,%d,%d]", ORTE_NAME_ARGS(*(trig->requestor)));
		}
		orte_gpr_replica_dump_load_string(buffer, &tmp_out);
		orte_dps.pack(buffer, "\tSynchro Mode:", 1, ORTE_STRING);
		if (ORTE_GPR_SYNCHRO_MODE_ASCENDING & trig->flag.trig_synchro) {
		    orte_dps.pack(buffer, "\t\tORTE_GPR_SYNCHRO_MODE_ASCENDING", 1, ORTE_STRING);
		}
		if (ORTE_GPR_SYNCHRO_MODE_DESCENDING & trig->flag.trig_synchro) {
		    orte_dps.pack(buffer, "\t\tORTE_GPR_SYNCHRO_MODE_DESCENDING", 1, ORTE_STRING);
		}
		if (ORTE_GPR_SYNCHRO_MODE_LEVEL & trig->flag.trig_synchro) {
		    orte_dps.pack(buffer, "\t\tORTE_GPR_SYNCHRO_MODE_LEVEL", 1, ORTE_STRING);
		}
		if (ORTE_GPR_SYNCHRO_MODE_GT_EQUAL & trig->flag.trig_synchro) {
		    orte_dps.pack(buffer, "\t\tORTE_GPR_SYNCHRO_MODE_GT_EQUAL", 1, ORTE_STRING);
		}
		if (ORTE_GPR_SYNCHRO_MODE_LT_EQUAL & trig->flag.trig_synchro) {
		    orte_dps.pack(buffer, "\t\tORTE_GPR_SYNCHRO_MODE_LT_EQUAL", 1, ORTE_STRING);
		}
		if (ORTE_GPR_SYNCHRO_MODE_CONTINUOUS & trig->flag.trig_synchro) {
		    orte_dps.pack(buffer, "\t\tORTE_GPR_SYNCHRO_MODE_CONTINUOUS", 1, ORTE_STRING);
		}
		if (ORTE_GPR_SYNCHRO_MODE_ONE_SHOT & trig->flag.trig_synchro) {
		    orte_dps.pack(buffer, "\t\tORTE_GPR_SYNCHRO_MODE_ONE_SHOT", 1, ORTE_STRING);
		}
		if (ORTE_GPR_SYNCHRO_MODE_STARTUP & trig->flag.trig_synchro) {
		    orte_dps.pack(buffer, "\t\tORTE_GPR_SYNCHRO_MODE_STARTUP", 1, ORTE_STRING);
		}
		asprintf(&tmp_out, "\tTrigger level: %d\tCurrent count: %d", trig->trigger, trig->count);
		orte_gpr_replica_dump_load_string(buffer, &tmp_out);
		asprintf(&tmp_out, "\tTransition status: %d", trig->above_below);
		orte_gpr_replica_dump_load_string(buffer, &tmp_out);
	}

    k = (int)orte_value_array_get_size(&(trig->tokentags));
    
    asprintf(&tmp_out, "\tAddressing mode: %X\tNumber of tokens: %d", trig->addr_mode, k);
    orte_gpr_replica_dump_load_string(buffer, &tmp_out);

    for (i=0; i < k; i++) {
        if (ORTE_SUCCESS == orte_gpr_replica_dict_reverse_lookup(&token, NULL,
                ORTE_VALUE_ARRAY_GET_ITEM(&(trig->tokentags), orte_gpr_replica_itag_t, i))) {
            asprintf(&tmp_out, "\t\tToken: %s", token);
		    orte_gpr_replica_dump_load_string(buffer, &tmp_out);
            free(token);
        }
	}

    k = (int)orte_value_array_get_size(&(trig->keytags));
    asprintf(&tmp_out, "\tNumber of keys: %d", k);
    orte_gpr_replica_dump_load_string(buffer, &tmp_out);

    for (i=0; i < k; i++) {
        if (ORTE_SUCCESS == orte_gpr_replica_dict_reverse_lookup(&token, NULL,
                ORTE_VALUE_ARRAY_GET_ITEM(&(trig->keytags), orte_gpr_replica_itag_t, i))) {
            asprintf(&tmp_out, "\t\tKey: %s", token);
            orte_gpr_replica_dump_load_string(buffer, &tmp_out);
            free(token);
        }
   }

	orte_dps.pack(buffer, "\n\n", 1, ORTE_STRING);

    return;
}


static void orte_gpr_replica_dump_itagval_value(orte_buffer_t *buffer,
                                                orte_gpr_replica_itagval_t *iptr)
{
    char *tmp;
    
    switch(iptr->type) {

        case ORTE_STRING:
            asprintf(&tmp, "\t\tData type: ORTE_STRING\tValue: %s\n", iptr->value.strptr);
            break;
            
        case ORTE_UINT8:
            asprintf(&tmp, "\t\tData type: ORTE_UINT8\tValue: %d\n", (int)iptr->value.ui8);
            break;
            
        case ORTE_UINT16:
            asprintf(&tmp, "\t\tData type: ORTE_UINT16\tValue: %d\n", (int)iptr->value.ui16);
            break;
            
        case ORTE_UINT32:
            asprintf(&tmp, "\t\tData type: ORTE_UINT32\tValue: %d\n", (int)iptr->value.ui32);
            break;
            
#ifdef HAVE_I64
        case ORTE_UINT64:
            asprintf(&tmp, "\t\tData type: ORTE_UINT64\tValue: %d\n", (int)iptr->value.ui64);
            break;
#endif

        case ORTE_INT8:
            asprintf(&tmp, "\t\tData type: ORTE_INT8\tValue: %d\n", (int)iptr->value.i8);
            break;
        
        case ORTE_INT16:
            asprintf(&tmp, "\t\tData type: ORTE_INT16\tValue: %d\n", (int)iptr->value.i16);
            break;
        
        case ORTE_INT32:
            asprintf(&tmp, "\t\tData type: ORTE_INT32\tValue: %d\n", (int)iptr->value.i32);
            break;
        
#ifdef HAVE_I64
        case ORTE_INT64:
            asprintf(&tmp, "\t\tData type: ORTE_INT64\tValue: %d\n", (int)iptr->value.i64);
            break;
#endif

        case ORTE_BYTE_OBJECT:
            asprintf(&tmp, "\t\tData type: ORTE_BYTE_OBJECT\tSize: %d\n", (int)(iptr->value.byteobject).size);
            break;
            
        case ORTE_NAME:
            asprintf(&tmp, "\t\tData type: ORTE_NAME\tValue: UNIMPLEMENTED\n");
            break;
            
        case ORTE_JOBID:
            asprintf(&tmp, "\t\tData type: ORTE_JOBID\tValue: UNIMPLEMENTED\n");
            break;
            
        case ORTE_NODE_STATE:
            asprintf(&tmp, "\t\tData type: ORTE_NODE_STATE\tValue: %d\n", (int)iptr->value.node_state);
            break;
            
        case ORTE_STATUS_KEY:
            asprintf(&tmp, "\t\tData type: ORTE_STATUS_KEY\tValue: %d\n", (int)iptr->value.proc_status);
            break;
            
        case ORTE_EXIT_CODE:
            asprintf(&tmp, "\t\tData type: ORTE_EXIT_CODE\tValue: %d\n", (int)iptr->value.exit_code);
            break;
            
        default:
            asprintf(&tmp, "\t\tData type: UNKNOWN\n");
            break;
    }
    
    orte_gpr_replica_dump_load_string(buffer, &tmp);
}


static void orte_gpr_replica_dump_load_string(orte_buffer_t *buffer, char **tmp)
{
    orte_dps.pack(buffer, tmp, 1, ORTE_STRING);
    free(*tmp);

}

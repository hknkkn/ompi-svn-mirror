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

#include "util/output.h"
#include "util/proc_info.h"

#include "mca/gpr/replica/transition_layer/gpr_replica_tl.h"

#include "gpr_replica_fn.h"

/*
 * Local typedef for storing a list of itagvals
 * - used exclusively by "get" routines
 */
typedef struct {
    ompi_list_item_t item;              /* required for this to be on list */
    orte_gpr_replica_itag_t itag;       /* itag for this value's key */
    orte_data_type_t type;              /* the type of value stored */
    orte_gpr_value_union_t value;
} orte_gpr_replica_get_list_t;

OBJ_CLASS_DECLARATION(orte_gpr_replica_get_list_t);

/* constructor */
static void orte_gpr_replica_get_list_constructor(orte_gpr_replica_get_list_t* ptr)
{
    ptr->itag = 0;
    ptr->type = 0;
    (ptr->value).strptr = NULL;
}

/* destructor - used to free any resources held by instance */
static void orte_gpr_replica_get_list_destructor(orte_gpr_replica_get_list_t* ptr)
{
    if (ORTE_BYTE_OBJECT == ptr->type) {
        free(((ptr->value).byteobject).bytes);
    }

}

/* define instance of orte_gpr_replica_segment_t */
OBJ_CLASS_INSTANCE(
          orte_gpr_replica_get_list_t,  /* type name */
          ompi_list_item_t, /* parent "class" name */
          orte_gpr_replica_get_list_constructor, /* constructor */
          orte_gpr_replica_get_list_destructor); /* destructor */


/*
 * FUNCTIONS
 */
 
int orte_gpr_replica_put_fn(orte_gpr_addr_mode_t addr_mode,
                            orte_gpr_replica_segment_t *seg,
                            orte_gpr_replica_itag_t *token_itags, int num_tokens,
                            int cnt, orte_gpr_keyval_t **keyvals,
                            int8_t *action_taken)
{
    orte_gpr_replica_itagval_t *iptr;
    orte_gpr_replica_container_t *cptr;
    orte_gpr_replica_itag_t itag;
    orte_gpr_keyval_t **kptr;
    bool overwrite;
    int rc, i;
    bool found;


    if (orte_gpr_replica_globals.debug) {
	    ompi_output(0, "[%d,%d,%d] gpr replica: put entered on segment %s",
		    ORTE_NAME_ARGS(*(orte_process_info.my_name)), seg->name);
    }

    /* initialize action */
    *action_taken = 0;
    
    /* all tokens are used
     * only overwrite permission mode flag has any affect
     */
    overwrite = false;
    if (addr_mode & ORTE_GPR_OVERWRITE) {
        overwrite = true;
    }

    /* find the specified container */
    found = false;
    cptr = (orte_gpr_replica_container_t*)((seg->containers)->addr);
    for (i=0; i < (seg->containers)->size && !found; i++) {
        if (NULL != cptr && orte_gpr_replica_check_itag_list(ORTE_GPR_XAND,
                                             num_tokens, token_itags,
                                             cptr->num_itags, cptr->itags)) {
            found = true;
            break;
        }
        cptr++;
    }
    
    if (!found) {  /* existing container not found - create one */
        if (ORTE_SUCCESS != (rc = orte_gpr_replica_create_container(&cptr, seg,
                                            num_tokens, token_itags))) {
            return rc;
        }
 
        /* ok, store all the keyvals in the container */
        kptr = keyvals;
        for (i=0; i < cnt; i++) {
            if (ORTE_SUCCESS != (rc = orte_gpr_replica_add_keyval(seg, cptr, &(kptr[i])))) {
                return rc;
            }
        }
        *action_taken = ORTE_GPR_REPLICA_ENTRY_ADDED;
    } else {  /* otherwise, see if entry already exists in container */
        kptr = keyvals;
        for (i=0; i < cnt; i++) {
            if (ORTE_SUCCESS == orte_gpr_replica_dict_lookup(&itag, seg, kptr[i]->key) &&
                orte_gpr_replica_search_container(&iptr, itag, cptr)) {
                /* this key already exists - overwrite, if permission given
                 * else error
                 */
                 if (overwrite) {
                    if (ORTE_SUCCESS != (rc = orte_gpr_replica_update_keyval(seg, iptr, (&kptr[i])))) {
                        return rc;
                    }
                 } else {
                    return ORTE_ERROR;
                 }
                 *action_taken = *action_taken | ORTE_GPR_REPLICA_ENTRY_UPDATED;
            } else { /* new key - add to container */
                if (ORTE_SUCCESS != (rc = orte_gpr_replica_add_keyval(seg, cptr, &(kptr[i])))) {
                    return rc;
                }
                *action_taken = *action_taken | ORTE_GPR_REPLICA_ENTRY_ADDED;
            }
        }
    }

    if (orte_gpr_replica_globals.debug) {
	    ompi_output(0, "[%d,%d,%d] gpr replica-put: complete", ORTE_NAME_ARGS(*(orte_process_info.my_name)));
    }

    return ORTE_SUCCESS;
}


int orte_gpr_replica_put_nb_fn(orte_gpr_addr_mode_t addr_mode,
                orte_gpr_replica_segment_t *seg,
                orte_gpr_replica_itag_t *token_itags, int num_tokens,
                int cnt, orte_gpr_keyval_t **keyvals,
                orte_gpr_notify_cb_fn_t cbfunc, void *user_tag)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}
                      

int orte_gpr_replica_get_fn(orte_gpr_addr_mode_t addr_mode,
                            orte_gpr_replica_segment_t *seg,
                            orte_gpr_replica_itag_t *tokentags, int num_tokens,
                            orte_gpr_replica_itag_t *keytags, int num_keys,
                            int *cnt, orte_gpr_value_t **values)
{
    ompi_list_t *list;
    orte_gpr_replica_get_list_t *ival_list;
    orte_gpr_replica_container_t **cptr;
    orte_gpr_replica_itagval_t *iptr;
    orte_gpr_keyval_t **kptr;
    int rc, i, j;
    
    if (orte_gpr_replica_globals.debug) {
        	ompi_output(0, "[%d,%d,%d] gpr replica: get entered", ORTE_NAME_ARGS(*(orte_process_info.my_name)));
    }

    /* initialize the list of findings */
    list = OBJ_NEW(ompi_list_t);
    *cnt = 0;
    *values = NULL;
    
    /* find all containers that meet search criteria */
    cptr = (orte_gpr_replica_container_t**)((seg->containers)->addr);
    for (i=0; i < (seg->containers)->size; i++) {
        if (NULL != cptr[i] && orte_gpr_replica_check_itag_list(addr_mode,
                                             num_tokens, tokentags,
                                             cptr[i]->num_itags, cptr[i]->itags)) {
            /* search container for matches and collect them onto list */
            for (j=0; j < num_keys; j++) {
                if (orte_gpr_replica_search_container(&iptr, keytags[j], cptr[i])) {
                    ival_list = OBJ_NEW(orte_gpr_replica_get_list_t);
                    
                    ival_list->type = iptr->type;
                    if (ORTE_SUCCESS != (rc = orte_gpr_replica_xfer_payload(
                                &(ival_list->value), &(iptr->value), iptr->type))) {
                        OBJ_RELEASE(ival_list);
                        return rc;
                    }
                    ompi_list_append(list, &ival_list->item);
                    (*cnt)++;
                }
            }
        }
    }
    
    if (0 == *cnt) {  /* nothing found - report that */
        OBJ_RELEASE(list);
        return ORTE_ERR_NOT_FOUND;
    }
    
    /* if something on the list, convert it to array of values */
    *values = OBJ_NEW(orte_gpr_value_t);
    if (NULL == *values) {
        OBJ_RELEASE(list);
        *cnt = 0;
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    (*values)->segment = strdup(seg->name);
    (*values)->cnt = *cnt;
    (*values)->keyvals = (orte_gpr_keyval_t**)malloc((*cnt) * sizeof(orte_gpr_keyval_t*));
    if (NULL == (*values)->keyvals) {
        OBJ_RELEASE(list);
        OBJ_RELEASE(*values);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    kptr = (*values)->keyvals;
    for (i=0; i < *cnt; i++) {
        kptr[i] = OBJ_NEW(orte_gpr_keyval_t);
        if (NULL == kptr[i]) {
            OBJ_RELEASE(list);
            OBJ_RELEASE(*values);
            return ORTE_ERR_OUT_OF_RESOURCE;
        }
        ival_list = (orte_gpr_replica_get_list_t*)ompi_list_remove_first(list);
        if (ORTE_SUCCESS != (rc = orte_gpr_replica_dict_reverse_lookup(
                                    &(kptr[i]->key), seg, ival_list->itag))) {
            OBJ_RELEASE(ival_list);
            OBJ_RELEASE(list);
            OBJ_RELEASE(*values);
            return rc;
        }
        kptr[i]->type = ival_list->type;
        if (ORTE_SUCCESS != (rc = orte_gpr_replica_xfer_payload(
                    &(kptr[i]->value), &(ival_list->value), ival_list->type))) {
            OBJ_RELEASE(ival_list);
            OBJ_RELEASE(list);
            OBJ_RELEASE(*values);
            return rc;
        }
        OBJ_RELEASE(ival_list);
    }
    
    if (orte_gpr_replica_globals.debug) {
	ompi_output(0, "[%d,%d,%d] gpr replica-get: finished search", ORTE_NAME_ARGS(*(orte_process_info.my_name)));
    }

    return ORTE_SUCCESS;
}

int orte_gpr_replica_get_nb_fn(orte_gpr_addr_mode_t addr_mode,
                                orte_gpr_replica_segment_t *seg,
                                orte_gpr_replica_itag_t *tokentags, int num_tokens,
                                orte_gpr_replica_itag_t *keytags, int num_keys,
                                orte_gpr_notify_cb_fn_t cbfunc, void *user_tag)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}


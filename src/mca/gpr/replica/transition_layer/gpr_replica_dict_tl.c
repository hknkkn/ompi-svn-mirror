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
 * The Open MPI general purpose registry - support functions.
 *
 */

/*
 * includes
 */

#include "ompi_config.h"

#include "gpr_replica.h"
#include "gpr_replica_internals.h"

mca_gpr_replica_dict_t
*mca_gpr_replica_find_dict_entry(mca_gpr_replica_segment_t *seg, char *token)
{
    mca_gpr_replica_dict_t *ptr_key;


    if (NULL == token) { /* just want segment token-key pair */
	/* search the global-level dict to find entry */
	for (ptr_key = (mca_gpr_replica_keytable_t*)ompi_list_get_first(&mca_gpr_replica_head.segment_dict);
	     ptr_key != (mca_gpr_replica_keytable_t*)ompi_list_get_end(&mca_gpr_replica_head.segment_dict);
	     ptr_key = (mca_gpr_replica_keytable_t*)ompi_list_get_next(ptr_key)) {
	    if (seg->key == ptr_key->key) {
		return(ptr_key);
	    }
	}
	return NULL; /* couldn't find the entry */
    }

    /* want specified token-key pair in that segment's dictionary */
    for (ptr_key = (mca_gpr_replica_keytable_t*)ompi_list_get_first(&seg->keytable);
	 ptr_key != (mca_gpr_replica_keytable_t*)ompi_list_get_end(&seg->keytable);
	 ptr_key = (mca_gpr_replica_keytable_t*)ompi_list_get_next(ptr_key)) {
	if (0 == strcmp(token, ptr_key->token)) {
	    return(ptr_key);
	}
    }
    return(NULL); /* couldn't find the specified entry */
}


mca_gpr_replica_key_t mca_gpr_replica_get_key(mca_gpr_replica_segment_t *seg, char *token)
{
    mca_gpr_replica_keytable_t *ptr_key;

    /* find the dictionary entry that matches token */
    ptr_key = mca_gpr_replica_find_dict_entry(seg, token);
    if (NULL != ptr_key) {
	return(ptr_key->key);
    }
    return MCA_GPR_REPLICA_KEY_MAX; /* couldn't find dictionary entry */
}


char *mca_gpr_replica_get_token(mca_gpr_replica_segment_t *seg, mca_gpr_replica_key_t key)
{
    mca_gpr_replica_keytable_t *ptr_key;
    char *answer;

    if (NULL == seg) {
	/* want to find a matching token for a segment name */
	for (ptr_key = (mca_gpr_replica_keytable_t*)ompi_list_get_first(&mca_gpr_replica_head.segment_dict);
	     ptr_key != (mca_gpr_replica_keytable_t*)ompi_list_get_end(&mca_gpr_replica_head.segment_dict);
	     ptr_key = (mca_gpr_replica_keytable_t*)ompi_list_get_next(ptr_key)) {
	    if (key == ptr_key->key) {
		answer = strdup(ptr_key->token);
		return answer;
	    }
	}
	return NULL;  /* couldn't find the specified entry */
    }

    /* find the matching key */
    for (ptr_key = (mca_gpr_replica_keytable_t*)ompi_list_get_first(&seg->keytable);
	 ptr_key != (mca_gpr_replica_keytable_t*)ompi_list_get_end(&seg->keytable);
	 ptr_key = (mca_gpr_replica_keytable_t*)ompi_list_get_next(ptr_key)) {
	if (key == ptr_key->key) {
	    answer = strdup(ptr_key->token);
	    return answer;
	}
    }
    return(NULL); /* couldn't find the specified entry */
}

mca_gpr_replica_key_t
*mca_gpr_replica_get_key_list(mca_gpr_replica_segment_t *seg,
			      char **tokens, int *num_tokens)
{
    char **tokptr;
    mca_gpr_replica_key_t *keys, *key2;
    int num_keys;

    *num_tokens = 0;

    /* check for wild-card case */
    if (NULL == tokens) {
	return NULL;
    }

    tokptr = tokens;
    num_keys = 0;
    while (NULL != *tokptr) {
	num_keys++;
	tokptr++;
    }

    keys = (mca_gpr_replica_key_t*)malloc(num_keys*sizeof(mca_gpr_replica_key_t));
    key2 = keys;
    *num_tokens = num_keys;

    tokptr = tokens;

    while (NULL != *tokptr) {  /* traverse array of tokens until NULL */
	*key2 = mca_gpr_replica_get_key(seg, *tokptr);
	if (MCA_GPR_REPLICA_KEY_MAX == *key2) {
	    *key2 = mca_gpr_replica_define_key(seg, *tokptr);
	}
	tokptr++; key2++;
    }
    return keys;
}

mca_gpr_replica_key_t
mca_gpr_replica_define_key(mca_gpr_replica_segment_t *seg, char *token)
{
    mca_gpr_replica_keytable_t *ptr_key, *new;

    /* if token is NULL, error */
    if (NULL == token) {
	return MCA_GPR_REPLICA_KEY_MAX;
    }

    /* if seg is NULL, use token to define new segment name in global dictionary */
    if (NULL == seg) {
	for (ptr_key = (mca_gpr_replica_keytable_t*)ompi_list_get_first(&mca_gpr_replica_head.segment_dict);
	     ptr_key != (mca_gpr_replica_keytable_t*)ompi_list_get_end(&mca_gpr_replica_head.segment_dict);
	     ptr_key = (mca_gpr_replica_keytable_t*)ompi_list_get_next(ptr_key)) {
	    if (0 == strcmp(token, ptr_key->token)) {
		return ptr_key->key; /* already taken, report value */
	    }
	}
	/* okay, token is unique - create dictionary entry */
	new = OBJ_NEW(mca_gpr_replica_keytable_t);
	new->token = strdup(token);
	if (0 == ompi_list_get_size(&mca_gpr_replica_head.freekeys)) { /* no keys waiting for reuse */
	    mca_gpr_replica_head.lastkey++;
	    new->key = mca_gpr_replica_head.lastkey;
	} else {
	    ptr_key = (mca_gpr_replica_keytable_t*)ompi_list_remove_first(&mca_gpr_replica_head.freekeys);
	    new->key = ptr_key->key;
	}
	ompi_list_append(&mca_gpr_replica_head.segment_dict, &new->item);
	return new->key;
    }

    /* check seg's dictionary to ensure uniqueness */
    for (ptr_key = (mca_gpr_replica_keytable_t*)ompi_list_get_first(&seg->keytable);
	 ptr_key != (mca_gpr_replica_keytable_t*)ompi_list_get_end(&seg->keytable);
	 ptr_key = (mca_gpr_replica_keytable_t*)ompi_list_get_next(ptr_key)) {
	if (0 == strcmp(token, ptr_key->token)) {
	    return ptr_key->key; /* already taken, report value */
	}
    }

    /* okay, token is unique - create dictionary entry */
    new = OBJ_NEW(mca_gpr_replica_keytable_t);
    new->token = strdup(token);
    if (0 == ompi_list_get_size(&seg->freekeys)) { /* no keys waiting for reuse */
	seg->lastkey++;
	new->key = seg->lastkey;
    } else {
	ptr_key = (mca_gpr_replica_keytable_t*)ompi_list_remove_first(&seg->freekeys);
	new->key = ptr_key->key;
    }
    ompi_list_append(&seg->keytable, &new->item);
    return new->key;
}


int mca_gpr_replica_delete_key(mca_gpr_replica_segment_t *seg, char *token)
{
    mca_gpr_replica_core_t *reg;
    mca_gpr_replica_keytable_t *ptr_seg, *ptr_key, *new;
    mca_gpr_replica_key_t *key;
    uint i;

    if (NULL == token) {
	/* remove the dictionary entry from the global registry dictionary*/
	ptr_seg = mca_gpr_replica_find_dict_entry(seg, NULL);
	if (NULL == ptr_seg) { /* failed to find dictionary entry */
	    return OMPI_ERROR;
	}

	/* add key to global registry's freekey list */
	new = OBJ_NEW(mca_gpr_replica_keytable_t);
	new->token = NULL;
	new->key = ptr_seg->key;
	ompi_list_append(&mca_gpr_replica_head.freekeys, &new->item);

	/* remove the dictionary entry */
	ompi_list_remove_item(&mca_gpr_replica_head.segment_dict, &ptr_seg->item);


	return(OMPI_SUCCESS);

    }

    /* token not null, so need to find dictionary element to delete */
    ptr_key = mca_gpr_replica_find_dict_entry(seg, token);
    if (NULL != ptr_key) {
	/* found key in dictionary */
	/* need to search this segment's registry to find all instances of key & "delete" them */
	for (reg = (mca_gpr_replica_core_t*)ompi_list_get_first(&seg->registry_entries);
	     reg != (mca_gpr_replica_core_t*)ompi_list_get_end(&seg->registry_entries);
	     reg = (mca_gpr_replica_core_t*)ompi_list_get_next(reg)) {

	    /* check the key list */
	    for (i=0, key=reg->keys; i < reg->num_keys; i++, key++) {
		if (ptr_key->key == *key) {  /* found match */
		    *key = MCA_GPR_REPLICA_KEY_MAX;
		}
	    }

	    /* add key to this segment's freekey list */
	    new = OBJ_NEW(mca_gpr_replica_keytable_t);
	    new->token = NULL;
	    new->key = ptr_key->key;
	    ompi_list_append(&seg->freekeys, &new->item);

	    /* now remove the dictionary entry from the segment's dictionary */
	    ompi_list_remove_item(&seg->keytable, &ptr_key->item);
	    return(OMPI_SUCCESS);
	}
    }
    return(OMPI_ERROR); /* if we get here, then we couldn't find token in dictionary */
}


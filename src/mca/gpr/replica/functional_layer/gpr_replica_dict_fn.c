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

#include "orte_config.h"

#include "gpr_replica_fn.h"

/*
 * A mode of "NONE" or "OVERWRITE" defaults to "XAND" behavior
 */
bool orte_gpr_replica_check_itag_list(orte_gpr_addr_mode_t addr_mode,
                    orte_gpr_replica_itag_t num_itags_search,
                    orte_gpr_replica_itag_t *itags,
                    orte_gpr_replica_itag_t num_itags_entry,
                    orte_gpr_replica_itag_t *entry_itags)
{
    orte_gpr_replica_itag_t *itag1, *itag2;
    int num_found;
    bool exclusive, no_match;
    int i, j;

    /* check for trivial case */
    if (NULL == itags) {  /* wildcard case - automatically true */
	   return true;
    }

    /* take care of trivial cases that don't require search */
    if ((ORTE_GPR_XAND & addr_mode) &&
	    (num_itags_search != num_itags_entry)) { /* can't possibly turn out "true" */
	    return false;
    }

    if ((ORTE_GPR_AND & addr_mode) &&
	    (num_itags_search > num_itags_entry)) {  /* can't find enough matches */
	    return false;
    }

    /* okay, have to search for remaining possibilities */
    num_found = 0;
    exclusive = true;
    for (i=0, itag1=entry_itags; i < num_itags_entry; i++, itag1++) {
        	no_match = true;
        	for (j=0, itag2=itags; j < num_itags_search; j++, itag2++) {
        	    if (*itag1 == *itag2) { /* found a match */
            		num_found++;
            		no_match = false;
            		if (ORTE_GPR_OR & addr_mode) { /* only need one match */
            		    return true;
            		}
        	    }
        	}
        	if (no_match) {
        	    exclusive = false;
        	}
    }

    if (ORTE_GPR_XAND & addr_mode) {  /* deal with XAND case */
        	if (num_found == num_itags_entry) { /* found all, and nothing more */
        	    return true;
        	} else {  /* found either too many or not enough */
        	    return false;
        	}
    }

    if (ORTE_GPR_XOR & addr_mode) {  /* deal with XOR case */
        	if (num_found > 0 && exclusive) {  /* found at least one and nothing not on list */
        	    return true;
        	} else {
        	    return false;
        	}
    }

    if (ORTE_GPR_AND & addr_mode) {  /* deal with AND case */
        	if (num_found == num_itags_search) {  /* found all the required keys */
        	    return true;
        	} else {
        	    return false;
        	}
    }

    /* should be impossible situation, but just to be safe... */
    return false;
}


int orte_gpr_replica_copy_itag_list(orte_gpr_replica_itag_t **dest,
                                    orte_gpr_replica_itag_t *src, int num_itags)
{
    if (0 == num_itags || NULL == src) {
        return ORTE_ERR_BAD_PARAM;
    }
    
    *dest = (orte_gpr_replica_itag_t*)malloc(num_itags * sizeof(orte_gpr_replica_itag_t));
    if (NULL == *dest) {
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    memcpy(*dest, src, num_itags);
    return ORTE_SUCCESS;
}


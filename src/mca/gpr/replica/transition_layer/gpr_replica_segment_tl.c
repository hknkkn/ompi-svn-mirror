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
#include <stdio.h>
#include "gpr_replica_tl.h"

int orte_gpr_replica_find_seg(orte_gpr_replica_segment_t **seg,
                              bool create, char *segment)
{
    size_t len;
    int rc, i;

    fprintf(stderr, "entered find seg\n");
    
    /* initialize to nothing */
    *seg = NULL;
    
    len = strlen(segment);
    
    fprintf(stderr, "got segment name length %d\n", len);
    
    /* search the registry segments to find which one is being referenced */
    *seg = (orte_gpr_replica_segment_t*)(orte_gpr_replica.segments->addr);
    for (i=0; i < (orte_gpr_replica.segments)->size; i++) {
        if (NULL != *seg) {
            if (0 == strncmp(segment, (*seg)->name, len)) {
                return ORTE_SUCCESS;
            }
        }
        (*seg)++;
    }
    
    if (!create) {
        /* couldn't find it and don't want it created - just return NULL */
        return ORTE_ERR_BAD_PARAM;
    }
    
    /* add the segment to the registry */
    *seg = OBJ_NEW(orte_gpr_replica_segment_t);
    (*seg)->name = strdup(segment);
    if (0 > (rc = orte_pointer_array_add(orte_gpr_replica.segments, (void*)(*seg)))) {
        OBJ_RELEASE(*seg);
        return rc;
    }
    (*seg)->itag = rc;
    return ORTE_SUCCESS;
}

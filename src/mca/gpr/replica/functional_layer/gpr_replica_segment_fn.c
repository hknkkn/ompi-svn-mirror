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


int orte_gpr_replica_release_segment(orte_gpr_replica_segment_t *seg)
{
    int rc;
    
    if (0 > (rc = orte_pointer_array_set_item(orte_gpr_replica.segments, seg->itag, NULL))) {
        return rc;
    }
    OBJ_RELEASE(seg);
    
    return ORTE_SUCCESS;
}

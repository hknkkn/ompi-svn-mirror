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

#include "ompi_config.h"

#include <unistd.h>

#include "dps_internal.h"

/**
 * DPS_BUFFER_INIT
 */

int orte_dps_buffer_init (orte_buffer_t **buffer, char *label)
{
    orte_buffer_t *bptr;
    
    size_t defaultinitsize = getpagesize(); /* should check the mca params here */

    /* check that we can return a buffer atall.. */
    if (!buffer) { return (ORTE_ERROR); }

    /* create new buffer object */
    bptr = OBJ_NEW (orte_buffer_t);

    if (!bptr) { return (ORTE_ERROR); }

    /* we have a buffer now, so lets populate it */

    /* if user provided a label, save it */
    if (NULL != label) {
        bptr->label = strdup(label);
    }
    
    bptr->base_ptr = (void*) malloc (defaultinitsize); 
    if (!bptr->base_ptr) { /* couldn't get data */
        OBJ_RELEASE (bptr);
        bptr = NULL;  /* make sure the ptr indicates free */
        return (ORTE_ERROR);
    }

    bptr->data_ptr = bptr->base_ptr; /* set the start of the buffer */
    bptr->from_ptr = bptr->base_ptr; /* set the unpack start at start */

    /* set counts for size and space */
    bptr->pages = 1;
    bptr->size = bptr->space = defaultinitsize;

    /* install the buffer address */
    *buffer = bptr;

    return (ORTE_SUCCESS);  
}



/**
 * DPS_BUFFER_FREE
 */
int orte_dps_buffer_free(orte_buffer_t **buffer)
{
    /* all internal memory storage is free'd by release call */
    OBJ_RELEASE(*buffer);
    
    /* NULL the pointer value to indicate it has been free'd */
    buffer = NULL;
    
    return ORTE_SUCCESS;
}

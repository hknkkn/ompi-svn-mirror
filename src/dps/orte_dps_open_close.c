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
 */
#include "ompi_config.h"

#include "dps_internal.h"

/**
 * globals
 */
bool orte_dps_debug;

orte_dps_t orte_dps = {
    orte_dps_pack,
    orte_dps_unpack,
    orte_dps_peek
};

/**
 * Object constructors, destructors, and instantiations
 */
static void orte_buffer_construct (orte_buffer_t* buffer)
{
    buffer->base_ptr = buffer->data_ptr = buffer->from_ptr = NULL;
    buffer->pages = buffer->size = buffer->len = buffer-> space = buffer-> toend = 0;
}

static void orte_buffer_destruct (orte_buffer_t* buffer)
{
    /* paranoid check */
    if (NULL != buffer) {
        if (NULL != buffer->base_ptr) free (buffer->base_ptr);
    }
}
OBJ_CLASS_INSTANCE(orte_buffer_t,
                   ompi_object_t,
                   orte_buffer_construct,
                   orte_buffer_destruct);


int orte_dps_open(void)
{
    char *enviro_val;
    
    enviro_val = getenv("ORTE_dps_debug");
    if (NULL != enviro_val) {  /* debug requested */
        orte_dps_debug = true;
    } else {
        orte_dps_debug = false;
    }
    
    return ORTE_SUCCESS;
}

int orte_dps_close(void)
{
    /* no idea what this would do right now - include it for completeness */
    return ORTE_SUCCESS;
}

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
    orte_dps_buffer_init,
    orte_dps_pack_value,
    orte_dps_unpack_value,
    orte_dps_pack_object,
    orte_dps_unpack_object,
    orte_dps_buffer_free
};

/**
 * Object constructors, destructors, and instantiations
 */
static void orte_buffer_construct (orte_buffer_t* buffer)
{
    buffer->base_ptr = buffer->data_ptr = buffer->from_ptr = buffer->label = NULL;
    buffer->pages = buffer->size = buffer->len = buffer-> space = buffer-> toend = 0;
}

static void orte_buffer_destruct (orte_buffer_t* buffer)
{
    /* paranoid check */
    if (NULL != buffer) {
        if (NULL != buffer->base_ptr) free (buffer->base_ptr);

        if (NULL != buffer->label) free(buffer->label);
    }
}
OBJ_CLASS_INSTANCE(orte_buffer_t,
                   ompi_object_t,
                   orte_buffer_construct,
                   orte_buffer_destruct);


/**
 * Object constructors, destructors, and instantiations
 */
static void orte_byte_object_construct (orte_byte_object_t* ptr)
{
    ptr->size = 0;
    ptr->bytes = NULL;
}

static void orte_byte_object_destruct (orte_byte_object_t* ptr)
{
    /* paranoid check */
    if (NULL != ptr) {
        if (NULL != ptr->bytes) free (ptr->bytes);
    }

}
OBJ_CLASS_INSTANCE(orte_byte_object_t,
                   ompi_object_t,
                   orte_byte_object_construct,
                   orte_byte_object_destruct);


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

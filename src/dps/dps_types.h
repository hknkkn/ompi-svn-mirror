/* -*- C -*-
 *
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
/**
 * @file
 *
 * Resource Discovery & Allocation Subsystem (RDAS)
 *
 * The RDAS is responsible for discovering the resources available to the universe, and
 * for allocating them to the requesting job.
 *
 */

#ifndef ORTE_DPS_TYPES_H_
#define ORTE_DPS_TYPES_H_

#include "orte_config.h"

#include "class/ompi_object.h"

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

/**
 * Supported datatypes for conversion operations.
 * NOTE, these have (or should) a one to one match to ompi_pack_type_t
 *
 */

typedef uint8_t orte_pack_type_t;

#define ORTE_BYTE_OBJECT               (orte_pack_type_t)  0x01     /**< a byte of data */
#define ORTE_INT8                      (orte_pack_type_t)  0x02     /**< an 8-bit integer */
#define ORTE_INT16                     (orte_pack_type_t)  0x03     /**< a 16 bit integer */
#define ORTE_INT32                     (orte_pack_type_t)  0x04     /**< a 32 bit integer */
#define ORTE_STRING                    (orte_pack_type_t)  0x05     /**< a NULL terminated string */
#define ORTE_NAME                      (orte_pack_type_t)  0x06     /**< an ompi_process_name_t */
#define ORTE_JOBID                     (orte_pack_type_t)  0x07     /**< a jobid */
#define ORTE_CELLID                    (orte_pack_type_t)  0x08     /**< a cellid */
#define ORTE_NODE_STATE                (orte_pack_type_t)  0x09     /**< node status flag */
#define ORTE_PROCESS_STATUS            (orte_pack_type_t)  0x0A     /**< process status key */
#define ORTE_EXIT_CODE                 (orte_pack_type_t)  0x0B     /**< process exit code */

struct orte_byte_object_t {
     /* first member must be the objects parent */
    ompi_object_t parent;
    
    uint32_t size;
    uint8_t *bytes;
};
typedef struct orte_byte_object_t orte_byte_object_t;
/* formalise the declaration */
OMPI_DECLSPEC OBJ_CLASS_DECLARATION (orte_byte_object_t);


typedef struct orte_buffer_t {
    /* first member must be the objects parent */
    ompi_object_t parent;
    
    /* now for the real elements of the type */
    char *label;       /* provide a label for the user to id this buffer */

    void*   base_ptr;  /* start of my memory */
    void*   data_ptr;  /* location of where next data will go */
    void*   from_ptr;  /* location of where to get the next data from */

    /* counters */

    int32_t   pages;     /* number of pages of memory used (pages) */
    int32_t   size;      /* total size of this buffer (bytes) */
    int32_t   len;       /* total amount already packed (bytes) */
    int32_t   space;     /* how much space we have left (bytes) */
                         /* yep, size=len+space */

    int32_t   toend;     /* how many bytes till the end when unpacking :) */
                         /* yep, toend is the opposite of len */

} orte_buffer_t;

/* formalise the declaration */
OMPI_DECLSPEC OBJ_CLASS_DECLARATION (orte_buffer_t);

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif

#endif /* ORTE_DPS_TYPES_H */

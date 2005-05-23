/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University.
 *                         All rights reserved.
 * Copyright (c) 2004-2005 The Trustees of the University of Tennessee.
 *                         All rights reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */
/**
 * @file
 */
#ifndef MCA_PML_GEN2_ENDPOINT_H
#define MCA_PML_GEN2_ENDPOINT_H

#include "util/output.h"
#include "mca/bmi/bmi.h"
#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

/**
 * A data structure associated with a ompi_proc_t that caches
 * addressing/scheduling attributes for a specific NTL instance
 * that can be used to reach the process.
 */

struct mca_pml_ob1_endpoint_t {
    int    bmi_weight;                            /**< NTL weight for scheduling */
    size_t bmi_eager_limit;                       /**< NTL eager limit */
    struct mca_bmi_base_module_t *bmi;            /**< NTL module */
    struct mca_bmi_base_endpoint_t* bmi_endpoint; /**< NTL addressing info */
    struct mca_bmi_base_descriptor_t* bmi_cache;

    /* NTL function table */
    mca_bmi_base_module_alloc_fn_t bmi_alloc;
    mca_bmi_base_module_free_fn_t  bmi_free;
    mca_bmi_base_module_send_fn_t  bmi_send;
    mca_bmi_base_module_put_fn_t   bmi_put;
    mca_bmi_base_module_get_fn_t   bmi_get;
};
typedef struct mca_pml_ob1_endpoint_t mca_pml_ob1_endpoint_t;

/**
 * A dynamically growable array of mca_pml_ob1_endpoint_t instances.
 * Maintains an index into the array that is used for round-robin
 * scheduling across contents.
 */
struct mca_pml_ob1_ep_array_t {
    ompi_object_t super;
    size_t arr_size;    /**< number available */
    size_t arr_reserve; /**< size of allocated bmi_proc array */
    size_t arr_index;   /**< last used index*/
    mca_pml_ob1_endpoint_t* arr_endpoints;   /**< array of bmi endpoints */
};
typedef struct mca_pml_ob1_ep_array_t mca_pml_ob1_ep_array_t;


OMPI_DECLSPEC OBJ_CLASS_DECLARATION(mca_pml_ob1_ep_array_t);


/**
 * If required, reallocate (grow) the array to the indicate size.
 * 
 * @param array (IN)
 * @param size (IN)
 */
int mca_pml_ob1_ep_array_reserve(mca_pml_ob1_ep_array_t*, size_t);

static inline size_t mca_pml_ob1_ep_array_get_size(mca_pml_ob1_ep_array_t* array)
{
    return array->arr_size;
}

/**
 * Grow the array if required, and set the size.
 * 
 * @param array (IN)
 * @param size (IN)
 */
static inline void mca_pml_ob1_ep_array_set_size(mca_pml_ob1_ep_array_t* array, size_t size)
{
    if(array->arr_size > array->arr_reserve)
        mca_pml_ob1_ep_array_reserve(array, size);
    array->arr_size = size;
}

/**
 * Grow the array size by one and return the item at that index.
 * 
 * @param array (IN)
 */
static inline mca_pml_ob1_endpoint_t* mca_pml_ob1_ep_array_insert(mca_pml_ob1_ep_array_t* array)
{
#if OMPI_ENABLE_DEBUG
    if(array->arr_size >= array->arr_reserve) {
        ompi_output(0, "mca_pml_ob1_ep_array_insert: invalid array index %d >= %d", 
            array->arr_size, array->arr_reserve);
        return 0;
    }
#endif
    return &array->arr_endpoints[array->arr_size++];
}

/**
 * Return an array item at the specified index.
 * 
 * @param array (IN)
 * @param index (IN)
 */
static inline mca_pml_ob1_endpoint_t* mca_pml_ob1_ep_array_get_index(mca_pml_ob1_ep_array_t* array, size_t index)
{
#if OMPI_ENABLE_DEBUG
    if(index >= array->arr_size) {
        ompi_output(0, "mca_pml_ob1_ep_array_get_index: invalid array index %d >= %d",
            index, array->arr_size);
        return 0;
    }
#endif
    return &array->arr_endpoints[index];
}

/**
 * Return the next LRU index in the array.
 * 
 * @param array (IN)
 * @param index (IN)
 */
static inline mca_pml_ob1_endpoint_t* mca_pml_ob1_ep_array_get_next(mca_pml_ob1_ep_array_t* array)
{
    mca_pml_ob1_endpoint_t* endpoint;
#if OMPI_ENABLE_DEBUG
    if(array->arr_size == 0) {
        ompi_output(0, "mca_pml_ob1_ep_array_get_next: invalid array size");
        return 0;
    }
#endif
    endpoint = &array->arr_endpoints[array->arr_index++];
    if(array->arr_index == array->arr_size) {
        array->arr_index = 0;
    }
    return endpoint;
}

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif
#endif


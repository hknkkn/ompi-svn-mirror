/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
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

#include "ompi_config.h"
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif  /* HAVE_STRING_H */
#include "ompi/mca/mpool/mpool.h"
#include "base.h"
#include "mpool_base_tree.h"
#include "opal/threads/mutex.h" 
#include "mpool_base_mem_cb.h"

extern int mca_mpool_base_use_mem_hooks; 

/**
 * Memory Pool Registration
 */

static void mca_mpool_base_registration_constructor( mca_mpool_base_registration_t * reg )
{
    reg->mpool = NULL;
    reg->base = NULL;
    reg->bound = NULL;
    reg->ref_count = 0;
}

static void mca_mpool_base_registration_destructor( mca_mpool_base_registration_t * reg )
{
    
}

OBJ_CLASS_INSTANCE(
    mca_mpool_base_registration_t,
    ompi_free_list_item_t,
    mca_mpool_base_registration_constructor,
    mca_mpool_base_registration_destructor);

/**
 * Function to allocate special memory according to what the user requests in
 * the info object.
 *
 * If the user passes in a valid info structure then the function will
 * try to allocate the memory and register it with every mpool that there is a
 * key for it in the info struct. If it fails at registering the memory with 
 * one of the requested mpools, an error will be returned. Also, if there is a 
 * key in info that does not match any mpool, an error will be returned.
 *
 * If the info parameter is MPI_INFO_NULL, then this function will try to allocate
 * the memory and register it wih as many mpools as possible. However, 
 * if any of the registratons fail the mpool will simply be ignored.
 *
 * @param size the size of the memory area to allocate
 * @param info an info object which tells us what kind of memory to allocate
 *
 * @retval pointer to the allocated memory
 * @retval NULL on failure
 */
void * mca_mpool_base_alloc(size_t size, ompi_info_t * info)
{
    opal_list_item_t * item;
    int num_modules = opal_list_get_size(&mca_mpool_base_modules);
    int reg_module_num = 0;
    int i, j, num_keys;
    mca_mpool_base_selected_module_t * current;
    mca_mpool_base_selected_module_t * no_reg_function = NULL;
    mca_mpool_base_selected_module_t ** has_reg_function = NULL;
    mca_mpool_base_registration_t * registration;
    mca_mpool_base_tree_item_t* mpool_tree_item;
    
    void * mem = NULL;
    char * key;
    bool match_found;
    

    if (mca_mpool_base_use_mem_hooks && 
        0 != (OPAL_MEMORY_FREE_SUPPORT & opal_mem_hooks_support_level())) {
        /* if we're using memory hooks, it's possible (likely, based
           on testing) that for some tests the memory returned from
           any of the malloc functions below will be part of a larger
           (lazily) freed chunk and therefore already be pinned.
           Which causes our caches to get a little confused, as the
           alloc/free pair are supposed to always have an exact match
           in the rcache.  This wasn't happening, leading to badness.
           Instead, just malloc and we'll get to the pinning later,
           when we try to first use it.  Since we're leaving things
           pinned, there's no advantage to doing it now over first
           use, and it works if we wait ... */
        return malloc(size);
    }


    if (num_modules > 0) {
        has_reg_function = (mca_mpool_base_selected_module_t **)
                           malloc(num_modules * sizeof(mca_mpool_base_module_t *));
        if(!has_reg_function){ 
            return NULL;
        }
    }

    mpool_tree_item = mca_mpool_base_tree_item_get();
    
    if(NULL == mpool_tree_item){ 
        if(has_reg_function) { 
            free(has_reg_function);
        }
        return NULL;
    }
    
    if(&ompi_mpi_info_null == info)
    {
        for(item = opal_list_get_first(&mca_mpool_base_modules);
            item != opal_list_get_end(&mca_mpool_base_modules);
            item = opal_list_get_next(item)) {
            current = ((mca_mpool_base_selected_module_t *) item);
            if(current->mpool_module->flags & MCA_MPOOL_FLAGS_MPI_ALLOC_MEM) {
                if(NULL == current->mpool_module->mpool_register){
                    no_reg_function = current;
                }
                else {
                    has_reg_function[reg_module_num++] = current;
                }
            }
        }
    }
    else
    {
        ompi_info_get_nkeys(info, &num_keys);
        key = malloc(MPI_MAX_INFO_KEY + 1);
        for(i = 0; i < num_keys; i++)
        {
            match_found = false;
            ompi_info_get_nthkey(info, i, key);
            for(item = opal_list_get_first(&mca_mpool_base_modules);
                item != opal_list_get_end(&mca_mpool_base_modules);
                item = opal_list_get_next(item)) 
            {
                current = ((mca_mpool_base_selected_module_t *)item);
                if(0 == strcmp(key, 
                       current->mpool_module->mpool_component->mpool_version.mca_component_name))
                {
                    match_found = true;
                    if(NULL == current->mpool_module->mpool_register)
                    {
                        if(NULL != no_reg_function)
                        {
                           /* there was more than one requested mpool that lacks 
                            * a registration function, so return failure */
                            free(key);
                            if(has_reg_function) { 
                                free(has_reg_function);
                            }
                            return NULL;
                        }
                        no_reg_function = current;
                    }
                    else
                    {
                        has_reg_function[reg_module_num++] = current;
                    }
                }
            }
            if(!match_found)
            {
                /* one of the keys given to us by the user did not match any
                 * mpools, so return an error */
                free(key);
                if(has_reg_function) { 
                    free(has_reg_function);
                }
                return NULL;
            }
        }
        free(key);
    }
    
    if(NULL == no_reg_function && 0 == reg_module_num)
    {
        if(has_reg_function) { 
            free(has_reg_function);
        }
        if(&ompi_mpi_info_null == info)
        {
            /* if the info argument was NULL and there were no useable mpools,
             * just malloc the memory and return it */
            mem = malloc(size);
            if(NULL != mem){
                /* don't need the tree */
                mca_mpool_base_tree_item_put(mpool_tree_item);
                return mem;
            }
        }
        /* the user passed info but we were not able to use any of the mpools 
         * specified */
        return NULL;
    }
    
    
    i = j = 0;
    num_modules = 0;
    if(NULL != no_reg_function)
    {
        mca_mpool_base_module_t* mpool = no_reg_function->mpool_module;
        mem = mpool->mpool_alloc(mpool, size, 0, MCA_MPOOL_FLAGS_PERSIST, &registration);
        num_modules++;
        mpool_tree_item->key = mem;
        mpool_tree_item->mpools[j] = mpool;
        mpool_tree_item->regs[j++] = registration;
    }
    else
    {
        mca_mpool_base_module_t* mpool = has_reg_function[i]->mpool_module;
        mem = mpool->mpool_alloc(mpool, size, 0, MCA_MPOOL_FLAGS_PERSIST, &registration);
        i++;
        num_modules++;
        mpool_tree_item->key = mem;
        mpool_tree_item->mpools[j] = mpool;
        mpool_tree_item->regs[j++] = registration;
    }
    
    while(i < reg_module_num)
    {
        mca_mpool_base_module_t* mpool = has_reg_function[i]->mpool_module;
        if(OMPI_SUCCESS != mpool->mpool_register(mpool, mem, size, MCA_MPOOL_FLAGS_PERSIST,  &registration))
        {
            if (has_reg_function) {
                free(has_reg_function);
            }
            return NULL;
        } else { 
            mpool_tree_item->mpools[j] = mpool;
            mpool_tree_item->regs[j++] = registration;
            num_modules++;
        }
        i++;
    }
    if(has_reg_function) { 
        free(has_reg_function);
    }
    
    /* null terminated array */
    mpool_tree_item->mpools[j] = NULL;
    mpool_tree_item->regs[j] = NULL;

    mca_mpool_base_tree_insert(mpool_tree_item);
    
    return mem;
}

/**
 * Function to free memory previously allocated by mca_mpool_base_alloc
 *
 * @param base pointer to the memory to free
 *
 * @retval OMPI_SUCCESS
 * @retval OMPI_ERR_BAD_PARAM if the passed base pointer was invalid
 */
int mca_mpool_base_free(void * base)
{
    int i = 0, rc = OMPI_SUCCESS;
    mca_mpool_base_tree_item_t* mpool_tree_item = NULL;
    mca_mpool_base_module_t* mpool;
    mca_mpool_base_registration_t* reg;
    
    if(!base) { 
        return OMPI_ERROR;
    }

    /* see comment in alloc function above */
    if (mca_mpool_base_use_mem_hooks && 
        0 != (OPAL_MEMORY_FREE_SUPPORT & opal_mem_hooks_support_level())) {
        free(base);
        return OMPI_SUCCESS;
    }

    mpool_tree_item = mca_mpool_base_tree_find(base); 
    
    if(!mpool_tree_item) { 
        /* nothing in the tree this was just 
           plain old malloc'd memory */ 
        free(base);
        return OMPI_SUCCESS;
    }
    
    for(i = 1; i < MCA_MPOOL_BASE_TREE_MAX; i++) {
        mpool = mpool_tree_item->mpools[i];
        reg = mpool_tree_item->regs[i];
        if(mpool) { 
            mpool->mpool_deregister(mpool, reg); 
        } else { 
            break;
        }
    }
    
    mpool = mpool_tree_item->mpools[0];
    reg =  mpool_tree_item->regs[0];
    mpool->mpool_free(mpool, base, reg);
    
    rc = mca_mpool_base_tree_delete(mpool_tree_item);
    
    return rc;
}


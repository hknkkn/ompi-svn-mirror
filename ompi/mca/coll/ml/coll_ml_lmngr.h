/*
 * Copyright (c) 2009-2012 Oak Ridge National Laboratory.  All rights reserved.
 * Copyright (c) 2009-2012 Mellanox Technologies.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef MCA_ML_LMNGR_H
#define MCA_ML_LMNGR_H

#include "ompi_config.h"
#include "opal/class/opal_list.h"
#include "ompi/mca/bcol/bcol.h"

#define MCA_COLL_ML_MAX_REG_INFO 32 

/* LMNGR - List manager for registred memory */
struct mca_coll_ml_lmngr_t {
    opal_object_t super;
    /* lock to control list access */
    opal_mutex_t mem_lock;

    /* list of memory chunks */
    opal_list_t blocks_list;

    /* base (allocated) address of the memory pool */
    void* base_addr;

    /* size of memory chunks */
    size_t list_block_size;

    /* memory chunk alignment */
    size_t list_alignment;

    /* init list size */
    size_t list_size;

    /* number network context of resources 
       In other words, number of different registration
       functions that will be used. For example in case
       of iboffload for each device (PD) we will have
       different entry
       */
   int n_resources;

   /* registration descriptor */
   void * reg_desc[MCA_COLL_ML_MAX_REG_INFO];

   /* bcol network context array */
   struct bcol_base_network_context_t * net_context[MCA_COLL_ML_MAX_REG_INFO];
};
typedef struct mca_coll_ml_lmngr_t mca_coll_ml_lmngr_t;
OBJ_CLASS_DECLARATION(mca_coll_ml_lmngr_t);

struct mca_coll_ml_lmngr_block_t {
    opal_list_item_t super;
    mca_coll_ml_lmngr_t *lmngr;
    void* base_addr;
};
typedef struct mca_coll_ml_lmngr_block_t mca_coll_ml_lmngr_block_t;
OBJ_CLASS_DECLARATION(mca_coll_ml_lmngr_block_t);

/* read user defined parametres for list manager */
int mca_coll_ml_lmngr_reg(void);
/* If programmer want to user other than default mca
parametres, he can use the tune function. The tune
function must be run before list initialization,
otherway error will be returned */
int mca_coll_ml_lmngr_tune(mca_coll_ml_lmngr_t *lmngr, 
        size_t block_size, size_t list_size, size_t alignment);

/* Append new network context to the existing list memory manager */
int mca_coll_ml_lmngr_append_nc(mca_coll_ml_lmngr_t *lmngr, bcol_base_network_context_t *nc);

/* Allocate a block from memory list manager */
mca_coll_ml_lmngr_block_t* mca_coll_ml_lmngr_alloc (
        mca_coll_ml_lmngr_t *lmngr);

/* Return block to list memory manager */
void mca_coll_ml_lmngr_free (mca_coll_ml_lmngr_block_t *block);

#endif

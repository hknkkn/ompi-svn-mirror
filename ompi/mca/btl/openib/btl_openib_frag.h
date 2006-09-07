/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2006 The University of Tennessee and The University
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

#ifndef MCA_BTL_IB_FRAG_H
#define MCA_BTL_IB_FRAG_H

#include "ompi_config.h"

#include <infiniband/verbs.h> 
#include "ompi/mca/mpool/openib/mpool_openib.h" 
#include "ompi/mca/btl/btl.h"

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

struct mca_btl_openib_header_t {
    mca_btl_base_tag_t tag;
    uint16_t credits;
};
typedef struct mca_btl_openib_header_t mca_btl_openib_header_t;
#define BTL_OPENIB_RDMA_CREDITS_FLAG (1<<15)
#define BTL_OPENIB_IS_RDMA_CREDITS(I) ((I)&BTL_OPENIB_RDMA_CREDITS_FLAG)
#define BTL_OPENIB_CREDITS(I) ((I)&~BTL_OPENIB_RDMA_CREDITS_FLAG)

struct mca_btl_openib_footer_t {
#if OMPI_ENABLE_DEBUG
    uint32_t seq;
#endif
    union {
        uint32_t size;
        uint8_t buf[4];
    } u;
};
typedef struct mca_btl_openib_footer_t mca_btl_openib_footer_t;

typedef enum {
    MCA_BTL_OPENIB_CONTROL_CREDITS,
    MCA_BTL_OPENIB_CONTROL_RDMA
} mca_btl_openib_control_t;

struct mca_btl_openib_control_header_t {
    mca_btl_openib_control_t type;
};
typedef struct mca_btl_openib_control_header_t mca_btl_openib_control_header_t;

struct mca_btl_openib_eager_rdma_header_t {
	mca_btl_openib_control_header_t control;
	ompi_ptr_t rdma_start;
	uint32_t rkey;
};
typedef struct mca_btl_openib_eager_rdma_header_t mca_btl_openib_eager_rdma_header_t;

struct mca_btl_openib_rdma_credits_header_t {
    mca_btl_openib_control_header_t control;
    uint16_t rdma_credits;
};
typedef struct mca_btl_openib_rdma_credits_header_t mca_btl_openib_rdma_credits_header_t;

enum mca_btl_openib_frag_type_t {
    MCA_BTL_OPENIB_FRAG_EAGER,
    MCA_BTL_OPENIB_FRAG_MAX,
    MCA_BTL_OPENIB_FRAG_FRAG,
    MCA_BTL_OPENIB_FRAG_EAGER_RDMA,
    MCA_BTL_OPENIB_FRAG_CONTROL
};
typedef enum mca_btl_openib_frag_type_t mca_btl_openib_frag_type_t;

/**
 * IB send fragment derived type.
 */
struct mca_btl_openib_frag_t {
    mca_btl_base_descriptor_t base; 
    struct mca_btl_base_endpoint_t *endpoint; 
    mca_btl_openib_footer_t *ftr;
    mca_btl_openib_header_t *hdr;
    mca_btl_base_segment_t segment; 
    size_t size; 
    int rc;
    mca_btl_openib_frag_type_t type; 
    union{ 
        struct ibv_recv_wr rd_desc; 
        struct ibv_send_wr sr_desc; 
    } wr_desc;  
    struct ibv_sge sg_entry;  
    struct ibv_mr *mr; 
    mca_mpool_openib_registration_t * openib_reg; 
}; 
typedef struct mca_btl_openib_frag_t mca_btl_openib_frag_t; 
OBJ_CLASS_DECLARATION(mca_btl_openib_frag_t);

typedef struct mca_btl_openib_frag_t mca_btl_openib_send_frag_eager_t; 
    
OBJ_CLASS_DECLARATION(mca_btl_openib_send_frag_eager_t); 

typedef struct mca_btl_openib_frag_t mca_btl_openib_send_frag_max_t; 
    
OBJ_CLASS_DECLARATION(mca_btl_openib_send_frag_max_t); 

typedef struct mca_btl_openib_frag_t mca_btl_openib_send_frag_frag_t; 
    
OBJ_CLASS_DECLARATION(mca_btl_openib_send_frag_frag_t); 

typedef struct mca_btl_openib_frag_t mca_btl_openib_recv_frag_eager_t; 
    
OBJ_CLASS_DECLARATION(mca_btl_openib_recv_frag_eager_t); 

typedef struct mca_btl_openib_frag_t mca_btl_openib_recv_frag_max_t; 
    
OBJ_CLASS_DECLARATION(mca_btl_openib_recv_frag_max_t); 

typedef struct mca_btl_openib_frag_t mca_btl_openib_send_frag_control_t; 
    
OBJ_CLASS_DECLARATION(mca_btl_openib_send_frag_control_t); 

/*
 * Allocate an IB send descriptor
 *
 */

#define MCA_BTL_IB_FRAG_ALLOC_CREDIT_WAIT(btl, frag, rc)               \
{                                                                      \
                                                                       \
    ompi_free_list_item_t *item;                                       \
    OMPI_FREE_LIST_WAIT(&((mca_btl_openib_module_t*)btl)->send_free_control, item, rc);       \
    frag = (mca_btl_openib_frag_t*) item;                              \
}

#define MCA_BTL_IB_FRAG_ALLOC_EAGER(btl, frag, rc)                     \
{                                                                      \
                                                                       \
    ompi_free_list_item_t *item;                                       \
    OMPI_FREE_LIST_GET(&((mca_btl_openib_module_t*)btl)->send_free_eager, item, rc);       \
    frag = (mca_btl_openib_frag_t*) item;                              \
}

#define MCA_BTL_IB_FRAG_ALLOC_MAX(btl, frag, rc)                               \
{                                                                      \
                                                                       \
    ompi_free_list_item_t *item;                                            \
    OMPI_FREE_LIST_GET(&((mca_btl_openib_module_t*)btl)->send_free_max, item, rc);       \
    frag = (mca_btl_openib_frag_t*) item;                                  \
}

#define MCA_BTL_IB_FRAG_ALLOC_FRAG(btl, frag, rc)                               \
{                                                                      \
                                                                       \
    ompi_free_list_item_t *item;                                            \
    OMPI_FREE_LIST_GET(&((mca_btl_openib_module_t*)btl)->send_free_frag, item, rc);       \
    frag = (mca_btl_openib_frag_t*) item;                                  \
}

#define MCA_BTL_IB_FRAG_RETURN(btl, frag)                                  \
{   do {                                                                   \
        ompi_free_list_t* my_list = NULL;                                  \
        switch(frag->type) {                                               \
         case MCA_BTL_OPENIB_FRAG_EAGER_RDMA:                              \
         case MCA_BTL_OPENIB_FRAG_EAGER:                                   \
          my_list = &btl->send_free_eager;                                 \
          break;                                                           \
         case MCA_BTL_OPENIB_FRAG_MAX:                                     \
          my_list = &btl->send_free_max;                                   \
          break;                                                           \
         case MCA_BTL_OPENIB_FRAG_CONTROL:                                    \
          my_list = &btl->send_free_control;                                  \
          break;                                                           \
         case MCA_BTL_OPENIB_FRAG_FRAG:                                    \
          my_list = &btl->send_free_frag;                                  \
          break;                                                           \
        }                                                                  \
        OMPI_FREE_LIST_RETURN(my_list, (ompi_free_list_item_t*)(frag));    \
    } while(0);                                                            \
}

struct mca_btl_openib_module_t;

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif
#endif

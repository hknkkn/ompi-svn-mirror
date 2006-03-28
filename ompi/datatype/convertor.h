/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 * Copyright (c) 2004-2006 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2006 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2006 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2006 The Regents of the University of California.
 *                         All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef CONVERTOR_H_HAS_BEEN_INCLUDED
#define CONVERTOR_H_HAS_BEEN_INCLUDED

#include "ompi_config.h"
#include "ompi/constants.h"
#include "ompi/datatype/datatype.h"
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

/*
 * CONVERTOR SECTION
 */
/* keep the last 16 bits free for data flags */
#define CONVERTOR_DATATYPE_MASK    0x0000FFFF
#define CONVERTOR_USELESS          0x00010000
#define CONVERTOR_RECV             0x00020000
#define CONVERTOR_SEND             0x00040000
#define CONVERTOR_HOMOGENEOUS      0x00080000
#define CONVERTOR_CLONE            0x00100000
#define CONVERTOR_WITH_CHECKSUM    0x00200000
#define CONVERTOR_TYPE_MASK        0x00FF0000
#define CONVERTOR_STATE_MASK       0xFF000000
#define CONVERTOR_STATE_START      0x01000000
#define CONVERTOR_STATE_COMPLETE   0x02000000
#define CONVERTOR_STATE_ALLOC      0x04000000
#define CONVERTOR_COMPLETED        0x08000000
#define CONVERTOR_COMPUTE_CRC      0x10000000

typedef struct ompi_convertor_t ompi_convertor_t;

typedef int32_t (*conversion_fct_t)( ompi_convertor_t* pConvertor, uint32_t count,
                                     const void* from, uint32_t from_len, long from_extent,
                                     void* to, uint32_t to_length, long to_extent, 
                                     uint32_t *advance );

typedef int32_t (*convertor_advance_fct_t)( ompi_convertor_t* pConvertor,
                                            struct iovec* iov,
                                            uint32_t* out_size,
                                            size_t* max_data,
                                            int32_t* freeAfter );
typedef void*(*memalloc_fct_t)( size_t* pLength, void* userdata );

/* The master convertor struct (defined in convertor_internal.h) */
struct ompi_convertor_master_t;

typedef struct dt_stack {
    int16_t index;    /**< index in the element description */
    int16_t type;     /**< the type used for the last pack/unpack (original or DT_BYTE) */
    int32_t count;    /**< number of times we still have to do it */
    int32_t end_loop; /**< for loops the end of the loop, otherwise useless */
    long    disp;     /**< actual displacement depending on the count field */
} dt_stack_t;

#define DT_STATIC_STACK_SIZE   5

struct ompi_convertor_t {
    opal_object_t                 super;        /**< basic superclass */
    uint32_t                      remoteArch;   /**< the remote architecture */
    uint32_t                      flags;        /**< the properties of this convertor */
    size_t                        local_size;
    size_t                        remote_size;
    const struct ompi_datatype_t* pDesc;        /**< the datatype description associated with the convertor */
    const struct dt_type_desc*    use_desc;     /**< the version used by the convertor (normal or optimized) */
    uint32_t                      count;        /**< the total number of full datatype elements */
    char*                         pBaseBuf;     /**< initial buffer as supplied by the user */
    dt_stack_t*                   pStack;       /**< the local stack for the actual conversion */
    uint32_t                      stack_size;   /**< size of the allocated stack */
    convertor_advance_fct_t       fAdvance;     /**< pointer to the pack/unpack functions */
    memalloc_fct_t                memAlloc_fn;  /**< pointer to the memory allocation function */
    void*                         memAlloc_userdata;  /**< user data for the malloc function */
    struct ompi_convertor_master_t* master;     /* the master convertor */
    conversion_fct_t*             pFunctions;   /**< the convertor functions pointer */
    /* All others fields get modified for every call to pack/unpack functions */
    uint32_t                      stack_pos;    /**< the actual position on the stack */
    size_t                        bConverted;   /**< # of bytes already converted */
    uint32_t                      checksum;     /**< checksum computed by pack/unpack operation */
    uint32_t                      csum_ui1;     /**< partial checksum computed by pack/unpack operation */
    uint32_t                      csum_ui2;     /**< partial checksum computed by pack/unpack operation */
    char                          pending[16];  /**< bytes pending from the last conversion */
    uint32_t                      pending_length; /**< # bytes pending ... */
    dt_stack_t                    static_stack[DT_STATIC_STACK_SIZE];  /**< local stack for small datatypes */
};
OBJ_CLASS_DECLARATION( ompi_convertor_t );

/* Base convertor for all external32 operations */
OMPI_DECLSPEC extern ompi_convertor_t* ompi_mpi_external32_convertor;
OMPI_DECLSPEC extern ompi_convertor_t* ompi_mpi_local_convertor;
OMPI_DECLSPEC extern uint32_t          ompi_mpi_local_arch;

extern conversion_fct_t ompi_ddt_copy_functions[];
extern conversion_fct_t ompi_ddt_heterogeneous_copy_functions[];

/*
 *
 */
static inline uint32_t
ompi_convertor_get_checksum( ompi_convertor_t* convertor )
{
    return convertor->checksum;
}

/*
 *
 */
OMPI_DECLSPEC int32_t
ompi_convertor_pack( ompi_convertor_t* pConv,
                     struct iovec* iov,
                     uint32_t* out_size,
                     size_t* max_data,
                     int32_t* freeAfter );

/*
 *
 */
OMPI_DECLSPEC int32_t
ompi_convertor_unpack( ompi_convertor_t* pConv,
                       struct iovec* iov,
                       uint32_t* out_size,
                       size_t* max_data,
                       int32_t* freeAfter );

/*
 *
 */
OMPI_DECLSPEC ompi_convertor_t* ompi_convertor_create( int32_t remote_arch, int32_t mode );

/*
 *
 */
OMPI_DECLSPEC int ompi_convertor_cleanup( ompi_convertor_t* convertor );

/*
 *
 */
OMPI_DECLSPEC int32_t
ompi_convertor_personalize( ompi_convertor_t* pConv, uint32_t flags,
                            size_t* starting_point,
                            memalloc_fct_t allocfn,
                            void* userdata );

/*
 *
 */
static inline int32_t
ompi_convertor_need_buffers( const ompi_convertor_t* pConvertor )
{
    return !ompi_ddt_is_contiguous_memory_layout( pConvertor->pDesc, pConvertor->count );
}

/*
 *
 */
static inline void
ompi_convertor_get_packed_size( const ompi_convertor_t* pConv,
                                size_t* pSize )
{
    *pSize = pConv->local_size;
}

/*
 *
 */
static inline void
ompi_convertor_get_unpacked_size( const ompi_convertor_t* pConv,
                                  size_t* pSize )
{
    *pSize = pConv->remote_size;
}

/*
 * This function is internal to the data type engine. It should not be called from
 * outside. The data preparation should use the specialized prepare_for_send and
 * prepare_for_recv functions.
 */
OMPI_DECLSPEC
int ompi_convertor_prepare( ompi_convertor_t* convertor,
                            const struct ompi_datatype_t* datatype, int32_t count,
                            const void* pUserBuf );

/*
 *
 */
OMPI_DECLSPEC int32_t
ompi_convertor_prepare_for_send( ompi_convertor_t* convertor,
                                 const struct ompi_datatype_t* datatype,
                                 int32_t count,
                                 const void* pUserBuf);
static inline int32_t
ompi_convertor_copy_and_prepare_for_send( const ompi_convertor_t* pSrcConv, 
                                          const struct ompi_datatype_t* datatype,
                                          int32_t count,
                                          const void* pUserBuf,
                                          int32_t flags,
                                          ompi_convertor_t* convertor )
{
    convertor->remoteArch = pSrcConv->remoteArch;
    convertor->pFunctions = pSrcConv->pFunctions;
    convertor->flags      = (pSrcConv->flags | flags) & ~CONVERTOR_STATE_MASK;
    
    return ompi_convertor_prepare_for_send( convertor, datatype, count, pUserBuf );
}

/*
 *
 */
OMPI_DECLSPEC int32_t
ompi_convertor_prepare_for_recv( ompi_convertor_t* convertor,
                                 const struct ompi_datatype_t* datatype,
                                 int32_t count,
                                 const void* pUserBuf );
static inline int32_t
ompi_convertor_copy_and_prepare_for_recv( const ompi_convertor_t* pSrcConv, 
                                          const struct ompi_datatype_t* datatype,
                                          int32_t count,
                                          const void* pUserBuf,
                                          int32_t flags,
                                          ompi_convertor_t* convertor )
{
    convertor->remoteArch = pSrcConv->remoteArch;
    convertor->pFunctions = pSrcConv->pFunctions;
    convertor->flags      = (pSrcConv->flags | flags) & ~CONVERTOR_STATE_MASK;
        
    return ompi_convertor_prepare_for_recv( convertor, datatype, count, pUserBuf );
}

/*
 * Upper level does not need to call the _nocheck function directly.
 */
OMPI_DECLSPEC int32_t
ompi_convertor_set_position_nocheck( ompi_convertor_t* convertor,
                                     size_t* position );
static inline int32_t
ompi_convertor_set_position( ompi_convertor_t* convertor,
                             size_t* position )
{
    /*
     * If the convertor is already at the correct position we are happy.
     */
    if( (*position) == convertor->bConverted ) return OMPI_SUCCESS;
    return ompi_convertor_set_position_nocheck( convertor, position );
}

/*
 *
 */
OMPI_DECLSPEC int
ompi_convertor_clone( const ompi_convertor_t* source,
                      ompi_convertor_t* destination,
                      int32_t copy_stack );
static inline int
ompi_convertor_clone_with_position( const ompi_convertor_t* source,
                                    ompi_convertor_t* destination,
                                    int32_t copy_stack,
                                    size_t* position )
{
    (void)ompi_convertor_clone( source, destination, copy_stack );
    return ompi_convertor_set_position( destination, position );
}

/*
 *
 */
OMPI_DECLSPEC void ompi_convertor_dump( ompi_convertor_t* convertor );
OMPI_DECLSPEC void ompi_ddt_dump_stack( const dt_stack_t* pStack, int stack_pos,
                                        const union dt_elem_desc* pDesc, const char* name );

/*
 *
 */
OMPI_DECLSPEC int
ompi_convertor_generic_simple_position( ompi_convertor_t* pConvertor,
                                        size_t* position );

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif

#endif  /* CONVERTOR_H_HAS_BEEN_INCLUDED */

/*
 * Copyright (c) 2004-2007 The Trustees of the University of Tennessee.
 *                         All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef __VPROTOCOL_PESSIMIST_SENDERBASED_H__
#define __VPROTOCOL_PESSIMIST_SENDERBASED_H__

#include "vprotocol_pessimist_request.h"
#include "ompi/mca/pml/base/pml_base_sendreq.h"


/* There is several different ways of packing the data to the sender-based 
 * buffer. Just pick one. 
 */
#define SB_USE_PACK_METHOD 
#undef SB_USE_SELFCOMM_METHOD
#undef SB_USE_CONVERTOR_METHOD


typedef struct vprotocol_pessimist_sender_based_t 
{
    int sb_pagesize;        /* size of memory pages on this architecture */
#ifdef SB_USE_SELF_METHOD
    ompi_communicator_t *sb_comm;
#endif    

    int sb_fd;              /* file descriptor of mapped file */
    off_t sb_offset;        /* offset in mmaped file          */
      
    uintptr_t sb_addr;      /* base address of mmaped segment */
    size_t sb_length;       /* length of mmaped segment */
    uintptr_t sb_cursor;    /* current pointer to writeable memory */
    size_t sb_available;    /* available space before end of segment */
} vprotocol_pessimist_sender_based_t;

#include "vprotocol_pessimist.h"

typedef struct vprotocol_pessimist_sender_based_header_t
{
    size_t size;
    int dst; 
    int tag;
    uint32_t contextid;
    vprotocol_pessimist_clock_t sequence;
} vprotocol_pessimist_sender_based_header_t;

int vprotocol_pessimist_sender_based_init(const char *mmapfile, size_t size);

void vprotocol_pessimist_sender_based_finalize(void);

/** Manage mmap floating window, allocating enough memory for the message to be 
  * asynchronously copied to disk.
  */
void vprotocol_pessimist_sender_based_alloc(size_t len);


/** Copy data associated to a pml_base_send_request_t to the sender based 
  * message payload buffer
  */
#define VPROTOCOL_PESSIMIST_SENDER_BASED_COPY(REQ) do {                       \
    mca_pml_base_send_request_t *req = (mca_pml_base_send_request_t *) (REQ); \
    if(req->req_bytes_packed +                                                \
       sizeof(vprotocol_pessimist_sender_based_header_t) >=                   \
       mca_vprotocol_pessimist.sender_based.sb_available)                     \
    {                                                                         \
        vprotocol_pessimist_sender_based_alloc(req->req_bytes_packed);        \
    }                                                                         \
    __SENDER_BASED_COPY(req);                                                 \
} while(0)

#define __SENDER_BASED_COPY(req) do {                                         \
    vprotocol_pessimist_sender_based_header_t *sbhdr =                        \
        (vprotocol_pessimist_sender_based_header_t *)                         \
            mca_vprotocol_pessimist.sender_based.sb_cursor;                   \
    sbhdr->size = req->req_bytes_packed;                                      \
    sbhdr->dst = req->req_base.req_peer;                                      \
    sbhdr->tag = req->req_base.req_tag;                                       \
    sbhdr->contextid = req->req_base.req_comm->c_contextid;                   \
    sbhdr->sequence = req->req_base.req_sequence;                             \
    mca_vprotocol_pessimist.sender_based.sb_cursor +=                         \
            sizeof(vprotocol_pessimist_sender_based_header_t);                \
                                                                              \
    __SENDER_BASED_METHOD_COPY(req);                                          \
    mca_vprotocol_pessimist.sender_based.sb_cursor += sbhdr->size;            \
    mca_vprotocol_pessimist.sender_based.sb_available -= (sbhdr->size +       \
            sizeof(vprotocol_pessimist_sender_based_header_t));               \
    V_OUTPUT_VERBOSE(70, "pessimist:\tsb\twrite\t%"PRIpclock"\tsize %lu", VPESSIMIST_REQ(&req->req_base)->reqid, sbhdr->size + sizeof(vprotocol_pessimist_sender_based_header_t)); \
} while(0)

/** Ensure sender based is finished before allowing user to touch send buffer
  */ 
#define VPROTOCOL_PESSIMIST_SENDER_BASED_FLUSH(REQ) __SENDER_BASED_METHOD_FLUSH(REQ)

#if defined(SB_USE_PACK_METHOD)
    /* Convertor pack (blocking) method */
#define __SENDER_BASED_METHOD_COPY(req) do {                                  \
    if(0 != req->req_bytes_packed) {                                          \
        ompi_convertor_t conv;                                                \
        size_t max_data;                                                      \
        size_t zero = 0;                                                      \
        unsigned int iov_count = 1;                                           \
        struct iovec iov;                                                     \
                                                                              \
        iov.iov_len = req->req_bytes_packed;                                  \
        iov.iov_base =                                                        \
            (IOVBASE_TYPE *) mca_vprotocol_pessimist.sender_based.sb_cursor;  \
        ompi_convertor_clone_with_position( &req->req_base.req_convertor,     \
                                            &conv, 0, &zero );                \
        ompi_convertor_pack(&conv, &iov, &iov_count, &max_data);              \
    }                                                                         \
} while(0)

#define __SENDER_BASED_METHOD_FLUSH(REQ)

#elif defined(SB_USE_SELFCOMM_METHOD)
/* iRecv/Send on SELF pack method */
#define __SENDER_BASED_METHOD_COPY(req) do {                                  \
    mca_pml_v.host_pml.pml_irecv(                                             \
            mca_vprotocol_pessimist.sender_based.sb_cursor,                   \
            req->req_bytes_packed, MPI_PACKED, 0, 0,                          \
            mca_vprotocol_pessimist.sender_based.sb_comm,                     \
            &VPESSIMIST_SEND_REQ(req)->sb_reqs[0]);                           \
    mca_pml_v.host_pml.pml_isend(req->req_base.req_addr,                      \
            req->req_base.req_count, req->req_base.req_datatype, 0, 0,        \
            MCA_PML_BASE_SEND_READY,                                          \
            mca_vprotocol_pessimist.sender_based.sb_comm,                     \
            &VPESSIMIST_SEND_REQ(req)->sb_reqs[1]);                           \
} while(0);

#define __SENDER_BASED_METHOD_FLUSH(REQ) do {                                 \
    if(NULL != VPESSIMIST_REQ(REQ)->sb_reqs[0])                               \
    {                                                                         \
        ompi_request_wait_all(2, VPESSIMIST_REQ(REQ)->sb_reqs,                \
                              MPI_STATUSES_IGNORE);                           \
        VPESSIMIST_REQ(REQ)->sb_reqs[0] = NULL;                               \
    }                                                                         \
} while(0)

#elif defined(SB_USE_CONVERTOR_METHOD)
/* Convertor replacement (non blocking) method */
convertor_advance_fct_t vprotocol_pessimist_sender_based_convertor_advance;

#define __SENDER_BASED_METHOD_COPY(REQ) do {                                  \
    ompi_convertor_t *pConv;                                                  \
    vprotocol_pessimist_send_request_t *pReq;                                 \
                                                                              \
    pConv = & (REQ)->req_base.req_convertor;                                  \
    pReq = VPESSIMIST_SEND_REQ(REQ);                                          \
    pReq->conv_flags = pConv->flags;                                          \
    pReq->conv_advance = pConv->f_advance;                                    \
                                                                              \
    pConv->flags |= CONVERTOR_NO_OP;                                          \
    pConv->f_advance = vprotocol_pessimist_sender_based_convertor_advance;    \
} while(0)

#define __SENDER_BASED_METHOD_FLUSH(REQ)

#endif /* SB_USE_*_METHOD */

#endif

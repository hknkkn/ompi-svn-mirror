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
 *  @file 
 */

#ifndef MCA_PML_OB1_H
#define MCA_PML_OB1_H

#include "threads/thread.h"
#include "threads/condition.h"
#include "class/ompi_free_list.h"
#include "util/cmd_line.h"
#include "request/request.h"
#include "mca/pml/pml.h"
#include "mca/pml/base/pml_base_request.h"
#include "mca/pml/base/pml_base_bsend.h"
#include "mca/pml/base/pml_base_sendreq.h"
#include "mca/bmi/bmi.h"


#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif
/**
 * OB1 PML module
 */

struct mca_pml_ob1_t {
    mca_pml_base_module_t super; 

    mca_bmi_base_component_t **bmi_components;
    size_t num_bmi_components;

    mca_bmi_base_module_t** bmi_modules;
    size_t num_bmi_modules;

    mca_bmi_base_component_progress_fn_t* bmi_progress;
    size_t num_bmi_progress;

    int priority;
    int free_list_num;      /* initial size of free list */
    int free_list_max;      /* maximum size of free list */
    int free_list_inc;      /* number of elements to grow free list */
    size_t eager_limit;     /* maximum eager limit size - overrides bmi setting */
    size_t rdma_offset;     /* offset at which we attempt to initiate rdma */
    size_t rdma_threshold;  /* message size at which rdma is attempted */
    size_t send_pipeline_depth;
    size_t recv_pipeline_depth;

    /* lock queue access */
    ompi_mutex_t lock;

    /* free lists */
    ompi_free_list_t send_requests;
    ompi_free_list_t recv_requests;
    ompi_free_list_t buffers;
    ompi_free_list_t fragments;

    /* list of pending send requests */
    ompi_list_t send_pending;
    ompi_list_t recv_pending;
    ompi_list_t acks_pending;
};
typedef struct mca_pml_ob1_t mca_pml_ob1_t; 

extern mca_pml_ob1_t mca_pml_ob1;


/*
 * PML module functions.
 */


extern int mca_pml_ob1_component_open(void);
extern int mca_pml_ob1_component_close(void);

extern mca_pml_base_module_t* mca_pml_ob1_component_init(
    int *priority, 
    bool enable_progress_threads,
    bool enable_mpi_threads
);

extern int mca_pml_ob1_component_fini(void);



/*
 * PML interface functions.
 */

extern int mca_pml_ob1_add_comm(
    struct ompi_communicator_t* comm
);

extern int mca_pml_ob1_del_comm(
    struct ompi_communicator_t* comm
);

extern int mca_pml_ob1_add_procs(
    struct ompi_proc_t **procs,
    size_t nprocs
);

extern int mca_pml_ob1_del_procs(
    struct ompi_proc_t **procs,
    size_t nprocs
);

extern int mca_pml_ob1_add_bmis(void);

extern int mca_pml_ob1_enable(
    bool enable
);

extern int mca_pml_ob1_progress(void);

extern int mca_pml_ob1_iprobe(
    int dst,
    int tag,
    struct ompi_communicator_t* comm,
    int *matched,
    ompi_status_public_t* status
);

extern int mca_pml_ob1_probe(
    int dst,
    int tag,
    struct ompi_communicator_t* comm,
    ompi_status_public_t* status
);

extern int mca_pml_ob1_isend_init(
    void *buf,
    size_t count,
    ompi_datatype_t *datatype,
    int dst,
    int tag,
    mca_pml_base_send_mode_t mode,
    struct ompi_communicator_t* comm,
    struct ompi_request_t **request
);

extern int mca_pml_ob1_isend(
    void *buf,
    size_t count,
    ompi_datatype_t *datatype,
    int dst,
    int tag,
    mca_pml_base_send_mode_t mode,
    struct ompi_communicator_t* comm,
    struct ompi_request_t **request
);

extern int mca_pml_ob1_send(
    void *buf,
    size_t count,
    ompi_datatype_t *datatype,
    int dst,
    int tag,
    mca_pml_base_send_mode_t mode,
    struct ompi_communicator_t* comm
);

extern int mca_pml_ob1_irecv_init(
    void *buf,
    size_t count,
    ompi_datatype_t *datatype,
    int src,
    int tag,
    struct ompi_communicator_t* comm,
    struct ompi_request_t **request
);

extern int mca_pml_ob1_irecv(
    void *buf,
    size_t count,
    ompi_datatype_t *datatype,
    int src,
    int tag,
    struct ompi_communicator_t* comm,
    struct ompi_request_t **request
);

extern int mca_pml_ob1_recv(
    void *buf,
    size_t count,
    ompi_datatype_t *datatype,
    int src,
    int tag,
    struct ompi_communicator_t* comm,
    ompi_status_public_t* status
);

extern int mca_pml_ob1_progress(void);

extern int mca_pml_ob1_start(
    size_t count,
    ompi_request_t** requests
);

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif

#define MCA_PML_OB1_FINI(request) \
{ \
    mca_pml_base_request_t* pml_request = *(mca_pml_base_request_t**)(request); \
    if(pml_request->req_persistent) { \
       if(pml_request->req_free_called) { \
           MCA_PML_OB1_FREE(request); \
       } else { \
           pml_request->req_ompi.req_state = OMPI_REQUEST_INACTIVE; \
       } \
    } else { \
        MCA_PML_OB1_FREE(request); \
    } \
}


#define MCA_PML_OB1_FREE(request) \
{ \
    mca_pml_base_request_t* pml_request = *(mca_pml_base_request_t**)(request); \
    pml_request->req_free_called = true; \
    if( pml_request->req_pml_complete == true) \
    { \
        switch(pml_request->req_type) { \
        case MCA_PML_REQUEST_SEND: \
            { \
                mca_pml_ob1_send_request_t* sendreq = (mca_pml_ob1_send_request_t*)pml_request; \
                if(sendreq->req_send.req_send_mode == MCA_PML_BASE_SEND_BUFFERED) { \
                    mca_pml_base_bsend_request_fini((ompi_request_t*)sendreq); \
                } \
                MCA_PML_OB1_SEND_REQUEST_RETURN(sendreq); \
                break; \
            } \
        case MCA_PML_REQUEST_RECV: \
            { \
                mca_pml_ob1_recv_request_t* recvreq = (mca_pml_ob1_recv_request_t*)pml_request; \
                MCA_PML_OB1_RECV_REQUEST_RETURN(recvreq); \
                break; \
            } \
        default: \
            break; \
        } \
    } \
    *(request) = MPI_REQUEST_NULL; \
}
                                                                                                                       


#endif


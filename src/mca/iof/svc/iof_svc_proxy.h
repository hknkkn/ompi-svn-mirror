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
/**
 * @file
 */
#ifndef MCA_IOF_SVC_PROXY_H
#define MCA_IOF_SVC_PROXY_H

#include "mca/iof/iof.h"
#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

#include "ompi_config.h"
#include "mca/iof/iof.h"
#include "mca/iof/base/iof_base_header.h"


/**
 *  Callback function from OOB on receipt of IOF request.
 *
 *  @param status (IN)  Completion status.
 *  @param peer (IN)    Opaque name of peer process.
 *  @param msg (IN)     Array of iovecs describing user buffers and lengths.
 *  @param count (IN)   Number of elements in iovec array.
 *  @param tag (IN)     User defined tag for matching send/recv.
 *  @param cbdata (IN)  User data.
*/
                                                                                                               
void mca_iof_svc_proxy_recv(
    int status,
    ompi_process_name_t* peer,
    struct iovec* msg,
    int count,
    int tag,
    void* cbdata);

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif
#endif


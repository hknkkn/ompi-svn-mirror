/*
 * $HEADER$
 */
#include "ompi_config.h"
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "include/constants.h"
#include "util/argv.h"
#include "util/output.h"
#include "mca/pml/pml.h"
#include "mca/ptl/ptl.h"
#include "ptl_mx.h"
#include "ptl_mx_sendfrag.h"
#include "ptl_mx_recvfrag.h"


mca_ptl_mx_module_t mca_ptl_mx_module = {
    {
    &mca_ptl_mx_component.super,
    8, /* ptl_cache_size */
    sizeof(mca_ptl_mx_send_frag_t), /* ptl_cache_bytes */
    0, /* ptl_frag_first_size */
    0, /* ptl_frag_min_size */
    0, /* ptl_frag_max_size */
    0, /* ptl_exclusivity */
    0, /* ptl_latency */
    0, /* ptl_bandwidth */
    MCA_PTL_PUT, /* ptl flags */
    mca_ptl_mx_add_procs,
    mca_ptl_mx_del_procs,
    mca_ptl_mx_finalize,
    mca_ptl_mx_send,  /* put */
    mca_ptl_mx_send,  /* put */
    NULL, /* get */
    mca_ptl_mx_matched, /* matched */
    mca_ptl_mx_request_init,
    mca_ptl_mx_request_fini,
    NULL, /* match */
    NULL,
    NULL
    }
};


/**
 * PML->PTL notification of addition to the process list.
 *
 * @param ptl (IN)
 * @param nprocs (IN)     Number of processes
 * @param procs (IN)      Set of processes
 * @param peers (OUT)     Set of (optional) peer addressing info.
 * @param peers (IN/OUT)  Set of processes that are reachable via this PTL.
 * @return                OMPI_SUCCESS or error status on failure.
 *
 */
                                                                                
int mca_ptl_mx_add_procs(
    struct mca_ptl_base_module_t* ptl, 
    size_t nprocs, 
    struct ompi_proc_t **ompi_proc, 
    struct mca_ptl_base_peer_t** peer_ret, 
    ompi_bitmap_t* reachable)
{
    size_t i;
    for( i = 0; i < nprocs; i++ ) {
    }
    return OMPI_SUCCESS;
}


/**
 * PML->PTL notification of change in the process list.
 *
 * @param ptl (IN)     PTL instance
 * @param nproc (IN)   Number of processes.
 * @param procs (IN)   Set of processes.
 * @param peers (IN)   Set of peer data structures.
 * @return             Status indicating if cleanup was successful
 *
 */

int mca_ptl_mx_del_procs(
    struct mca_ptl_base_module_t* ptl, 
    size_t nprocs, 
    struct ompi_proc_t **proc, 
    struct mca_ptl_base_peer_t** ptl_peer)
{
    return OMPI_SUCCESS;
}

int mca_ptl_mx_finalize(struct mca_ptl_base_module_t* ptl)
{
    return OMPI_SUCCESS;
}

/**
 * PML->PTL Initialize a send request for use by the PTL.
 *
 * @param ptl (IN)       PTL instance
 * @param request (IN)   Pointer to allocated request.
 *
 * To reduce latency (number of required allocations), the PML allocates up
 * to ptl_cache_bytes of additional space contigous w/ the base send request.
 * This space may be used by the PTL for additional control information (e.g.
 * first fragment descriptor).
 *
 * The ptl_request_init() function is called by the PML when requests are
 * allocated to the PTLs cache. These requests will be cached by the PML
 * on completion and re-used by the same PTL w/out additional calls to
 * ptl_request_init().
 *
 * If the cache size is exceeded, the PML may pass requests to ptl_send/ptl_put
 * that have been taken from the global pool and have not been initialized by the
 * PTL. These requests will have the req_cached attribute set to false.
 *
 */

int mca_ptl_mx_request_init(struct mca_ptl_base_module_t* ptl, mca_pml_base_send_request_t* request)
{
    OBJ_CONSTRUCT(request+1, mca_ptl_mx_send_frag_t);
    return OMPI_SUCCESS;
}


/**
 * PML->PTL Cleanup any resources that may have been associated with the
 *          request by the PTL.
 *
 * @param ptl (IN)       PTL instance
 * @param request (IN)   Pointer to allocated request.
 *
 * The ptl_request_fini function is called when the PML removes a request
 * from the PTLs cache (due to resource constraints).  This routine provides
 * the PTL the chance to cleanup/release any resources cached on the send
 * descriptor by the PTL.
 */

void mca_ptl_mx_request_fini(struct mca_ptl_base_module_t* ptl, mca_pml_base_send_request_t* request)
{
    OBJ_DESTRUCT(request+1);
}


/**
 * PML->PTL Notification from the PML to the PTL that a receive
 * has been posted and matched against the indicated fragment.
 *
 * @param ptl (IN)       PTL instance
 * @param recv_frag      Matched fragment
 *
 * The ptl_matched() function is called by the PML when a fragment
 * is matched to a posted receive. This may occur during a call to
 * ptl_match() if the receive is matched, or at a later point in time
 * when a matching receive is posted.
 *
 * When this routine is called, the PTL is responsible for generating
 * an acknowledgment to the peer if the MCA_PTL_FLAGS_ACK_MATCHED
 * bit is set in the original fragment header. Additionally, the PTL
 * is responsible for transferring any data associated with the fragment
 * into the users buffer utilizing the datatype engine, and notifying
 * the PML that the fragment has completed via the ptl_recv_progress()
 * function.
 */

void mca_ptl_mx_matched( 
    mca_ptl_base_module_t* ptl,
    mca_ptl_base_recv_frag_t* frag)
{
}



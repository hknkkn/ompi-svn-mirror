/*
 * $HEADER$
 */

#include <string.h>
#include "util/output.h"
#include "util/if.h"
#include "mca/pml/pml.h"
#include "mca/ptl/ptl.h"
#include "mca/ptl/base/ptl_base_header.h"
#include "mca/ptl/base/ptl_base_sendfrag.h"
#include "mca/pml/base/pml_base_sendreq.h"
#include "mca/pml/base/pml_base_recvreq.h"
#include "mca/ptl/base/ptl_base_recvfrag.h"
#include "mca/base/mca_base_module_exchange.h"
#include "ptl_elan.h"
#include "ptl_elan_peer.h"
#include "ptl_elan_proc.h"
#include "ptl_elan_req.h"
#include "ptl_elan_frag.h"
#include "ptl_elan_priv.h"


/* XXX: There must be multiple PTL's. This could be the template */
mca_ptl_elan_t  mca_ptl_elan = {
    {
        &mca_ptl_elan_module.super,
        0,                         /* ptl_exclusivity */
        0,                         /* ptl_latency */
        0,                         /* ptl_bandwidth */
        0,                         /* ptl_frag_first_size */
        0,                         /* ptl_frag_min_size */
        0,                         /* ptl_frag_max_size */
        MCA_PTL_PUT,               /* ptl flags */
        
        /* collection of interfaces */
        mca_ptl_elan_add_procs,
        mca_ptl_elan_del_procs,
        mca_ptl_elan_finalize,
        mca_ptl_elan_put,
        mca_ptl_elan_get,
        mca_ptl_elan_matched,
        mca_ptl_elan_req_alloc,
        mca_ptl_elan_req_return
    }
};

int mca_ptl_elan_add_procs (struct mca_ptl_t *ptl,
	                    size_t nprocs,
                            struct ompi_proc_t **procs,
                            struct mca_ptl_base_peer_t **peers,
			    ompi_bitmap_t* reachable)
{
    struct ompi_proc_t  *ompi_proc;
    mca_ptl_elan_proc_t *ptl_proc;
    mca_ptl_elan_peer_t *ptl_peer;

    int rc;
    int i;

    for(i=0; i<nprocs; i++) {

       	ompi_proc = procs[i];
       	ptl_proc  = mca_ptl_elan_proc_create(ompi_proc);
	OMPI_PTL_ELAN_CHECK_UNEX(ptl_proc, NULL, OMPI_ERR_OUT_OF_RESOURCE, 0);

        /* Check to make sure that the peer has at least as many 
	 * interface addresses exported as we are trying to use. 
	 * If not, then don't bind this PTL instance to the proc.
         */
        OMPI_THREAD_LOCK(&ptl_proc->proc_lock);

        if(ptl_proc->proc_addr_count == ptl_proc->proc_peer_count) {
            OMPI_THREAD_UNLOCK(&ptl_proc->proc_lock);
	    ompi_output(0, "all peers are taken already\n");
            return OMPI_ERR_UNREACH;
        }

        /* The ptl_proc datastructure is shared by all TCP PTL 
	 * instances that are trying to reach this destination. 
	 * Cache the peer instance on the ptl_proc.
         */
        ptl_peer = OBJ_NEW(mca_ptl_elan_peer_t);
        if(NULL == ptl_peer) {
            OMPI_THREAD_UNLOCK(&ptl_proc->proc_lock);
            return OMPI_ERR_OUT_OF_RESOURCE;
        }
        ptl_peer->peer_ptl = (mca_ptl_elan_t*)ptl;
        rc = mca_ptl_elan_proc_insert(ptl_proc, ptl_peer);
        if(rc != OMPI_SUCCESS) {
            OBJ_RELEASE(ptl_peer);
            OMPI_THREAD_UNLOCK(&ptl_proc->proc_lock);
            return rc;
        }
        ompi_bitmap_set_bit(reachable, i);
        OMPI_THREAD_UNLOCK(&ptl_proc->proc_lock);
        peers[i] = ptl_peer;
    }
    return OMPI_SUCCESS;
}

int mca_ptl_elan_del_procs (struct mca_ptl_t *ptl, 
			   size_t nprocs,
			   struct ompi_proc_t ** procs, 
			   struct mca_ptl_base_peer_t **peers)
{
    return OMPI_SUCCESS;
}

int mca_ptl_elan_finalize (struct mca_ptl_t *ptl)
{
    int rail_index;
    struct mca_ptl_elan_t * elan_ptl ;

    elan_ptl = (struct mca_ptl_elan_t *) ptl;

    /* XXX: Free all the lists, etc, hanged over PTL */

    /* Free the PTL */
    rail_index = elan_ptl->ptl_ni_local;
    free(elan_ptl);

    /* Record the missing of this entry */
    mca_ptl_elan_module.elan_ptls[rail_index] = NULL;
    mca_ptl_elan_module.elan_num_ptls -- ;

    return OMPI_SUCCESS;
}

int mca_ptl_elan_req_alloc (struct mca_ptl_t *ptl, 
        struct mca_pml_base_send_request_t **request)
{
    int       rc = OMPI_SUCCESS;
    mca_pml_base_send_request_t* sendreq;
    ompi_list_item_t* item;

#if 0 
    /* PTL_TCP allocate request from the module
     * But PTL_ELAN have to allocate from each PTL since
     * all the descriptors are bound to the PTL related command queue, etc
     * for their functions.
     */
    OMPI_FREE_LIST_GET(&mca_ptl_elan_module.elan_send_requests, item, rc);

    if(NULL != (sendreq = (mca_pml_base_send_request_t*)item))
        sendreq->req_owner = ptl;
    *request = sendreq;
#endif 
    return rc;
}


void mca_ptl_elan_req_return (struct mca_ptl_t *ptl, 
        struct mca_pml_base_send_request_t *request)
{
    /*OBJ_DESTRUCT(&request->req_convertor);*/
#if 0
    OMPI_FREE_LIST_RETURN(&mca_ptl_elan_module.elan_send_requests, 
            (ompi_list_item_t*)request);
#endif
    return;
}


void mca_ptl_elan_recv_frag_return (struct mca_ptl_t *ptl,
                                    struct mca_ptl_elan_recv_frag_t *frag)
{
    return;
}


void mca_ptl_elan_send_frag_return (struct mca_ptl_t *ptl,
                                    struct mca_ptl_elan_send_frag_t *frag)
{
    return;
}

/*
 *  Initiate a put operation. 
 */

int mca_ptl_elan_put (struct mca_ptl_t* ptl, 
		      struct mca_ptl_base_peer_t* ptl_base_peer, 
		      struct mca_pml_base_send_request_t* request,
		      size_t offset,
		      size_t size,
		      int flags)
{
    return OMPI_SUCCESS;
}

/*
 *  Initiate a get. 
 */

int mca_ptl_elan_get (struct mca_ptl_t* ptl, 
		      struct mca_ptl_base_peer_t* ptl_base_peer, 
		      struct mca_pml_base_recv_request_t* request,
		      size_t offset,
		      size_t size,
		      int flags)
{
    return OMPI_SUCCESS;
}

/*
 *  A posted receive has been matched - if required send an
 *  ack back to the peer and process the fragment.
 */

void mca_ptl_elan_matched (mca_ptl_t * ptl,
                           mca_ptl_base_recv_frag_t * frag)
{
    return;
}

/*
 * $HEADER$
 */

#ifdef HAVE_CONFIG_H
#include "ompi_config.h"
#endif

#include <unistd.h>
#include <sys/types.h>
#include <sys/errno.h>

#include "types.h"
#include "datatype/datatype.h"
#include "mca/pml/base/pml_base_sendreq.h"
#include "mca/pml/base/pml_base_recvreq.h"
#include "ptl_elan.h"
#include "ptl_elan_peer.h"
#include "ptl_elan_proc.h"
#include "ptl_elan_frag.h"
#include "ptl_elan_priv.h"

#include "mca/pml/base/pml_base_sendreq.h"
#include "mca/pml/base/pml_base_recvreq.h"
#include "mca/ptl/base/ptl_base_sendfrag.h"
#include "mca/ptl/base/ptl_base_recvfrag.h"
#include "ptl_elan.h"

static void
mca_ptl_elan_send_frag_construct (mca_ptl_elan_send_frag_t * frag)
{
    frag->frag_progressed = 0;
    frag->desc = 0;
}

static void
mca_ptl_elan_send_frag_destruct (mca_ptl_elan_send_frag_t * frag)
{
    /* Nothing to do then */
}

ompi_class_t mca_ptl_elan_send_frag_t_class = {
    "mca_ptl_elan_send_frag_t",
    OBJ_CLASS (mca_ptl_base_frag_t),
    (ompi_construct_t) mca_ptl_elan_send_frag_construct,
    (ompi_destruct_t) mca_ptl_elan_send_frag_destruct
};

static void
mca_ptl_elan_recv_frag_construct (mca_ptl_elan_recv_frag_t * frag)
{
    frag->frag_hdr_cnt = 0;
    frag->frag_msg_cnt = 0;
    frag->frag_progressed = 0;

    /*frag->frag.qdma = NULL;*/
    frag->alloc_buff = (char *) malloc (sizeof (char) * 2048 + 32);
    if (NULL == frag->alloc_buff) {
        ompi_output (0,
                     "[%s:%d] Fatal error, unable to allocate recv buff \n",
                     __FILE__, __LINE__);
    }
    frag->unex_buff = (char *) (((int) frag->alloc_buff + 32) >> 5 << 5);
}

static void
mca_ptl_elan_recv_frag_destruct (mca_ptl_elan_recv_frag_t * frag)
{
    frag->frag_hdr_cnt = 0;
    frag->frag_msg_cnt = 0;
    frag->frag_progressed = 0;

    /*frag->frag.qdma = NULL;*/
    free (frag->alloc_buff);
    frag->alloc_buff = NULL;
    frag->unex_buff = NULL;
}

ompi_class_t mca_ptl_elan_recv_frag_t_class = {
    "mca_ptl_elan_recv_frag_t",
    OBJ_CLASS (mca_ptl_base_recv_frag_t),
    (ompi_construct_t) mca_ptl_elan_recv_frag_construct,
    (ompi_destruct_t) mca_ptl_elan_recv_frag_destruct
};


extern mca_ptl_elan_state_t mca_ptl_elan_global_state;

/* FIXME: Even though we have also get implemened here, temporarily 
 * mca_ptl_elan_send_frag_t is still used as the return type */
mca_ptl_elan_send_frag_t *
mca_ptl_elan_alloc_desc (struct mca_ptl_base_module_t *ptl_ptr,
       	struct mca_pml_base_request_t *req, int desc_type)
{

    ompi_free_list_t *flist;
    ompi_list_item_t *item = NULL;
    mca_ptl_elan_send_frag_t *desc;

    START_FUNC(PTL_ELAN_DEBUG_SEND);

    /* TODO: Dynamically bind a base request to PUT/GET/QDMA/STEN */
    if (MCA_PTL_ELAN_DESC_QDMA == desc_type) {
        flist = &(((mca_ptl_elan_module_t *) ptl_ptr)->queue)->tx_desc_free;
    } else if (MCA_PTL_ELAN_DESC_PUT == desc_type) {
        flist = &(((mca_ptl_elan_module_t *) ptl_ptr)->putget)->put_desc_free;
    } else if (MCA_PTL_ELAN_DESC_GET == desc_type) {
        flist = &(((mca_ptl_elan_module_t *) ptl_ptr)->putget)->get_desc_free;
    } else {
        ompi_output (0,
                     "[%s:%d] Error: unknown to descriptor desc type\n",
                     __FILE__, __LINE__);
	return NULL;
    }

    if (ompi_using_threads ()) {
	ompi_mutex_lock(&flist->fl_lock);
	item = ompi_list_remove_first (&((flist)->super));
	while (NULL == item) {
	    mca_ptl_tstamp_t tstamp = 0;
	    ptl_ptr->ptl_component->ptlm_progress (tstamp);
	    item = ompi_list_remove_first (&((flist)->super));
	}
	ompi_mutex_unlock(&flist->fl_lock);
    } else {
	item = ompi_list_remove_first (&((flist)->super));
	/* XXX: 
	 * Ouch..., this still does not trigger the progress on 
	 * PTL's from other modules.  Wait for PML to change.
	 * Otherwise have to trigger PML progress from PTL.  */
	while (NULL == item) {
	    mca_ptl_tstamp_t tstamp = 0;
	    ptl_ptr->ptl_component->ptlm_progress (tstamp);
	    item = ompi_list_remove_first (&((flist)->super));
	}
    }
    desc = (mca_ptl_elan_send_frag_t *) item; 
    desc->desc->req = req;
    desc->desc->desc_type = desc_type;
    END_FUNC(PTL_ELAN_DEBUG_SEND);
    return desc;
}

mca_ptl_elan_recv_frag_t *
mca_ptl_elan_alloc_recv_desc (struct mca_pml_base_recv_request_t * req)
{
    return NULL;
}


void 
mca_ptl_elan_send_desc_done (
       	mca_ptl_elan_send_frag_t *frag,
       	mca_pml_base_send_request_t *req) 
{ 
    mca_ptl_elan_module_t *ptl;
    mca_ptl_base_header_t *header;
 
    START_FUNC(PTL_ELAN_DEBUG_SEND);

    ptl = ((ompi_ptl_elan_qdma_desc_t *)frag->desc)->ptl;
    header = &frag->frag_base.frag_header;

    if (frag->desc->desc_type == MCA_PTL_ELAN_DESC_GET) {
	if(ompi_atomic_fetch_and_set_int (&frag->frag_progressed, 1) == 0) {
	    ptl->super.ptl_recv_progress(ptl, 
		    (mca_pml_base_recv_request_t *) req, 
		    frag->frag_base.frag_size,
		    frag->frag_base.frag_size);
	}
	elan4_freecq_space (ptl->ptl_elan_ctx,
		((ompi_ptl_elan_putget_desc_t *) frag->desc)
		->chain_event->ev_Params[1], 8);
	OMPI_FREE_LIST_RETURN (&ptl->putget->get_desc_free,
	       	(ompi_list_item_t *) frag);
	END_FUNC(PTL_ELAN_DEBUG_SEND);
	return;
    }

    LOG_PRINT(PTL_ELAN_DEBUG_SEND, 
	    "req %p done frag %p desc_status %d desc_type %d length %d\n", 
	    req, frag, frag->desc->desc_status, 
	    frag->desc->desc_type,
	    header->hdr_frag.hdr_frag_length);

    if(NULL == req) { /* An ack descriptor */
	OMPI_FREE_LIST_RETURN (&ptl->queue->tx_desc_free,
		(ompi_list_item_t *) frag);
    } 
#if 1   
    else if (0 == (header->hdr_common.hdr_flags 
		& MCA_PTL_FLAGS_ACK_MATCHED)
	    || mca_pml_base_send_request_matched(req)) {
	if(ompi_atomic_fetch_and_set_int (&frag->frag_progressed, 1) == 0) 
	{
	    ptl->super.ptl_send_progress(ptl, req, 
		    header->hdr_frag.hdr_frag_length);
	}

	/* Return a frag or if not cached, or it is a follow up */ 
	if ( /*(header->hdr_frag.hdr_frag_offset != 0) || */
		(frag->desc->desc_status != MCA_PTL_ELAN_DESC_CACHED)){
	    ompi_free_list_t  *flist;
	    if (frag->desc->desc_type == MCA_PTL_ELAN_DESC_PUT) {
		flist = &ptl->putget->put_desc_free;
		elan4_freecq_space (ptl->ptl_elan_ctx,
		       	((ompi_ptl_elan_putget_desc_t *) frag->desc)
			->chain_event->ev_Params[1], 8);
	    } else {
		flist = &ptl->queue->tx_desc_free;
	    }
	    OMPI_FREE_LIST_RETURN (flist, (ompi_list_item_t *) frag);
	} else {
	    LOG_PRINT(PTL_ELAN_DEBUG_ACK,
		    "PML will return frag to list %p, length %d\n", 
		    &ptl->queue->tx_desc_free,
		    ptl->queue->tx_desc_free.super.ompi_list_length);
	}
     }
#else
    else  {

	/* XXX: need to discuss this with TSW.
	 * There is a little confusion here. 
	 * Why the release of this send fragment is dependent 
	 * on the receiving of an acknowledgement 
	 * There are two drawbacks,
	 * a) Send fragment is not immediately returned to the free pool
	 * b) Some list is needed to hold on this fragment and
	 *    later on find an time slot to process it.
	 * c) If ever local completion happens later then the receive
	 *    of the acknowledgement. The following will happen
	 *    1) The receiving of an acknoledgement can not immediatly
	 *    trigger the scheduling the followup fragment since it
	 *    is dependent on the send fragment to complete.
	 *    2) Later, the local send completeion cannot trigger 
	 *       the start of following fragments. As the logic is not there.
	 */

	if(ompi_atomic_fetch_and_set_int (&frag->frag_progressed, 1) == 0) {
	    ptl->super.ptl_send_progress(ptl, req, 
		    header->hdr_frag.hdr_frag_length);
	}

	/* Return a frag or if not cached, or it is a follow up */ 
	if((header->hdr_frag.hdr_frag_offset != 0) || (frag->desc->desc_status 
		    != MCA_PTL_ELAN_DESC_CACHED)) 
	    OMPI_FREE_LIST_RETURN (&queue->tx_desc_free,
		    (ompi_list_item_t *) frag);
    } 
#endif

    END_FUNC(PTL_ELAN_DEBUG_SEND);
}
 
void 
mca_ptl_elan_recv_frag_done (
       	mca_ptl_base_header_t *header,
       	mca_ptl_elan_recv_frag_t* frag,
       	mca_pml_base_recv_request_t *request) 
{ 
    frag->frag_recv.frag_base.frag_owner->ptl_recv_progress (
	    frag->frag_recv.frag_base.frag_owner, 
	    request, 
	    frag->frag_recv.frag_base.frag_size, 
	    frag->frag_recv.frag_base.frag_size);

    /* FIXME: 
     * To support the required ACK, do not return
     * until the ack is out */
    if (frag->frag_ack_pending == false) {
	mca_ptl_elan_recv_frag_return (
		frag->frag_recv.frag_base.frag_owner, frag);
    } else {
	/* XXX: Chaining it into the list of completion pending recv_frag,
	 * Until the ack frag is sent out, they will stay in the list */
    }
}



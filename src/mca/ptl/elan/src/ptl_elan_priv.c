/*
 * $HEADER$
 */

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

/* Initialize an ack descriptor and queue it to the command queue */
int
mca_ptl_elan_start_ack ( mca_ptl_base_module_t * ptl, 
			 mca_ptl_elan_send_frag_t * desc,
			 mca_ptl_elan_recv_frag_t * recv_frag)
{
    struct ompi_ptl_elan_qdma_desc_t *qdma;
    mca_ptl_base_header_t *hdr;
    mca_pml_base_recv_request_t* request;


    START_FUNC();

    desc->desc->desc_type = MCA_PTL_ELAN_DESC_QDMA; 
    qdma = (ompi_ptl_elan_qdma_desc_t *)desc->desc;

    hdr = (mca_ptl_base_header_t *) & desc->buff[0];

    request = recv_frag->frag_recv.frag_request;

    hdr->hdr_common.hdr_type = MCA_PTL_HDR_TYPE_ACK;
    hdr->hdr_common.hdr_flags = 0;
    hdr->hdr_common.hdr_size = sizeof(mca_ptl_base_ack_header_t);

    hdr->hdr_ack.hdr_src_ptr = 
	recv_frag->frag_recv.frag_base.frag_header.hdr_frag.hdr_src_ptr;
    hdr->hdr_ack.hdr_dst_match.lval = 0; 
    hdr->hdr_ack.hdr_dst_match.pval = request;
    hdr->hdr_ack.hdr_dst_addr.lval = 0; 
    hdr->hdr_ack.hdr_dst_addr.pval = request->req_base.req_addr;
    hdr->hdr_ack.hdr_dst_size = request->req_bytes_packed;

    hdr->hdr_frag.hdr_frag_length = sizeof(mca_ptl_base_ack_header_t);

    /* Filling up QDMA descriptor */
    qdma->main_dma.dma_srcAddr = MAIN2ELAN (desc->rail->r_ctx,
					    &desc->buff[0]);

    /* XXX: Hardcoded DMA retry count */
    qdma->main_dma.dma_typeSize = E4_DMA_TYPE_SIZE ((header_length +
						     size_out),
						    DMA_DataTypeByte,
						DMA_QueueWrite, 16);
    qdma->main_dma.dma_cookie = elan4_local_cookie (ptl->queue->tx_cpool, 
		E4_COOKIE_TYPE_LOCAL_DMA, destvp);
    qdma->main_dma.dma_vproc = ((mca_ptl_elan_peer_t *)
	 recv_frag->frag_recv.frag_base.frag_peer)->peer_vp;

    /* Make main memory coherent with IO domain (IA64) */
    MEMBAR_VISIBLE ();

    elan4_run_dma_cmd (ptl->queue->tx_cmdq, (DMA *) & qdma->main_dma);

    /*ptl->queue->tx_cmdq->cmdq_flush */
    elan4_flush_cmdq_reorder (ptl->queue->tx_cmdq);

    /* Insert desc into the list of outstanding DMA's */
    ompi_list_append (&ptl->queue->tx_desc, (ompi_list_item_t *) desc);

    /* fragment state */
    desc->desc->req = NULL;
    desc->frag_base.frag_owner = ptl;
    desc->frag_base.frag_peer = recv_frag->frag_recv.frag_base.frag_peer; 
    desc->frag_base.frag_addr = NULL;
    desc->frag_base.frag_size = 0;
    desc->frag_progressed     = 0;
    desc->desc->desc_status   = MCA_PTL_ELAN_DESC_LOCAL;

    END_FUNC();
    return OMPI_SUCCESS;
}

static void
mca_ptl_elan_init_qdma_desc (struct ompi_ptl_elan_qdma_desc_t *desc,
                             mca_ptl_elan_module_t * ptl,
			     struct mca_ptl_elan_peer_t *ptl_peer,
                             mca_pml_base_send_request_t *pml_req,
			     size_t offset,
			     size_t *size,
			     int flags)
{
    int         header_length;
    mca_ptl_base_header_t *hdr;

    int         destvp;
    int         size_out;
    int         size_in;
    int         rc = OMPI_SUCCESS;
   
    START_FUNC();

    destvp = ptl_peer->peer_vp;
    size_in = *size;

    hdr = (mca_ptl_base_header_t *) & desc->buff[0];

    if(offset == 0) {
	hdr->hdr_common.hdr_type = MCA_PTL_HDR_TYPE_MATCH;
	hdr->hdr_common.hdr_flags = flags;
	hdr->hdr_common.hdr_size = sizeof (mca_ptl_base_match_header_t);
	hdr->hdr_frag.hdr_frag_offset = offset;
	hdr->hdr_frag.hdr_frag_seq = 0;
        hdr->hdr_frag.hdr_src_ptr.lval = 0;
	hdr->hdr_frag.hdr_src_ptr.pval = desc;
	hdr->hdr_frag.hdr_dst_ptr.lval = 0;

	hdr->hdr_match.hdr_contextid = pml_req->req_base.req_comm->c_contextid;
	hdr->hdr_match.hdr_src = pml_req->req_base.req_comm->c_my_rank;
	hdr->hdr_match.hdr_dst = pml_req->req_base.req_peer;
	hdr->hdr_match.hdr_tag = pml_req->req_base.req_tag;
	hdr->hdr_match.hdr_msg_length = pml_req->req_bytes_packed;
	hdr->hdr_match.hdr_msg_seq = pml_req->req_base.req_sequence;

	header_length = sizeof (mca_ptl_base_match_header_t);
    } else {
	hdr->hdr_common.hdr_type = MCA_PTL_HDR_TYPE_FRAG;
	hdr->hdr_common.hdr_flags = flags;
	hdr->hdr_common.hdr_size = sizeof (mca_ptl_base_frag_header_t);
	hdr->hdr_frag.hdr_frag_offset = offset;
	hdr->hdr_frag.hdr_frag_seq = 0;
        hdr->hdr_frag.hdr_src_ptr.lval = 0;
	hdr->hdr_frag.hdr_src_ptr.pval = desc;
        hdr->hdr_frag.hdr_dst_ptr = pml_req->req_peer_match;
	header_length = sizeof (mca_ptl_base_frag_header_t);
    }

    /* initialize convertor */
    if(size_in > 0) {
	struct iovec iov;
        ompi_convertor_t *convertor;

        if( offset <= mca_ptl_elan_module.super.ptl_first_frag_size ) {
            convertor = &pml_req->req_convertor;
        } else {
            convertor = &desc->frag_convertor;
            ompi_convertor_copy(&pml_req->req_convertor, convertor);
            ompi_convertor_init_for_send( 
                convertor,
                0, 
                pml_req->req_base.req_datatype,
                pml_req->req_base.req_count,
                pml_req->req_base.req_addr,
                offset);
        }

	/* For now, eager sends are always packed into the descriptor
         * TODO: Inline up to 256 bytes (including the header), then
	 *       do a chained send for mesg < first_frag_size */
        iov.iov_base = &desc->buff[header_length];
        iov.iov_len  = size_in;
        rc = ompi_convertor_pack(convertor, &iov, 1);
       	if (rc < 0) {
	    ompi_output (0, "[%s:%d] Unable to pack data\n",
			 __FILE__, __LINE__);
            return;
	}
        size_out = iov.iov_len;
    } else {
	size_out = size_in;
    }

    *size = size_out;
    hdr->hdr_frag.hdr_frag_length = size_out;

    desc->main_dma.dma_srcAddr = MAIN2ELAN (desc->rail->r_ctx,
                                            &desc->buff[0]);

    /* XXX: Hardcoded DMA retry count */
    desc->main_dma.dma_typeSize = E4_DMA_TYPE_SIZE ((header_length +
                                                     size_out),
                                                    DMA_DataTypeByte,
                                                    DMA_QueueWrite, 16);

    desc->main_dma.dma_cookie =
        elan4_local_cookie (ptl->queue->tx_cpool, 
		E4_COOKIE_TYPE_LOCAL_DMA, destvp);

    if (CHECK_ELAN) {
	char hostname[32];

	gethostname(hostname, 32);
	fprintf(stderr, "[%s send...] destvp %d type %d flag %d size %d\n",
		hostname, destvp, hdr->hdr_common.hdr_type,
		hdr->hdr_common.hdr_flags,
		hdr->hdr_common.hdr_size);
    }
	
    desc->main_dma.dma_vproc = destvp;

    /* Make main memory coherent with IO domain (IA64) */
    MEMBAR_VISIBLE ();
    END_FUNC();
}

int
mca_ptl_elan_start_desc (mca_ptl_elan_send_frag_t * desc,
			 struct mca_ptl_elan_peer_t *ptl_peer,
			 struct mca_pml_base_send_request_t *sendreq,
			 size_t offset,
			 size_t *size,
			 int flags)
{
    mca_ptl_elan_module_t *ptl;

    START_FUNC();

    if (desc->desc->desc_type == MCA_PTL_ELAN_DESC_QDMA) {
        struct ompi_ptl_elan_qdma_desc_t *qdma;

        qdma = (ompi_ptl_elan_qdma_desc_t *)desc->desc;
        ptl = qdma->ptl;
	
        mca_ptl_elan_init_qdma_ack (qdma, ptl, ptl_peer, sendreq, 
		offset, size, flags);

        elan4_run_dma_cmd (ptl->queue->tx_cmdq, (DMA *) & qdma->main_dma);

	/*ptl->queue->tx_cmdq->cmdq_flush */
	elan4_flush_cmdq_reorder (ptl->queue->tx_cmdq);

        /* Insert desc into the list of outstanding DMA's */
        ompi_list_append (&ptl->queue->tx_desc, (ompi_list_item_t *) desc);

    } else {
        ompi_output (0,
                     "Other types of DMA are not supported right now \n");
        return OMPI_ERROR;
    }

    /*mca_ptl_base_frag_t frag_base;  */

    /* fragment state */
    desc->frag_base.frag_owner = &ptl_peer->peer_ptl->super;
    desc->frag_base.frag_peer = ptl_peer;
    desc->frag_base.frag_addr = NULL;
    desc->frag_base.frag_size = *size;
    desc->frag_progressed  = 0;
    desc->frag_ack_pending = 0; /* this is ack for internal elan */

    END_FUNC();
    return OMPI_SUCCESS;
}


static void
mca_ptl_elan_data_frag (struct mca_ptl_elan_module_t *ptl,
                        mca_ptl_base_header_t * header)
{
    /* For PML interfacing, refer to mca_ptl_tcp_recv_frag_match(frag, sd);*/

    /* Allocate a recv frag descriptor */
    mca_ptl_elan_recv_frag_t *recv_frag;
    ompi_list_item_t *item;
    mca_pml_base_recv_request_t *request;

    bool        matched; 
    int         rc = OMPI_SUCCESS;

    OMPI_FREE_LIST_GET (&mca_ptl_elan_component.elan_recv_frags_free, 
                        item, rc);

    while (OMPI_SUCCESS != rc) {

	/* TODO: progress the recv state machine */
        ompi_output (0,
                     "[%s:%d] Retry to allocate a recv fragment",
                     __FILE__, __LINE__);
	OMPI_FREE_LIST_GET (&mca_ptl_elan_component.elan_recv_frags_free, 
                item, rc);
    } 

    recv_frag = (mca_ptl_elan_recv_frag_t *) item;
    recv_frag->frag_recv.frag_base.frag_owner = (mca_ptl_base_module_t *) ptl;

    /* XXX: 
     * Since elan is not connection oriented, 
     * No information about which peer until checking the header
     */
    recv_frag->frag_recv.frag_base.frag_peer = NULL;
    recv_frag->frag_recv.frag_request = NULL;
    recv_frag->frag_recv.frag_is_buffered = false;
    recv_frag->frag_hdr_cnt = 0;
    recv_frag->frag_msg_cnt = 0;
    recv_frag->frag_ack_pending = false;
    recv_frag->frag_progressed = 0;

    /* Copy the header, mca_ptl_base_match() does not do what it claims */
    recv_frag->frag_recv.frag_base.frag_header = *header;

    /* Taking the data starting point be default */
    recv_frag->frag_recv.frag_base.frag_addr = 
	(char *) header + sizeof (mca_ptl_base_header_t);
    recv_frag->frag_recv.frag_base.frag_size = header->hdr_frag.hdr_frag_length;

    /* match with preposted requests */
    matched = mca_ptl_base_recv_frag_match (
	    recv_frag->frag_recv.frag_base.frag_owner,
	    &recv_frag->frag_recv,
	    &recv_frag->frag_recv.frag_base.frag_header.hdr_match); 
    
    if (!matched) {
	/* Buffer the fragment into unex buffer
	 * TODO: 
	 *   Next version need to do this only when it is 
	 *   blocking the recv queue */
        memcpy (recv_frag->unex_buff,
                (char *) header + sizeof (mca_ptl_base_header_t),
                header->hdr_frag.hdr_frag_length);
        recv_frag->frag_recv.frag_is_buffered = true;
        recv_frag->frag_recv.frag_base.frag_addr = recv_frag->unex_buff;
    } 
}

static void
mca_ptl_elan_ctrl_frag (struct mca_ptl_elan_module_t *ptl,
                        mca_ptl_base_header_t * header)
{
     /* TODO:
      * 0) First of all, no need to allocate recv frag descriptors, 
      *    since control packet does not contain data.
      * 1) Update the send request, which will start successive fragments 
      *    if it is an ACK and there are more data to go.
      * XXX: Resend if it is a NACK, which layer PML/PTL to handle that?
      */
     mca_ptl_elan_send_frag_t* desc;
     mca_pml_base_send_request_t* req;
 
     desc = (mca_ptl_elan_send_frag_t*) header->hdr_ack.hdr_src_ptr.pval;
     req = (mca_pml_base_send_request_t *) desc->desc->req;
     req->req_peer_match = header->hdr_ack.hdr_dst_match;
 
     /* FIXME: 
      * This sort of synchronized fragment release will lead
      * to race conditions, also see the note insize the follwoing routine */
     mca_ptl_elan_send_desc_done (desc, req); 
}

int
mca_ptl_elan_drain_recv (mca_ptl_elan_component_t * emp)
{
    struct ompi_ptl_elan_queue_ctrl_t *queue;
    ompi_ptl_elan_recv_queue_t *rxq;
    struct mca_ptl_elan_module_t *ptl;
    ELAN_CTX   *ctx;

    int         num_ptl_modules;
    int         i;
    int         rc;

    START_FUNC();
    num_ptl_modules = emp->elan_num_ptl_modules;

    /* Iterate over all the PTL input Queues */
    for (i = 0; i < num_ptl_modules; i++) {

        ptl = emp->elan_ptl_modules[i];
        queue = emp->elan_ptl_modules[i]->queue;
        rxq = queue->rxq;
        ctx = ptl->ptl_elan_ctx;

        OMPI_LOCK (&queue->rx_lock);

#if 0
        rc = (*(int *) (&rxq->qr_doneWord));
#else
        rc = elan4_pollevent_word (ctx, &rxq->qr_doneWord, 2000);
#endif
 
        if (rc) {

            mca_ptl_base_header_t *header;

            header = (mca_ptl_base_header_t *) rxq->qr_fptr;

	    if (CHECK_ELAN) {
		char hostname[32];
		gethostname(hostname, 32);

		fprintf(stderr, 
			"[%s recv...] type %d flag %d size %d\n",
			hostname,
			header->hdr_common.hdr_type,
			header->hdr_common.hdr_flags,
			header->hdr_common.hdr_size);
	    }
	
            switch (header->hdr_common.hdr_type) {
            case MCA_PTL_HDR_TYPE_MATCH:
            case MCA_PTL_HDR_TYPE_FRAG:
                /* a data fragment */
                mca_ptl_elan_data_frag (ptl, header);
                break;
            case MCA_PTL_HDR_TYPE_ACK:
            case MCA_PTL_HDR_TYPE_NACK:
                /* a control fragment for a message */
                mca_ptl_elan_ctrl_frag (ptl, header);
                break;
            default:
                fprintf(stdout, "[%s:%d] unknow fragment type %d\n",
                             __FILE__, __LINE__,
                             header->hdr_common.hdr_type);
		fflush(stdout);
                break;
            }

            /* Work out the new front pointer */
            if (rxq->qr_fptr == rxq->qr_top) {
                rxq->qr_fptr = rxq->qr_base;
                rxq->qr_efptr = rxq->qr_efitem;
            } else {
                rxq->qr_fptr = (void *) ((uintptr_t) rxq->qr_fptr
                                         + queue->rx_slotsize);
                rxq->qr_efptr += queue->rx_slotsize;
            }

            /* PCI Write */
            queue->input->q_fptr = rxq->qr_efptr;

            /* Reset the event */
            RESETEVENT_WORD (&rxq->qr_doneWord);

            /* Order RESETEVENT wrt to wait_event_cmd */
            MEMBAR_STORESTORE ();

            /* Re-prime queue event by issuing a waitevent(1) on it */
            elan4_wait_event_cmd (rxq->qr_cmdq,
                    /* Is qr_elanDone really a main memory address? */
                    MAIN2ELAN (ctx, rxq->qr_elanDone),
                    E4_EVENT_INIT_VALUE (-32, E4_EVENT_WRITE,
                        E4_EVENT_DTYPE_LONG, 0), 
                    MAIN2ELAN (ctx, (void *) &rxq->qr_doneWord),
                    0xfeedfacedeadbeef);

            /*rxq->qr_cmdq->cmdq_flush */
	    elan4_flush_cmdq_reorder (rxq->qr_cmdq);

        }
        OMPI_UNLOCK (&queue->rx_lock);
    }

    END_FUNC();
    return OMPI_SUCCESS;
}

int
mca_ptl_elan_update_desc (mca_ptl_elan_component_t * emp)
{
    struct mca_ptl_elan_module_t *ptl;
    ompi_ptl_elan_queue_ctrl_t *queue;
    mca_ptl_elan_send_frag_t *desc;
    ELAN4_CTX  *ctx;

    int         num_ptl_modules;
    int         i;
    int         rc = 0;

    START_FUNC();

    num_ptl_modules = emp->elan_num_ptl_modules;

    /* Update the send request if any of send's is completed */
    for (i = 0; i < num_ptl_modules; i++) {

        ptl = emp->elan_ptl_modules[i];
        queue = ptl->queue;
        ctx = ptl->ptl_elan_ctx;

	while (ompi_list_get_size (&queue->tx_desc) > 0) {
            desc = (mca_ptl_elan_send_frag_t *)
                ompi_list_get_first (&queue->tx_desc);
#if 0
            rc = * ((int *) (&desc->desc->main_doneWord));
#else
            /* Poll the completion event for 1usec */
            rc = elan4_pollevent_word(ctx, &desc->desc->main_doneWord, 2000);
#endif
	    if (rc) {
		mca_ptl_base_header_t *header;
                mca_ptl_elan_send_request_t *req;
                /* Remove the desc, update the request, put back to free list */
                desc = (mca_ptl_elan_send_frag_t *)
                    ompi_list_remove_first (&queue->tx_desc);
                req = desc->desc->req;
		header = (mca_ptl_base_header_t *)& 
                    ((ompi_ptl_elan_qdma_desc_t *)desc->desc)->buff[0];
		if (CHECK_ELAN) {
		    char hostname[32];
		    gethostname(hostname, 32);

		    fprintf(stderr, 
			    "[%s comp sending...] type %d flag %d size %d\n",
			    hostname,
			    header->hdr_common.hdr_type,
			    header->hdr_common.hdr_flags,
			    header->hdr_common.hdr_size);
		}
 		mca_ptl_elan_send_desc_done (desc, req);
 	    } else {
 		/* XXX: Stop at any incomplete send desc */
		break;
	    }
	} /* end of the while loop */
    } /* end of the for loop */

    END_FUNC();
    return OMPI_SUCCESS;
}

#include "ompi_config.h"
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_FCNTL_H
#include <sys/fcntl.h>
#endif
#include <netinet/in.h>
#include "util/output.h"
#include "mca/oob/base/base.h"
#include "mca/iof/base/base.h"
#include "iof_base_endpoint.h"
#include "iof_base_fragment.h"

/**
 *
 */

static void mca_iof_base_endpoint_construct(mca_iof_base_endpoint_t* endpoint)
{
    endpoint->ep_mode = 0;
    endpoint->ep_state = MCA_IOF_EP_CLOSED;
    endpoint->ep_seq = 0;
    endpoint->ep_ack = 0;
    memset(&endpoint->ep_event,0,sizeof(endpoint->ep_event));
    OBJ_CONSTRUCT(&endpoint->ep_frags, ompi_list_t);
}

static void mca_iof_base_endpoint_destruct(mca_iof_base_endpoint_t* endpoint)
{
    OBJ_DESTRUCT(&endpoint->ep_frags);
}

OBJ_CLASS_INSTANCE(
    mca_iof_base_endpoint_t,
    ompi_list_item_t,
    mca_iof_base_endpoint_construct,
    mca_iof_base_endpoint_destruct);


/*
 *
 */

static void mca_iof_base_endpoint_send_cb(
    int status,
    ompi_process_name_t* peer,
    struct iovec* msg,
    int count,
    int tag,
    void* cbdata)
{
    mca_iof_base_frag_t* frag = (mca_iof_base_frag_t*)cbdata;
    mca_iof_base_endpoint_t* endpoint = frag->frag_owner;
    ompi_list_remove_item(&endpoint->ep_frags, &frag->super);
    MCA_IOF_BASE_FRAG_RETURN(frag);
}


/*
 *  Receive from pipe/pty/etc. and forward to the 
 *  service.
 */

static void mca_iof_base_endpoint_recv_handler(int fd, short flags, void *cbdata)
{
    mca_iof_base_endpoint_t* endpoint = (mca_iof_base_endpoint_t*)cbdata;
    mca_iof_base_frag_t* frag;
    mca_iof_base_header_t* hdr;
    int rc;

    /* allocate a fragment */
    MCA_IOF_BASE_FRAG_ALLOC(frag,rc);
    if(NULL == frag) {
        return;
    }

    OMPI_THREAD_LOCK(&mca_iof_base.iof_lock);
    frag->frag_owner = endpoint;
    ompi_list_append(&endpoint->ep_frags, &frag->super);

    /* read up to the fragment size */
    rc = read(fd, frag->frag_data, sizeof(frag->frag_data));
    if(rc <= 0) {
        /* peer has closed the connection */
        mca_iof_base_endpoint_closed(endpoint);
        rc = 0;
    }
    frag->frag_iov[1].iov_len = frag->frag_len = rc;

    /* fill in the header */
    hdr = &frag->frag_hdr;
    hdr->hdr_common.hdr_type = MCA_IOF_BASE_HDR_MSG;
    hdr->hdr_msg.msg_src = endpoint->ep_name;
    hdr->hdr_msg.msg_tag = endpoint->ep_tag;
    hdr->hdr_msg.msg_seq = endpoint->ep_seq;
    hdr->hdr_msg.msg_len = frag->frag_len;
    MCA_IOF_BASE_HDR_MSG_HTON(hdr->hdr_msg);

    /* if window size has been exceeded - disable forwarding */
    endpoint->ep_seq += frag->frag_len;
    if(MCA_IOF_BASE_SEQDIFF(endpoint->ep_seq,endpoint->ep_ack) > mca_iof_base.iof_window_size) {
        ompi_event_del(&endpoint->ep_event);
    }
    OMPI_THREAD_LOCK(&mca_iof_base.iof_lock);

    /* start non-blocking OOB call to forward received data */
    rc = mca_oob_send_nb(
        mca_iof_base.iof_service, 
        frag->frag_iov, 
        2,
        MCA_OOB_TAG_IOF_SVC,
        0,
        mca_iof_base_endpoint_send_cb,
        frag);
}


static void mca_iof_base_endpoint_send_handler(int sd, short flags, void *user)
{
    mca_iof_base_endpoint_t* endpoint = (mca_iof_base_endpoint_t*)user; 
    OMPI_THREAD_LOCK(&mca_iof_base.iof_lock);
    while(ompi_list_get_size(&endpoint->ep_frags)) {
        mca_iof_base_frag_t* frag = (mca_iof_base_frag_t*)ompi_list_get_first(&endpoint->ep_frags);
        
        int rc = write(endpoint->ep_fd, frag->frag_ptr, frag->frag_len);
        if(rc < 0) {
            mca_iof_base_endpoint_closed(endpoint);
            OMPI_THREAD_UNLOCK(&mca_iof_base.iof_lock);
            return;
        }
        frag->frag_len -= rc;
        frag->frag_ptr += rc;
        if(frag->frag_len > 0) {
            break;
        }
        ompi_list_remove_item(&endpoint->ep_frags, &frag->super);
        mca_iof_base_frag_ack(frag);
    }

    /* is there anything left to write? */
    if(ompi_list_get_size(&endpoint->ep_frags) == 0) {
        ompi_event_del(&endpoint->ep_event);
    }
    OMPI_THREAD_UNLOCK(&mca_iof_base.iof_lock);
}

/*
 * 
 */
 
mca_iof_base_endpoint_t* mca_iof_base_endpoint_lookup(
    const ompi_process_name_t* proc,
    mca_iof_base_mode_t mode,
    int tag)
{
    return NULL;
}


/*
 *
 */

int mca_iof_base_endpoint_create(
    const ompi_process_name_t* proc,
    mca_iof_base_mode_t mode,
    int tag,
    int fd)
{
    mca_iof_base_endpoint_t* endpoint;
    int flags;
 
    /* create local endpoint */
    OMPI_THREAD_LOCK(&mca_iof_base.iof_lock);
    if((endpoint = mca_iof_base_endpoint_lookup(proc,mode,tag)) != NULL) {
        OMPI_THREAD_UNLOCK(&mca_iof_base.iof_lock);
        return OMPI_EXISTS;
    }
    endpoint = OBJ_NEW(mca_iof_base_endpoint_t);
    if(NULL == endpoint) {
        OMPI_THREAD_UNLOCK(&mca_iof_base.iof_lock);
        return OMPI_ERR_OUT_OF_RESOURCE;
    }
    endpoint->ep_name = *proc;
    endpoint->ep_mode = mode;
    endpoint->ep_tag = tag;
    endpoint->ep_fd = fd;

    /* set file descriptor to be non-blocking */
    if((flags = fcntl(fd, F_GETFL, 0)) < 0) {
        ompi_output(0, "mca_iof_base_endpoint_create: fcntl(F_GETFL) failed with errno=%d\n", errno);
    } else {
        flags |= O_NONBLOCK;
        fcntl(fd, F_SETFL, flags);
    }
    
    /* setup event handler */
    switch(mode) {
        case MCA_IOF_SOURCE:
            ompi_event_set(
                &endpoint->ep_event,
                endpoint->ep_fd,
                OMPI_EV_READ|OMPI_EV_PERSIST,
                mca_iof_base_endpoint_recv_handler,
                endpoint);
            ompi_event_add(&endpoint->ep_event, 0);
            break;
        case MCA_IOF_SINK:
            ompi_event_set(
                &endpoint->ep_event,
                endpoint->ep_fd,
                OMPI_EV_WRITE,
                mca_iof_base_endpoint_send_handler,
                endpoint);
            break;
        default:
            ompi_output(0, "mca_iof_base_endpoint_create: invalid mode %d\n", mode);
            return OMPI_ERR_BAD_PARAM;
    }

    ompi_list_append(&mca_iof_base.iof_endpoints, &endpoint->super);
    OMPI_THREAD_UNLOCK(&mca_iof_base.iof_lock);
    return OMPI_SUCCESS;
}


/*
 * 
 */

int mca_iof_base_endpoint_delete(
    const ompi_process_name_t* proc,
    ompi_ns_cmp_bitmask_t mask,
    int tag)
{
    return OMPI_ERROR;
}

/*
 * 
 */

int mca_iof_base_endpoint_close(mca_iof_base_endpoint_t* endpoint)
{ 
    bool closed = false;
    endpoint->ep_state = MCA_IOF_EP_CLOSING;
    switch(endpoint->ep_mode) {
    case MCA_IOF_SOURCE:
        ompi_event_del(&endpoint->ep_event);
        if(endpoint->ep_seq == endpoint->ep_ack) {
            endpoint->ep_state = MCA_IOF_EP_CLOSED;
        }
        break;
    case MCA_IOF_SINK:
        if(ompi_list_get_size(&endpoint->ep_frags) == 0) {
            endpoint->ep_state = MCA_IOF_EP_CLOSED;
        }
        break;
    }
    return OMPI_SUCCESS;
}

/*
 *  Peer has gone away - cleanup and signal SOH monitor.
 */

void mca_iof_base_endpoint_closed(mca_iof_base_endpoint_t* endpoint)
{
}

/*
 *
 */

mca_iof_base_endpoint_t* mca_iof_base_endpoint_match(
    const ompi_process_name_t* dst_name, 
    ompi_ns_cmp_bitmask_t dst_mask,
    int dst_tag)
{
    ompi_list_item_t* item;
    OMPI_THREAD_LOCK(&mca_iof_base.iof_lock);
    for(item =  ompi_list_get_first(&mca_iof_base.iof_endpoints);
        item != ompi_list_get_end(&mca_iof_base.iof_endpoints);
        item =  ompi_list_get_next(item)) {
        mca_iof_base_endpoint_t* endpoint = (mca_iof_base_endpoint_t*)item;
        if(ompi_name_server.compare(dst_mask,dst_name,&endpoint->ep_name) == 0) {
            if(endpoint->ep_tag == dst_tag || endpoint->ep_tag == MCA_IOF_ANY || dst_tag == MCA_IOF_ANY) {
                OBJ_RETAIN(endpoint);
                OMPI_THREAD_UNLOCK(&mca_iof_base.iof_lock);
                return endpoint;
            }
        }
    }
    OMPI_THREAD_UNLOCK(&mca_iof_base.iof_lock);
    return NULL;
}

/*
 *
 */

int mca_iof_base_endpoint_forward(
    mca_iof_base_endpoint_t* endpoint, 
    const ompi_process_name_t* src,
    mca_iof_base_msg_header_t* hdr,
    const unsigned char* data)
{
    mca_iof_base_frag_t* frag;
    size_t len = hdr->msg_len;
    int rc = 0;

    /* allocate and initialize a fragment */
    MCA_IOF_BASE_FRAG_ALLOC(frag, rc);
    if(NULL == frag) {
         return OMPI_ERR_OUT_OF_RESOURCE;
    }

    OMPI_THREAD_LOCK(&mca_iof_base.iof_lock);
    frag->frag_owner = endpoint;
    frag->frag_src = *src;
    frag->frag_hdr.hdr_msg = *hdr;

    /* try to write w/out copying data */
    if(ompi_list_get_size(&endpoint->ep_frags) == 0) {
        rc = write(endpoint->ep_fd,data,len);
        if(rc < 0) {
            mca_iof_base_endpoint_closed(endpoint);
            OMPI_THREAD_UNLOCK(&mca_iof_base.iof_lock);
            return OMPI_SUCCESS;
         } 
    } 

    frag->frag_len = len - rc;
    if(frag->frag_len > 0) {
        /* handle incomplete write */
        frag->frag_ptr = frag->frag_data;
        memcpy(frag->frag_ptr, data+rc, frag->frag_len);
        ompi_list_append(&endpoint->ep_frags, &frag->super);
        if(ompi_list_get_size(&endpoint->ep_frags) == 1) {
            ompi_event_add(&endpoint->ep_event,0);
        }
    } else {
        /* acknowledge fragment */
        mca_iof_base_frag_ack(frag);
    }
    OMPI_THREAD_UNLOCK(&mca_iof_base.iof_lock);
    return OMPI_SUCCESS;
}


/**
 *
 */

int mca_iof_base_endpoint_ack(
    mca_iof_base_endpoint_t* endpoint,
    uint32_t seq)
{
    bool window_closed, window_open;

    OMPI_THREAD_LOCK(&mca_iof_base.iof_lock);
    window_closed =
            MCA_IOF_BASE_SEQDIFF(endpoint->ep_seq,endpoint->ep_ack) >= mca_iof_base.iof_window_size;
    endpoint->ep_ack = seq;
    window_open =
            MCA_IOF_BASE_SEQDIFF(endpoint->ep_seq,endpoint->ep_ack) < mca_iof_base.iof_window_size;
                                                                                                              
    /* if we are shutting down - cleanup endpoint */
    if(endpoint->ep_state == MCA_IOF_EP_CLOSING) {
        if(endpoint->ep_seq == endpoint->ep_ack) {
            endpoint->ep_state = MCA_IOF_EP_CLOSED;
            ompi_condition_signal(&mca_iof_base.iof_condition);
        }

    /* otherwise check to see if we can reenable forwarding */
    } else if(window_closed && window_open) {
        ompi_event_add(&endpoint->ep_event, 0);
    }
    OMPI_THREAD_UNLOCK(&mca_iof_base.iof_lock);
    return OMPI_SUCCESS;
}


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
/** @file:
 *
 * the oob framework
 */

#ifndef _MCA_OOB_BASE_H_
#define _MCA_OOB_BASE_H_
#include "mca/mca.h"
#include "mca/ns/ns.h"
#ifdef HAVE_SYS_UIO_H
	#include <sys/uio.h>
#endif
#include "util/bufpack.h"

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

/*
 * Well known address
 */

OMPI_DECLSPEC extern ompi_process_name_t mca_oob_name_any;
OMPI_DECLSPEC extern ompi_process_name_t mca_oob_name_seed;
OMPI_DECLSPEC extern ompi_process_name_t mca_oob_name_self;

/**
 * The wildcard for receives from any peer.
 */
#define MCA_OOB_NAME_ANY  &mca_oob_name_any
/**
 * Process name of self
 */
#define MCA_OOB_NAME_SELF &mca_oob_name_self
/**
 * Process name of seed
 */
#define MCA_OOB_NAME_SEED &mca_oob_name_seed


/*
 * Other constants
 */
/**
 * Receive from any tag.
 */
#define MCA_OOB_TAG_ANY 0
/**
 * Service tags
 */
#define MCA_OOB_TAG_XCAST      -1
#define MCA_OOB_TAG_NS          1
#define MCA_OOB_TAG_GPR         2
#define MCA_OOB_TAG_GPR_NOTIFY  3
#define MCA_OOB_TAG_RTE         4
#define MCA_OOB_TAG_EXEC        5
#define MCA_OOB_TAG_DAEMON      6
#define MCA_OOB_TAG_STDIO       7
#define MCA_OOB_TAG_SCHED       8
#define MCA_OOB_TAG_PCM_KILL    9
#define MCA_OOB_TAG_USER        1000 /* user defined tags should be assigned above this level */

/*
 * OOB API 
 */

/**
*   General flags for send/recv
* 
*   An example of usage - to determine the size of the next available message w/out receiving it:
*
*   int size = mca_oob_recv(name, 0, 0, MCA_OOB_TRUNC|MCA_OOB_PEEK);
*/

#define MCA_OOB_PEEK  0x01   /**< flag to oob_recv to allow caller to peek a portion of the next available
                              * message w/out removing the message from the queue.  */
#define MCA_OOB_TRUNC 0x02   /**< flag to oob_recv to return the actual size of the message even if 
                              * the receive buffer is smaller than the number of bytes available */
#define MCA_OOB_ALLOC 0x04   /**< flag to oob_recv to request the oob to allocate a buffer of the appropriate
                              * size for the receive and return the allocated buffer and size in the first
                              * element of the iovec array. */

        
/**
*  Obtain a string representation of the OOB contact information for
*  the selected OOB channels. This string may be passed to another 
*  application via an MCA parameter (OMPI_MCA_oob_base_seed) to bootstrap
*  communications.
*
*  @return  A null terminated string that should be freed by the caller.
*
*  Note that mca_oob_base_init() must be called to load and select
*  an OOB module prior to calling this routine.
*/

OMPI_DECLSPEC char* mca_oob_get_contact_info(void);

/**
*  Pre-populate the cache of contact information required by the OOB 
*  to reach a given destination. This is required to setup a pointer
*  to initial registry/name server/etc.
*
*  @param  uri   The contact information of the peer process obtained
*  via a call to mca_oob_get_contact_info().
*
*/

OMPI_DECLSPEC int mca_oob_set_contact_info(const char*);

/**
 *  A routine to ping a given process name to determine if it is reachable. 
 *
 *  @param  name  The peer name.
 *  @param  tv    The length of time to wait on a connection/response.
 *  
 *  Note that this routine blocks up to the specified timeout waiting for a
 *  connection / response from the specified peer. If the peer is unavailable
 *  an error status is returned.
 */

OMPI_DECLSPEC int mca_oob_ping(ompi_process_name_t* name, struct timeval* tv);

/**
 *  A barrier across all processes w/in the same job.
 */

OMPI_DECLSPEC int mca_oob_barrier(void);

/**
*  Extract from the contact info the peer process identifier.
*
*  @param  cinfo (IN)   The contact information of the peer process.
*  @param  name (OUT)   The peer process identifier.
*  @param  uris (OUT)   Will return an array of uri strings corresponding
*                       to the peers exported protocols.
*              
*  Note the caller may pass NULL for the uris if they only wish to extact
*  the process name.
*/

OMPI_DECLSPEC int mca_oob_parse_contact_info(const char* uri, ompi_process_name_t* peer, char*** uris);


/**
 *  Set the contact info for the seed daemon.
 *
 *  Note that this can also be passed to the application as an
 *  MCA parameter (OMPI_MCA_oob_base_seed). The contact info (of the seed)
 *  must currently be set before calling mca_oob_base_init().
 */

OMPI_DECLSPEC int mca_oob_set_contact_info(const char*);

/**
*  Similiar to unix writev(2).
*
*  @param peer (IN)    Opaque name of peer process.
*  @param msg (IN)     Array of iovecs describing user buffers and lengths.
*  @param count (IN)   Number of elements in iovec array.
*  @param tag (IN)     User defined tag for matching send/recv.
*  @param flags (IN)   Currently unused.
*  @return             OMPI error code (<0) on error number of bytes actually sent.
*
*  This routine provides semantics similar to unix send/writev with the addition of
*  a tag parameter that can be used by the application to match the send w/ a specific
*  receive. In other words - a recv call by the specified peer will only succeed when
*  the corresponding (or wildcard) tag is used.
*
*  The <i>peer</i> parameter represents an opaque handle to the peer process that
*  is resolved by the oob layer (using the registry) to an actual physical network
*  address.
*/

OMPI_DECLSPEC int mca_oob_send(
    ompi_process_name_t* peer, 
    struct iovec *msg, 
    int count, 
    int tag,
    int flags);

/*
*  Similiar to unix send(2) and mca_oob_send.
*
* @param peer (IN)   Opaque name of peer process.
* @param buffer (IN) Prepacked OMPI_BUFFER containing data to send
* @param flags (IN)  Currently unused.
* @return            OMPI error code (<0) on error or number of bytes actually sent.
*/

OMPI_DECLSPEC int mca_oob_send_packed(
    ompi_process_name_t* peer, 
    ompi_buffer_t buffer, 
    int tag, 
    int flags);


/**
* Similiar to unix readv(2)
*
* @param peer (IN/OUT)    Opaque name of peer process or MCA_OOB_NAME_ANY for wildcard receive. In the
*                         case of a wildcard receive, will be modified to return the matched peer name.
* @param msg (IN)         Array of iovecs describing user buffers and lengths.
* @param count (IN)       Number of elements in iovec array.
* @param tag (IN/OUT)     User defined tag for matching send/recv. In the case of a wildcard receive, will
*                         be modified to return the matched tag. May be optionally by NULL to specify a 
*                         wildcard receive with no return value.
* @param flags (IN)       May be MCA_OOB_PEEK to return up to the number of bytes provided in the
*                         iovec array without removing the message from the queue.
* @return                 OMPI error code (<0) on error or number of bytes actually received.
*
* The OOB recv call is similar to unix recv/readv in that it requires the caller to manage
* memory associated w/ the message. The routine accepts an array of iovecs (<i>msg</i>); however,
* the caller must determine the appropriate number of elements (<i>count</i>) and allocate the
* buffer space for each entry. 
*
* The <i>tag</i> parameter is provided to facilitate this. The user may define tags based on message 
* type to determine the message layout and size, as the mca_oob_recv call will block until a message
* with the matching tag is received.
*
* Alternately, the <i>flags</i> parameter may be used to peek (MCA_OOB_PEEK) a portion of the message 
* (e.g. a standard message header) or determine the overall message size (MCA_OOB_TRUNC|MCA_OOB_PEEK)
* without removing the message from the queue. 
*
*/

OMPI_DECLSPEC int mca_oob_recv(
    ompi_process_name_t* peer, 
    struct iovec *msg, 
    int count, 
    int* tag, 
    int flags);

/**
* Similiar to unix read(2)
*
* @param peer (IN)    Opaque name of peer process or MCA_OOB_NAME_ANY for wildcard receive.
* @param buf (OUT)    Array of iovecs describing user buffers and lengths.
* @param tag (IN/OUT) User defined tag for matching send/recv.
* @return             OMPI error code (<0) on error or number of bytes actually received.
*
*
* This version of oob_recv is as above except it does NOT take a iovec list
* but instead hands back a ompi_buffer_t buffer with the message in it. 
* The user is responsible for freeing this buffer with ompi_buffer_free()
* when finished.
*
*
*/

OMPI_DECLSPEC int mca_oob_recv_packed (
	ompi_process_name_t* peer, 
	ompi_buffer_t *buf, 
	int* tag);

/*
 * Non-blocking versions of send/recv.
*/


/**
*  Callback function on send/recv completion.
*
*  @param status (IN)  Completion status - equivalent to the return value from blocking send/recv.
*  @param peer (IN)    Opaque name of peer process.
*  @param msg (IN)     Array of iovecs describing user buffers and lengths.
*  @param count (IN)   Number of elements in iovec array.
*  @param tag (IN)     User defined tag for matching send/recv.
*  @param cbdata (IN)  User data.
*/

typedef void (*mca_oob_callback_fn_t)(
    int status,
    ompi_process_name_t* peer, 
    struct iovec* msg, 
    int count,
    int tag,
    void* cbdata);

/**
*  Callback function on send/recv completion for buffer PACKED message only.
*  i.e. only mca_oob_send_packed_nb and mca_oob_recv_packed_nb USE this.
*
*  @param status (IN)  Completion status - equivalent to the return value from blocking send/recv.
*  @param peer (IN)    Opaque name of peer process.
*  @param buffer (IN)  For sends, this is a pointer to a prepacked buffer
                       For recvs, OOB creates and returns a buffer 
*  @param tag (IN)     User defined tag for matching send/recv.
*  @param cbdata (IN)  User data.
*/

typedef void (*mca_oob_callback_packed_fn_t)(
    int status,
    ompi_process_name_t* peer, 
    ompi_buffer_t buffer,
    int tag,
    void* cbdata);

/**
*  Non-blocking version of mca_oob_send().
*
*  @param peer (IN)    Opaque name of peer process.
*  @param msg (IN)     Array of iovecs describing user buffers and lengths.
*  @param count (IN)   Number of elements in iovec array.
*  @param tag (IN)     User defined tag for matching send/recv.
*  @param flags (IN)   Currently unused.
*  @param cbfunc (IN)  Callback function on send completion.
*  @param cbdata (IN)  User data that is passed to callback function.
*  @return             OMPI error code (<0) on error number of bytes actually sent.
*
*  The user supplied callback function is called when the send completes. Note that
*  the callback may occur before the call to mca_oob_send returns to the caller, 
*  if the send completes during the call.
*
*/

OMPI_DECLSPEC int mca_oob_send_nb(
    ompi_process_name_t* peer, 
    struct iovec* msg, 
    int count, 
    int tag,
    int flags, 
    mca_oob_callback_fn_t cbfunc,
    void* cbdata);

/**
*  Non-blocking version of mca_oob_send_packed().
*
*  @param peer (IN)    Opaque name of peer process.
*  @param buffer (IN)  Opaque buffer handle.
*  @param tag (IN)     User defined tag for matching send/recv.
*  @param flags (IN)   Currently unused.
*  @param cbfunc (IN)  Callback function on send completion.
*  @param cbdata (IN)  User data that is passed to callback function.
*  @return             OMPI error code (<0) on error number of bytes actually sent.
*
*  The user supplied callback function is called when the send completes. Note that
*  the callback may occur before the call to mca_oob_send returns to the caller, 
*  if the send completes during the call.
*
*/

OMPI_DECLSPEC int mca_oob_send_packed_nb(
    ompi_process_name_t* peer, 
    ompi_buffer_t buffer,
    int tag,
    int flags, 
    mca_oob_callback_packed_fn_t cbfunc,
    void* cbdata);

/**
* Non-blocking version of mca_oob_recv().
*
* @param peer (IN)    Opaque name of peer process or MCA_OOB_NAME_ANY for wildcard receive.
* @param msg (IN)     Array of iovecs describing user buffers and lengths.
* @param count (IN)   Number of elements in iovec array.
* @param tag (IN)     User defined tag for matching send/recv.
* @param flags (IN)   May be MCA_OOB_PEEK to return up to size bytes of msg w/out removing it from the queue,
* @param cbfunc (IN)  Callback function on recv completion.
* @param cbdata (IN)  User data that is passed to callback function.
* @return             OMPI error code (<0) on error or number of bytes actually received.
*
* The user supplied callback function is called asynchronously when a message is received 
* that matches the call parameters.
*/

OMPI_DECLSPEC int mca_oob_recv_nb(
    ompi_process_name_t* peer, 
    struct iovec* msg,  
    int count, 
    int tag,
    int flags, 
    mca_oob_callback_fn_t cbfunc,
    void* cbdata);

/**
* Routine to cancel pending non-blocking recvs.
*
* @param peer (IN)    Opaque name of peer process or MCA_OOB_NAME_ANY for wildcard receive.
* @param tag (IN)     User defined tag for matching send/recv.
* @return             OMPI error code (<0) on error or number of bytes actually received.
*/

OMPI_DECLSPEC int mca_oob_recv_cancel(
    ompi_process_name_t* peer, 
    int tag);

/**
* Non-blocking version of mca_oob_recv_packed().
*
* @param peer (IN)    Opaque name of peer process or MCA_OOB_NAME_ANY for wildcard receive.
* @param buffer (IN)  Array of iovecs describing user buffers and lengths.
* @param count (IN)   Number of elements in iovec array.
* @param tag (IN)     User defined tag for matching send/recv.
* @param flags (IN)   May be MCA_OOB_PEEK to return up to size bytes of msg w/out removing it from the queue,
* @param cbfunc (IN)  Callback function on recv completion.
* @param cbdata (IN)  User data that is passed to callback function.
* @return             OMPI error code (<0) on error or number of bytes actually received.
*
* The user supplied callback function is called asynchronously when a message is received 
* that matches the call parameters.
*/

OMPI_DECLSPEC int mca_oob_recv_packed_nb(
    ompi_process_name_t* peer, 
    int tag,
    int flags, 
    mca_oob_callback_packed_fn_t cbfunc,
    void* cbdata);

/**
 *  A "broadcast-like" function over the specified set of peers. 
 *  @param  root    The process acting as the root of the broadcast.
 *  @param  peers   The list of processes receiving the broadcast (excluding root).
 *  @param  buffer  The data to broadcast - only significant at root.
 *  @param  cbfunc  Callback function on receipt of data - not significant at root.
 * 
 *  Note that the callback function is provided so that the data can be
 *  received and interpreted by the application prior to the broadcast
 *  continuing to forward data along the distribution tree.
 */

OMPI_DECLSPEC int mca_oob_xcast(
    ompi_process_name_t* root, 
    ompi_list_t* peers,
    ompi_buffer_t buffer,
    mca_oob_callback_packed_fn_t cbfunc);

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif

#endif


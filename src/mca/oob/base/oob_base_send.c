#include "mca/oob/oob.h"
#include "mca/oob/base/base.h"
#include <string.h>
#include <netinet/in.h>
/*
*  Similiar to unix send(2).
*
* @param peer (IN)   Opaque name of peer process.
* @param msg (IN)    Array of iovecs describing user buffers and lengths.
* @param count (IN)  Number of elements in iovec array.
* @param flags (IN)  Currently unused.
* @return            OMPI error code (<0) on error or number of bytes actually sent.
*/
                                                                                                         
int mca_oob_send(ompi_process_name_t* peer, struct iovec *msg, int count, int tag, int flags)
{
    return(mca_oob.oob_send(peer, msg, count, tag, flags));
}

/*
*  Similiar to unix send(2) and mca_oob_send.
*
* @param peer (IN)   Opaque name of peer process.
* @param buffer (IN) Prepacked OMPI_BUFFER containing data to send
* @param flags (IN)  Currently unused.
* @return            OMPI error code (<0) on error or number of bytes actually sent.
*/
                                                                                                         
int mca_oob_send_packed (ompi_process_name_t* peer, ompi_buffer_t buffer, int tag, int flags)
{
void *dataptr;
size_t datalen;
struct iovec msg[1];
int rc;

    /* first build iovec from buffer information */
    rc = ompi_buffer_size (buffer, &datalen);
    if (OMPI_ERROR==rc) { return (rc); }

    rc = ompi_buffer_get_ptrs (buffer, NULL, NULL, &dataptr);
    if (OMPI_ERROR==rc) { return (rc); }

    msg[0].iov_base = dataptr;
    msg[0].iov_len  = datalen;

    return(mca_oob.oob_send(peer, msg, 1, tag, flags));
}
 

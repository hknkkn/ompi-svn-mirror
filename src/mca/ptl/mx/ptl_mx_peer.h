/*
 * $HEADER$
 */
/**
 * @file
 */
#ifndef MCA_PTL_MX_PEER_H
#define MCA_PTL_MX_PEER_H

#include "ompi_config.h"
#include <myriexpress.h>

/**
 *  An abstraction that represents a a peer process.
*/
struct mca_ptl_base_peer_t {
    ompi_list_item_t peer_item;
    mx_endpoint_addr_t peer_addr;
    struct mca_ptl_mx_module_t* peer_ptl;
    struct mca_ptl_mx_proc_t* peer_proc;
    bool peer_byte_swap;
};
typedef struct mca_ptl_base_peer_t mca_ptl_base_peer_t;
typedef struct mca_ptl_base_peer_t mca_ptl_mx_peer_t;

OBJ_CLASS_DECLARATION(mca_ptl_mx_peer_t);


#endif


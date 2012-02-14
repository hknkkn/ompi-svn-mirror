/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2006 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2011 Cisco Systems, Inc.  All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

#ifndef MCA_BTL_TCP_PROC_H
#define MCA_BTL_TCP_PROC_H

#include "opal/class/opal_object.h"
#include "ompi/proc/proc.h"
#include "orte/types.h"
#include "btl_tcp2.h"
#include "btl_tcp2_addr.h"
#include "btl_tcp2_endpoint.h"

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

/**
 * Represents the state of a remote process and the set of addresses
 * that it exports. Also cache an instance of mca_btl_base_endpoint_t for
 * each
 * BTL instance that attempts to open a connection to the process.
 */
struct mca_btl_tcp2_proc_t {
    opal_list_item_t super;                  
    /**< allow proc to be placed on a list */

    ompi_proc_t *proc_ompi;                  
    /**< pointer to corresponding ompi_proc_t */

    orte_process_name_t proc_name;           
    /**< globally unique identifier for the process */

    struct mca_btl_tcp2_addr_t* proc_addrs;
    /**< array of addresses exported by peer */

    size_t proc_addr_count;                  
    /**< number of addresses published by endpoint */

    struct mca_btl_base_endpoint_t **proc_endpoints; 
    /**< array of endpoints that have been created to access this proc */    

    size_t proc_endpoint_count;                  
    /**< number of endpoints */

    opal_mutex_t proc_lock;                  
    /**< lock to protect against concurrent access to proc state */
};
typedef struct mca_btl_tcp2_proc_t mca_btl_tcp2_proc_t;
OBJ_CLASS_DECLARATION(mca_btl_tcp2_proc_t);

/*	the highest possible interface kernel index we can handle */
#define MAX_KERNEL_INTERFACE_INDEX 65536

/*	the maximum number of kernel interfaces we can handle */
#define MAX_KERNEL_INTERFACES 8

/*
 * FIXME: this should probably be part of an ompi list, so we need the
 * appropriate definitions
 */

struct mca_btl_tcp2_interface_t {
	struct sockaddr_storage* ipv4_address;
	struct sockaddr_storage* ipv6_address;
	mca_btl_tcp2_addr_t* ipv4_endpoint_addr;
	mca_btl_tcp2_addr_t* ipv6_endpoint_addr;
	uint32_t ipv4_netmask;
	uint32_t ipv6_netmask;
	int kernel_index;
	int peer_interface;
	int index;
	int inuse;
};

typedef struct mca_btl_tcp2_interface_t mca_btl_tcp2_interface_t;

/*
 * describes the quality of a possible connection between a local and
 * a remote network interface
 */
enum mca_btl_tcp2_connection_quality { 
	CQ_NO_CONNECTION,
	CQ_PRIVATE_DIFFERENT_NETWORK,
	CQ_PRIVATE_SAME_NETWORK,
	CQ_PUBLIC_DIFFERENT_NETWORK,
	CQ_PUBLIC_SAME_NETWORK
};


mca_btl_tcp2_proc_t* mca_btl_tcp2_proc_create(ompi_proc_t* ompi_proc);
mca_btl_tcp2_proc_t* mca_btl_tcp2_proc_lookup(const orte_process_name_t* name);
int  mca_btl_tcp2_proc_insert(mca_btl_tcp2_proc_t*, mca_btl_base_endpoint_t*);
int  mca_btl_tcp2_proc_remove(mca_btl_tcp2_proc_t*, mca_btl_base_endpoint_t*);
bool mca_btl_tcp2_proc_accept(mca_btl_tcp2_proc_t*, struct sockaddr*, int);
bool mca_btl_tcp2_proc_tosocks(mca_btl_tcp2_addr_t*, struct sockaddr_storage*);

/**
 * Inlined function to return local TCP proc instance.
 */

static inline mca_btl_tcp2_proc_t* mca_btl_tcp2_proc_local(void)
{
    if(NULL == mca_btl_tcp2_component.tcp_local)
        mca_btl_tcp2_component.tcp_local = mca_btl_tcp2_proc_create(ompi_proc_local());
    return mca_btl_tcp2_component.tcp_local;
}

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif
#endif

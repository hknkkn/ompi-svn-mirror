/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University.
 *                         All rights reserved.
 * Copyright (c) 2004-2005 The Trustees of the University of Tennessee.
 *                         All rights reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */
/**
 * @file
 */
#ifndef MCA_PML_COMM_H
#define MCA_PML_COMM_H

#include "opal/threads/mutex.h"
#include "opal/threads/condition.h"
#include "mca/ptl/ptl.h"
#include "opal/class/opal_list.h"
#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif
OMPI_DECLSPEC extern opal_class_t mca_pml_ptl_comm_t_class;

/**
 *  Cached on ompi_communicator_t to hold queues/state
 *  used by the PML<->PTL interface for matching logic. 
 */
struct mca_pml_comm_t {
    opal_object_t super;
    uint32_t *c_msg_seq;               /**< send message sequence number - sender side */
    uint16_t *c_next_msg_seq;          /**< send message sequence number - receiver side */
    mca_ptl_sequence_t c_recv_seq;     /**< recv request sequence number - receiver side */
    opal_mutex_t c_matching_lock;      /**< matching lock */
    opal_list_t *c_unexpected_frags;   /**< unexpected fragment queues */
    opal_list_t *c_frags_cant_match;   /**< out-of-order fragment queues */
    opal_list_t *c_specific_receives;  /**< queues of unmatched specific (source process specified) receives */
    opal_list_t c_wild_receives;       /**< queue of unmatched wild (source process not specified) receives */
};
typedef struct mca_pml_comm_t mca_pml_ptl_comm_t;


/**
 * Initialize an instance of mca_pml_ptl_comm_t based on the communicator size.
 *
 * @param  comm   Instance of mca_pml_ptl_comm_t
 * @param  size   Size of communicator 
 * @return        OMPI_SUCCESS or error status on failure.
 */

OMPI_DECLSPEC extern int mca_pml_ptl_comm_init_size(mca_pml_ptl_comm_t* comm, size_t size);

/**
 * Obtain the next sequence number (MPI) for a given destination rank.
 *
 * @param  comm   Instance of mca_pml_ptl_comm_t
 * @param  dst    Rank of destination.
 * @return        Next available sequence number.
 */

static inline mca_ptl_sequence_t mca_pml_ptl_comm_send_sequence(mca_pml_ptl_comm_t* comm, int dst)
{
   volatile int32_t *msg_seq = (volatile int32_t*)(comm->c_msg_seq+dst);
   return (mca_ptl_sequence_t)OPAL_THREAD_ADD32(msg_seq, 1)-1;
}

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif
#endif


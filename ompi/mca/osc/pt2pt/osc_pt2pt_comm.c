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

#include "ompi_config.h"
#include "mpi.h"

#include <stdio.h>

#include "osc_pt2pt.h"
#include "osc_pt2pt_sendreq.h"

static int
enqueue_sendreq(ompi_osc_pt2pt_module_t *module,
                ompi_osc_pt2pt_sendreq_t *sendreq)
{
    OPAL_THREAD_LOCK(&(module->p2p_lock));
    opal_list_append(&(module->p2p_pending_sendreqs),
                     (opal_list_item_t*) sendreq);
    module->p2p_num_pending_sendreqs[sendreq->req_target_rank]++;
    OPAL_THREAD_UNLOCK(&(module->p2p_lock));

    return OMPI_SUCCESS;
}



int
ompi_osc_pt2pt_module_accumulate(void *origin_addr, int origin_count,
                                 struct ompi_datatype_t *origin_dt,
                                 int target, int target_disp, int target_count,
                                 struct ompi_datatype_t *target_dt,
                                 struct ompi_op_t *op, ompi_win_t *win)
{
    int ret;
    ompi_osc_pt2pt_sendreq_t *sendreq;

    if (!ompi_ddt_is_predefined(target_dt)) {
        fprintf(stderr, "MPI_Accumulate currently does not support reductions\n");
        fprintf(stderr, "with any user-defined types.  This will be rectified\n");
        fprintf(stderr, "in a future release.\n");
        return MPI_ERR_UNSUPPORTED_OPERATION;
    }

    /* create sendreq */
    ret = ompi_osc_pt2pt_sendreq_alloc_init(OMPI_OSC_PT2PT_ACC,
                                            origin_addr,
                                            origin_count,
                                            origin_dt,
                                            target,
                                            target_disp,
                                            target_count,
                                            target_dt,
                                            P2P_MODULE(win),
                                            &sendreq);
    if (OMPI_SUCCESS != ret) return ret;

    sendreq->req_op_id = op->o_f_to_c_index;

    /* enqueue sendreq */
    ret = enqueue_sendreq(P2P_MODULE(win), sendreq);

    return ret;
}


int
ompi_osc_pt2pt_module_get(void *origin_addr,
                          int origin_count,
                          struct ompi_datatype_t *origin_dt,
                          int target,
                          int target_disp,
                          int target_count,
                          struct ompi_datatype_t *target_dt,
                          ompi_win_t *win)
{
    int ret;
    ompi_osc_pt2pt_sendreq_t *sendreq;

    /* create sendreq */
    ret = ompi_osc_pt2pt_sendreq_alloc_init(OMPI_OSC_PT2PT_GET,
                                            origin_addr,
                                            origin_count,
                                            origin_dt,
                                            target,
                                            target_disp,
                                            target_count,
                                            target_dt,
                                            P2P_MODULE(win),
                                            &sendreq);
    if (OMPI_SUCCESS != ret) return ret;

    /* enqueue sendreq */
    ret = enqueue_sendreq(P2P_MODULE(win), sendreq);

    return ret;
}


int
ompi_osc_pt2pt_module_put(void *origin_addr, int origin_count,
                          struct ompi_datatype_t *origin_dt,
                          int target, int target_disp, int target_count,
                          struct ompi_datatype_t *target_dt, ompi_win_t *win)
{
    int ret;
    ompi_osc_pt2pt_sendreq_t *sendreq;

    /* create sendreq */
    ret = ompi_osc_pt2pt_sendreq_alloc_init(OMPI_OSC_PT2PT_PUT,
                                            origin_addr,
                                            origin_count,
                                            origin_dt,
                                            target,
                                            target_disp,
                                            target_count,
                                            target_dt,
                                            P2P_MODULE(win),
                                            &sendreq);
    if (OMPI_SUCCESS != ret) return ret;

    /* enqueue sendreq */
    ret = enqueue_sendreq(P2P_MODULE(win), sendreq);

    return ret;
}

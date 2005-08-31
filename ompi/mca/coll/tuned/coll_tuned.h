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

#ifndef MCA_COLL_TUNED_EXPORT_H
#define MCA_COLL_TUNED_EXPORT_H

#include "ompi_config.h"

#include "mpi.h"
#include "mca/mca.h"
#include "mca/coll/coll.h"
#include "request/request.h"
#include "mca/pml/pml.h"

/* need to include our own topo prototypes so we can malloc data on the comm correctly */
#include "coll_tuned_topo.h"

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

/*
 * Globally exported variable
 */

OMPI_COMP_EXPORT extern const mca_coll_base_component_1_0_0_t mca_coll_tuned_component;
OMPI_COMP_EXPORT extern int mca_coll_tuned_priority_param;


/*
 * coll API functions
 */


  /* API functions */

  int mca_coll_tuned_init_query(bool enable_progress_threads,
                                bool enable_mpi_threads);
  const struct mca_coll_base_module_1_0_0_t *
    mca_coll_tuned_comm_query(struct ompi_communicator_t *comm, int *priority,
                              struct mca_coll_base_comm_t **data);

  const struct mca_coll_base_module_1_0_0_t *
    mca_coll_tuned_module_init(struct ompi_communicator_t *comm);
  int mca_coll_tuned_module_finalize(struct ompi_communicator_t *comm);

  /* API functions of decision functions and any implementations */

  int mca_coll_tuned_allgather_intra_dec(void *sbuf, int scount, 
                                     struct ompi_datatype_t *sdtype, 
                                     void *rbuf, int rcount, 
                                     struct ompi_datatype_t *rdtype, 
                                     struct ompi_communicator_t *comm);
  int mca_coll_tuned_allgather_inter_dec(void *sbuf, int scount, 
                                     struct ompi_datatype_t *sdtype, 
                                     void *rbuf, int rcount, 
                                     struct ompi_datatype_t *rdtype, 
                                     struct ompi_communicator_t *comm);

  int mca_coll_tuned_allgatherv_intra_dec(void *sbuf, int scount, 
                                      struct ompi_datatype_t *sdtype, 
                                      void * rbuf, int *rcounts, int *disps, 
                                      struct ompi_datatype_t *rdtype, 
                                      struct ompi_communicator_t *comm);
  int mca_coll_tuned_allgatherv_inter_dec(void *sbuf, int scount, 
                                      struct ompi_datatype_t *sdtype, 
                                      void * rbuf, int *rcounts, int *disps, 
                                      struct ompi_datatype_t *rdtype, 
                                      struct ompi_communicator_t *comm);

  int mca_coll_tuned_allreduce_intra_dec(void *sbuf, void *rbuf, int count, 
                                     struct ompi_datatype_t *dtype, 
                                     struct ompi_op_t *op, 
                                     struct ompi_communicator_t *comm);
  int mca_coll_tuned_allreduce_inter_dec(void *sbuf, void *rbuf, int count, 
                                     struct ompi_datatype_t *dtype, 
                                     struct ompi_op_t *op, 
                                     struct ompi_communicator_t *comm);

  int mca_coll_tuned_alltoall_intra_dec(void *sbuf, int scount, 
                                    struct ompi_datatype_t *sdtype, 
                                    void* rbuf, int rcount, 
                                    struct ompi_datatype_t *rdtype, 
                                    struct ompi_communicator_t *comm);
  int mca_coll_tuned_alltoall_inter_dec(void *sbuf, int scount, 
                                    struct ompi_datatype_t *sdtype, 
                                    void* rbuf, int rcount, 
                                    struct ompi_datatype_t *rdtype, 
                                    struct ompi_communicator_t *comm);

  int mca_coll_tuned_alltoallv_intra_dec(void *sbuf, int *scounts, int *sdisps, 
                                     struct ompi_datatype_t *sdtype, 
                                     void *rbuf, int *rcounts, int *rdisps, 
                                     struct ompi_datatype_t *rdtype, 
                                     struct ompi_communicator_t *comm);
  int mca_coll_tuned_alltoallv_inter_dec(void *sbuf, int *scounts, int *sdisps, 
                                     struct ompi_datatype_t *sdtype, 
                                     void *rbuf, int *rcounts, int *rdisps, 
                                     struct ompi_datatype_t *rdtype, 
                                     struct ompi_communicator_t *comm);

  int mca_coll_tuned_alltoallw_intra_dec(void *sbuf, int *scounts, int *sdisps, 
                                     struct ompi_datatype_t **sdtypes, 
                                     void *rbuf, int *rcounts, int *rdisps, 
                                     struct ompi_datatype_t **rdtypes, 
                                     struct ompi_communicator_t *comm);
  int mca_coll_tuned_alltoallw_inter_dec(void *sbuf, int *scounts, int *sdisps, 
                                     struct ompi_datatype_t **sdtypes, 
                                     void *rbuf, int *rcounts, int *rdisps, 
                                     struct ompi_datatype_t **rdtypes, 
                                     struct ompi_communicator_t *comm);

  int mca_coll_tuned_barrier_intra_dec(struct ompi_communicator_t *comm);
  int mca_coll_tuned_barrier_inter_dec(struct ompi_communicator_t *comm);


  int mca_coll_tuned_bcast_intra_dec(void *buff, int count, 
                                     struct ompi_datatype_t *datatype,
                                     int root, 
                                     struct ompi_communicator_t *comm);

  int mca_coll_tuned_bcast_intra_linear(void *buff, int count, 
                                     struct ompi_datatype_t *datatype,
                                     int root, 
                                     struct ompi_communicator_t *comm);

  int mca_coll_tuned_bcast_intra_chain(void *buff, int count, 
                                     struct ompi_datatype_t *datatype,
                                     int root, 
                                     struct ompi_communicator_t *comm,
                                     uint32_t segsize, int32_t chains);

  int mca_coll_tuned_bcast_intra_pipeline(void *buff, int count, 
                                     struct ompi_datatype_t *datatype,
                                     int root, 
                                     struct ompi_communicator_t *comm,
                                     uint32_t segsize);

  int mca_coll_tuned_bcast_intra_bmtree(void *buff, int count, 
                                     struct ompi_datatype_t *datatype,
                                     int root, 
                                     struct ompi_communicator_t *comm,
                                     uint32_t segsize, int32_t chains);

  int mca_coll_tuned_bcast_intra_bintree(void *buff, int count, 
                                     struct ompi_datatype_t *datatype,
                                     int root, 
                                     struct ompi_communicator_t *comm,
                                     uint32_t segsize);




  int mca_coll_tuned_bcast_inter_dec(void *buff, int count, 
                                     struct ompi_datatype_t *datatype, 
                                     int root, 
                                     struct ompi_communicator_t *comm);

  int mca_coll_tuned_exscan_intra_dec(void *sbuf, void *rbuf, int count, 
                                  struct ompi_datatype_t *dtype, 
                                  struct ompi_op_t *op, 
                                  struct ompi_communicator_t *comm);
  int mca_coll_tuned_exscan_inter_dec(void *sbuf, void *rbuf, int count, 
                                  struct ompi_datatype_t *dtype, 
                                  struct ompi_op_t *op, 
                                  struct ompi_communicator_t *comm);

  int mca_coll_tuned_gather_intra_dec(void *sbuf, int scount, 
                                  struct ompi_datatype_t *sdtype, void *rbuf, 
                                  int rcount, struct ompi_datatype_t *rdtype, 
                                  int root, struct ompi_communicator_t *comm);
  int mca_coll_tuned_gather_inter_dec(void *sbuf, int scount, 
                                  struct ompi_datatype_t *sdtype, void *rbuf, 
                                  int rcount, struct ompi_datatype_t *rdtype, 
                                  int root, struct ompi_communicator_t *comm);

  int mca_coll_tuned_gatherv_intra_dec(void *sbuf, int scount, 
                                   struct ompi_datatype_t *sdtype, void *rbuf, 
                                   int *rcounts, int *disps, 
                                   struct ompi_datatype_t *rdtype, int root, 
                                   struct ompi_communicator_t *comm);
  int mca_coll_tuned_gatherv_inter_dec(void *sbuf, int scount, 
                                   struct ompi_datatype_t *sdtype, void *rbuf, 
                                   int *rcounts, int *disps, 
                                   struct ompi_datatype_t *rdtype, int root, 
                                   struct ompi_communicator_t *comm);

  int mca_coll_tuned_reduce_intra_dec(void *sbuf, void* rbuf, int count, 
                                      struct ompi_datatype_t *dtype, 
                                      struct ompi_op_t *op, 
                                      int root,
                                      struct ompi_communicator_t *comm);
  int mca_coll_tuned_reduce_inter_dec(void *sbuf, void* rbuf, int count, 
                                      struct ompi_datatype_t *dtype,
                                      struct ompi_op_t *op, 
                                      int root,
                                      struct ompi_communicator_t *comm);

  int mca_coll_tuned_reduce_scatter_intra_dec(void *sbuf, void *rbuf, 
                                          int *rcounts, 
                                          struct ompi_datatype_t *dtype, 
                                          struct ompi_op_t *op, 
                                          struct ompi_communicator_t *comm);
  int mca_coll_tuned_reduce_scatter_inter_dec(void *sbuf, void *rbuf, 
                                          int *rcounts, 
                                          struct ompi_datatype_t *dtype, 
                                          struct ompi_op_t *op, 
                                          struct ompi_communicator_t *comm);
  
  int mca_coll_tuned_scan_intra_dec(void *sbuf, void *rbuf, int count, 
                                struct ompi_datatype_t *dtype, 
                                struct ompi_op_t *op, 
                                struct ompi_communicator_t *comm);
  int mca_coll_tuned_scan_inter_dec(void *sbuf, void *rbuf, int count, 
                                struct ompi_datatype_t *dtype, 
                                struct ompi_op_t *op, 
                                struct ompi_communicator_t *comm);

  int mca_coll_tuned_scatter_intra_dec(void *sbuf, int scount, 
                                   struct ompi_datatype_t *sdtype, void *rbuf, 
                                   int rcount, struct ompi_datatype_t *rdtype, 
                                   int root, struct ompi_communicator_t *comm);
  int mca_coll_tuned_scatter_inter_dec(void *sbuf, int scount, 
                                   struct ompi_datatype_t *sdtype, void *rbuf, 
                                   int rcount, struct ompi_datatype_t *rdtype, 
                                   int root, struct ompi_communicator_t *comm);

  int mca_coll_tuned_scatterv_intra_dec(void *sbuf, int *scounts, int *disps, 
                                    struct ompi_datatype_t *sdtype, 
                                    void* rbuf, int rcount, 
                                    struct ompi_datatype_t *rdtype, int root, 
                                    struct ompi_communicator_t *comm);
  int mca_coll_tuned_scatterv_inter_dec(void *sbuf, int *scounts, int *disps, 
                                    struct ompi_datatype_t *sdtype, 
                                    void* rbuf, int rcount, 
                                    struct ompi_datatype_t *rdtype, int root, 
                                    struct ompi_communicator_t *comm);



/* Utility functions */

static inline void mca_coll_tuned_free_reqs(ompi_request_t **reqs, int count)
{
  int i;
  for (i = 0; i < count; ++i)
     ompi_request_free(&reqs[i]);
}

/* decision table declaraion */
/* currently a place holder */
typedef struct rule_s {
} rule_t;

/*
 * Data structure for hanging data off the communicator 
 */
struct mca_coll_base_comm_t {
  /* standard data for requests and PML usage */

  /* we need to keep this here for now incase we fall through to the basic functions 
   * that expect these fields/and memory to be avaliable (GEF something for JS?)
   */
  ompi_request_t **mccb_reqs;
  int mccb_num_reqs;


  /* 
   * tuned topo information caching per communicator 
   *
   * for each communicator we cache the topo information so we can reuse without regenerating 
   * if we change the root, [or fanout] then regenerate and recache this information 
   *
   */

   ompi_coll_tree_t *cached_tree;
   int cached_tree_root; 
   int cached_tree_fanout; 

   ompi_coll_bmtree_t *cached_bmtree;
   int cached_bmtree_root;

   ompi_coll_chain_t *cached_chain;
   int cached_chain_root;
   int cached_chain_fanout; 

  /* extra data required by the decision functions */
  rule_t* decision_table;
};

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif
#endif /* MCA_COLL_TUNED_EXPORT_H */

/*
 * $HEADER$
 */

#include "ompi_config.h"
#include "coll_basic.h"

#include "mpi.h"
#include "include/constants.h"
#include "mca/coll/coll.h"
#include "mca/coll/base/coll_tags.h"
#include "mca/pml/pml.h"
#include "coll_basic.h"


/*
 *	scatter_intra
 *
 *	Function:	- scatter operation
 *	Accepts:	- same arguments as MPI_Scatter()
 *	Returns:	- MPI_SUCCESS or error code
 */
int mca_coll_basic_scatter_intra(void *sbuf, int scount, 
                                 struct ompi_datatype_t *sdtype,
                                 void *rbuf, int rcount, 
                                 struct ompi_datatype_t *rdtype,
                                 int root, 
                                 struct ompi_communicator_t *comm)
{
  int i;
  int rank;
  int size;
  int err;
  char *ptmp;
  long lb;
  long incr;

  /* Initialize */

  rank = ompi_comm_rank(comm);
  size = ompi_comm_size(comm);

  /* If not root, receive data. */

  if (rank != root) {
    err = mca_pml.pml_recv(rbuf, rcount, rdtype, root,
                           MCA_COLL_BASE_TAG_SCATTER, 
                           comm, MPI_STATUS_IGNORE);
    return err;
  }

  /* I am the root, loop sending data. */

  err = ompi_ddt_get_extent(rdtype, &lb, &incr);
  if (OMPI_SUCCESS != err) {
    return OMPI_ERROR;
  }

  incr *= scount;
  for (i = 0, ptmp = (char *) sbuf; i < size; ++i, ptmp += incr) {

    /* simple optimization */

    if (i == rank) {
      err = ompi_ddt_sndrcv(ptmp, scount, sdtype, rbuf,
                           rcount, rdtype, MCA_COLL_BASE_TAG_SCATTER, comm);
    } else {
      err = mca_pml.pml_send(ptmp, scount, sdtype, i, 
                             MCA_COLL_BASE_TAG_SCATTER, 
                             MCA_PML_BASE_SEND_STANDARD, comm);
    }
    if (MPI_SUCCESS != err) {
      return err;
    }
  }

  /* All done */

  return MPI_SUCCESS;
}


/*
 *	scatter_inter
 *
 *	Function:	- scatter operation
 *	Accepts:	- same arguments as MPI_Scatter()
 *	Returns:	- MPI_SUCCESS or error code
 */
int mca_coll_basic_scatter_inter(void *sbuf, int scount, 
                                 struct ompi_datatype_t *sdtype,
                                 void *rbuf, int rcount, 
                                 struct ompi_datatype_t *rdtype,
                                 int root, 
                                 struct ompi_communicator_t *comm)
{
  int i;
  int rank;
  int size;
  int err;
  char *ptmp;
  long lb;
  long incr;
  ompi_request_t **reqs=comm->c_coll_basic_data->mccb_reqs;

  /* Initialize */

  rank = ompi_comm_rank(comm);
  size = ompi_comm_remote_size(comm);

  if ( MPI_PROC_NULL == root ) {
      /* do nothing */
      err = OMPI_SUCCESS;
  }
  else if ( MPI_ROOT != root ) {
      /* If not root, receive data. */
      err = mca_pml.pml_recv(rbuf, rcount, rdtype, root,
                             MCA_COLL_BASE_TAG_SCATTER, 
                             comm, MPI_STATUS_IGNORE);
  }
  else{
      /* I am the root, loop sending data. */
      err = ompi_ddt_get_extent(rdtype, &lb, &incr);
      if (OMPI_SUCCESS != err) {
          return OMPI_ERROR;
      }

      incr *= scount;
      for (i = 0, ptmp = (char *) sbuf; i < size; ++i, ptmp += incr) {
          err = mca_pml.pml_isend(ptmp, scount, sdtype, i, 
                                  MCA_COLL_BASE_TAG_SCATTER, 
                                  MCA_PML_BASE_SEND_STANDARD, comm, reqs++);
          if (OMPI_SUCCESS != err) {
              return err;
          }
      }
      
      err = mca_pml.pml_wait_all (size, reqs, MPI_STATUSES_IGNORE);
  }
  
  return err;
}

/*
 * $HEADER$
 */

#include "ompi_config.h"

#include <stdio.h>

#include "mpi.h"
#include "mpi/f77/bindings.h"

#if OMPI_HAVE_WEAK_SYMBOLS && OMPI_PROFILE_LAYER
#pragma weak PMPI_REDUCE_SCATTER = mpi_reduce_scatter_f
#pragma weak pmpi_reduce_scatter = mpi_reduce_scatter_f
#pragma weak pmpi_reduce_scatter_ = mpi_reduce_scatter_f
#pragma weak pmpi_reduce_scatter__ = mpi_reduce_scatter_f
#elif OMPI_PROFILE_LAYER
OMPI_GENERATE_F77_BINDINGS (PMPI_REDUCE_SCATTER,
                           pmpi_reduce_scatter,
                           pmpi_reduce_scatter_,
                           pmpi_reduce_scatter__,
                           pmpi_reduce_scatter_f,
                           (char *sendbuf, char *recvbuf, MPI_Fint *recvcounts, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr),
                           (sendbuf, recvbuf, recvcounts, datatype, op, comm, ierr) )
#endif

#if OMPI_HAVE_WEAK_SYMBOLS
#pragma weak MPI_REDUCE_SCATTER = mpi_reduce_scatter_f
#pragma weak mpi_reduce_scatter = mpi_reduce_scatter_f
#pragma weak mpi_reduce_scatter_ = mpi_reduce_scatter_f
#pragma weak mpi_reduce_scatter__ = mpi_reduce_scatter_f
#endif

#if ! OMPI_HAVE_WEAK_SYMBOLS && ! OMPI_PROFILE_LAYER
OMPI_GENERATE_F77_BINDINGS (MPI_REDUCE_SCATTER,
                           mpi_reduce_scatter,
                           mpi_reduce_scatter_,
                           mpi_reduce_scatter__,
                           mpi_reduce_scatter_f,
                           (char *sendbuf, char *recvbuf, MPI_Fint *recvcounts, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr),
                           (sendbuf, recvbuf, recvcounts, datatype, op, comm, ierr) )
#endif


#if OMPI_PROFILE_LAYER && ! OMPI_HAVE_WEAK_SYMBOLS
#include "mpi/f77/profile/defines.h"
#endif

void mpi_reduce_scatter_f(char *sendbuf, char *recvbuf, 
			  MPI_Fint *recvcounts, MPI_Fint *datatype,
			  MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr)
{
    MPI_Comm c_comm;
    MPI_Datatype c_type;
    MPI_Op c_op;
    int size;
    OMPI_ARRAY_NAME_DECL(recvcounts);

    c_comm = MPI_Comm_f2c(*comm);
    c_type = MPI_Type_f2c(*datatype);
    c_op = MPI_Op_f2c(*op);

    MPI_Comm_size(c_comm, &size);
    OMPI_ARRAY_FINT_2_INT(recvcounts, size);
    
    *ierr = OMPI_INT_2_FINT(MPI_Reduce_scatter(sendbuf, recvbuf,
				       OMPI_ARRAY_NAME_CONVERT(recvcounts),
				       c_type, c_op, c_comm));
}

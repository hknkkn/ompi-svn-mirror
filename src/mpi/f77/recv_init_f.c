/*
 * $HEADER$
 */

#include "ompi_config.h"

#include <stdio.h>

#include "mpi.h"
#include "mpi/f77/bindings.h"

#if OMPI_HAVE_WEAK_SYMBOLS && OMPI_PROFILE_LAYER
#pragma weak PMPI_RECV_INIT = mpi_recv_init_f
#pragma weak pmpi_recv_init = mpi_recv_init_f
#pragma weak pmpi_recv_init_ = mpi_recv_init_f
#pragma weak pmpi_recv_init__ = mpi_recv_init_f
#elif OMPI_PROFILE_LAYER
OMPI_GENERATE_F77_BINDINGS (PMPI_RECV_INIT,
                           pmpi_recv_init,
                           pmpi_recv_init_,
                           pmpi_recv_init__,
                           pmpi_recv_init_f,
                           (char *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr),
                           (buf, count, datatype, source, tag, comm, request, ierr) )
#endif

#if OMPI_HAVE_WEAK_SYMBOLS
#pragma weak MPI_RECV_INIT = mpi_recv_init_f
#pragma weak mpi_recv_init = mpi_recv_init_f
#pragma weak mpi_recv_init_ = mpi_recv_init_f
#pragma weak mpi_recv_init__ = mpi_recv_init_f
#endif

#if ! OMPI_HAVE_WEAK_SYMBOLS && ! OMPI_PROFILE_LAYER
OMPI_GENERATE_F77_BINDINGS (MPI_RECV_INIT,
                           mpi_recv_init,
                           mpi_recv_init_,
                           mpi_recv_init__,
                           mpi_recv_init_f,
                           (char *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr),
                           (buf, count, datatype, source, tag, comm, request, ierr) )
#endif


#if OMPI_PROFILE_LAYER && ! OMPI_HAVE_WEAK_SYMBOLS
#include "mpi/f77/profile/defines.h"
#endif

void mpi_recv_init_f(char *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    MPI_Datatype c_type = MPI_Type_f2c(*datatype);
    MPI_Request c_req;
    MPI_Comm c_comm;

    c_comm = MPI_Comm_f2c (*comm);

    *ierr = MPI_Recv_init(buf, *count, c_type, *source, *tag, c_comm, &c_req);

    if (MPI_SUCCESS == *ierr) {
        *request = MPI_Request_c2f(c_req);
    }
}

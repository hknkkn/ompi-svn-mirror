/*
 * $HEADER$
 */

#include "ompi_config.h"

#include <stdio.h>

#include "mpi.h"
#include "mpi/f77/bindings.h"

#if OMPI_HAVE_WEAK_SYMBOLS && OMPI_PROFILE_LAYER
#pragma weak PMPI_CARTDIM_GET = mpi_cartdim_get_f
#pragma weak pmpi_cartdim_get = mpi_cartdim_get_f
#pragma weak pmpi_cartdim_get_ = mpi_cartdim_get_f
#pragma weak pmpi_cartdim_get__ = mpi_cartdim_get_f
#elif OMPI_PROFILE_LAYER
OMPI_GENERATE_F77_BINDINGS (PMPI_CARTDIM_GET,
                           pmpi_cartdim_get,
                           pmpi_cartdim_get_,
                           pmpi_cartdim_get__,
                           pmpi_cartdim_get_f,
                           (MPI_Fint *comm, MPI_Fint *ndims, MPI_Fint *ierr),
                           (comm, ndims, ierr) )
#endif

#if OMPI_HAVE_WEAK_SYMBOLS
#pragma weak MPI_CARTDIM_GET = mpi_cartdim_get_f
#pragma weak mpi_cartdim_get = mpi_cartdim_get_f
#pragma weak mpi_cartdim_get_ = mpi_cartdim_get_f
#pragma weak mpi_cartdim_get__ = mpi_cartdim_get_f
#endif

#if ! OMPI_HAVE_WEAK_SYMBOLS && ! OMPI_PROFILE_LAYER
OMPI_GENERATE_F77_BINDINGS (MPI_CARTDIM_GET,
                           mpi_cartdim_get,
                           mpi_cartdim_get_,
                           mpi_cartdim_get__,
                           mpi_cartdim_get_f,
                           (MPI_Fint *comm, MPI_Fint *ndims, MPI_Fint *ierr),
                           (comm, ndims, ierr) )
#endif


#if OMPI_PROFILE_LAYER && ! OMPI_HAVE_WEAK_SYMBOLS
#include "mpi/f77/profile/defines.h"
#endif

OMPI_EXPORT
void mpi_cartdim_get_f(MPI_Fint *comm, MPI_Fint *ndims, MPI_Fint *ierr)
{
    MPI_Comm c_comm;
    OMPI_SINGLE_NAME_DECL(ndims);
    
    c_comm = MPI_Comm_f2c(*comm);

    *ierr = OMPI_INT_2_FINT(MPI_Cartdim_get(c_comm,
					    OMPI_SINGLE_NAME_CONVERT(ndims)));

    OMPI_SINGLE_INT_2_FINT(ndims);
}

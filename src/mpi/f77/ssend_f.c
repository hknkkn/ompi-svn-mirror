/*
 * $HEADER$
 */

#include "ompi_config.h"

#include <stdio.h>

#include "mpi.h"
#include "mpi/f77/bindings.h"

#if OMPI_HAVE_WEAK_SYMBOLS && OMPI_PROFILE_LAYER
#pragma weak PMPI_SSEND = mpi_ssend_f
#pragma weak pmpi_ssend = mpi_ssend_f
#pragma weak pmpi_ssend_ = mpi_ssend_f
#pragma weak pmpi_ssend__ = mpi_ssend_f
#elif OMPI_PROFILE_LAYER
OMPI_GENERATE_F77_BINDINGS (PMPI_SSEND,
                           pmpi_ssend,
                           pmpi_ssend_,
                           pmpi_ssend__,
                           pmpi_ssend_f,
                           (char *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierr),
                           (buf, count, datatype, dest, tag, comm, ierr) )
#endif

#if OMPI_HAVE_WEAK_SYMBOLS
#pragma weak MPI_SSEND = mpi_ssend_f
#pragma weak mpi_ssend = mpi_ssend_f
#pragma weak mpi_ssend_ = mpi_ssend_f
#pragma weak mpi_ssend__ = mpi_ssend_f
#endif

#if ! OMPI_HAVE_WEAK_SYMBOLS && ! OMPI_PROFILE_LAYER
OMPI_GENERATE_F77_BINDINGS (MPI_SSEND,
                           mpi_ssend,
                           mpi_ssend_,
                           mpi_ssend__,
                           mpi_ssend_f,
                           (char *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierr),
                           (buf, count, datatype, dest, tag, comm, ierr) )
#endif


#if OMPI_PROFILE_LAYER && ! OMPI_HAVE_WEAK_SYMBOLS
#include "mpi/f77/profile/defines.h"
#endif

OMPI_EXPORT
void mpi_ssend_f(char *buf, MPI_Fint *count, MPI_Fint *datatype, 
		 MPI_Fint *dest, MPI_Fint *tag,
		 MPI_Fint *comm, MPI_Fint *ierr)
{
    MPI_Datatype c_type = MPI_Type_f2c(*datatype);
    MPI_Comm c_comm;

    c_comm = MPI_Comm_f2c (*comm);

    *ierr = OMPI_INT_2_FINT(MPI_Ssend(buf, OMPI_FINT_2_INT(*count),
				      c_type, OMPI_FINT_2_INT(*dest),
				      OMPI_FINT_2_INT(*tag), c_comm));
}

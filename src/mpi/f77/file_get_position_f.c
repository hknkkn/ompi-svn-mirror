/*
 * $HEADER$
 */

#include "ompi_config.h"

#include <stdio.h>

#include "mpi.h"
#include "mpi/f77/bindings.h"

#if OMPI_HAVE_WEAK_SYMBOLS && OMPI_PROFILE_LAYER
#pragma weak PMPI_FILE_GET_POSITION = mpi_file_get_position_f
#pragma weak pmpi_file_get_position = mpi_file_get_position_f
#pragma weak pmpi_file_get_position_ = mpi_file_get_position_f
#pragma weak pmpi_file_get_position__ = mpi_file_get_position_f
#elif OMPI_PROFILE_LAYER
OMPI_GENERATE_F77_BINDINGS (PMPI_FILE_GET_POSITION,
                           pmpi_file_get_position,
                           pmpi_file_get_position_,
                           pmpi_file_get_position__,
                           pmpi_file_get_position_f,
                           (MPI_Fint *fh, MPI_Fint *offset, MPI_Fint *ierr),
                           (fh, offset, ierr) )
#endif

#if OMPI_HAVE_WEAK_SYMBOLS
#pragma weak MPI_FILE_GET_POSITION = mpi_file_get_position_f
#pragma weak mpi_file_get_position = mpi_file_get_position_f
#pragma weak mpi_file_get_position_ = mpi_file_get_position_f
#pragma weak mpi_file_get_position__ = mpi_file_get_position_f
#endif

#if ! OMPI_HAVE_WEAK_SYMBOLS && ! OMPI_PROFILE_LAYER
OMPI_GENERATE_F77_BINDINGS (MPI_FILE_GET_POSITION,
                           mpi_file_get_position,
                           mpi_file_get_position_,
                           mpi_file_get_position__,
                           mpi_file_get_position_f,
                           (MPI_Fint *fh, MPI_Fint *offset, MPI_Fint *ierr),
                           (fh, offset, ierr) )
#endif


#if OMPI_PROFILE_LAYER && ! OMPI_HAVE_WEAK_SYMBOLS
#include "mpi/f77/profile/defines.h"
#endif

OMPI_EXPORT
void mpi_file_get_position_f(MPI_Fint *fh, MPI_Fint *offset, MPI_Fint *ierr)
{
    MPI_File c_fh = MPI_File_f2c(*fh);
    MPI_Offset c_offset;

    *ierr = OMPI_INT_2_FINT(MPI_File_get_position(c_fh, 
						  &c_offset));
    *offset = (MPI_Fint) c_offset;
}

/*
 * $HEADER$
 */

#include "ompi_config.h"

#include <stdio.h>

#include "mpi.h"
#include "mpi/f77/bindings.h"

#if OMPI_HAVE_WEAK_SYMBOLS && OMPI_PROFILE_LAYER
#pragma weak PMPI_FILE_GET_INFO = mpi_file_get_info_f
#pragma weak pmpi_file_get_info = mpi_file_get_info_f
#pragma weak pmpi_file_get_info_ = mpi_file_get_info_f
#pragma weak pmpi_file_get_info__ = mpi_file_get_info_f
#elif OMPI_PROFILE_LAYER
OMPI_GENERATE_F77_BINDINGS (PMPI_FILE_GET_INFO,
                           pmpi_file_get_info,
                           pmpi_file_get_info_,
                           pmpi_file_get_info__,
                           pmpi_file_get_info_f,
                           (MPI_Fint *fh, MPI_Fint *info_used, MPI_Fint *ierr),
                           (fh, info_used, ierr) )
#endif

#if OMPI_HAVE_WEAK_SYMBOLS
#pragma weak MPI_FILE_GET_INFO = mpi_file_get_info_f
#pragma weak mpi_file_get_info = mpi_file_get_info_f
#pragma weak mpi_file_get_info_ = mpi_file_get_info_f
#pragma weak mpi_file_get_info__ = mpi_file_get_info_f
#endif

#if ! OMPI_HAVE_WEAK_SYMBOLS && ! OMPI_PROFILE_LAYER
OMPI_GENERATE_F77_BINDINGS (MPI_FILE_GET_INFO,
                           mpi_file_get_info,
                           mpi_file_get_info_,
                           mpi_file_get_info__,
                           mpi_file_get_info_f,
                           (MPI_Fint *fh, MPI_Fint *info_used, MPI_Fint *ierr),
                           (fh, info_used, ierr) )
#endif


#if OMPI_PROFILE_LAYER && ! OMPI_HAVE_WEAK_SYMBOLS
#include "mpi/f77/profile/defines.h"
#endif

OMPI_EXPORT
void mpi_file_get_info_f(MPI_Fint *fh, MPI_Fint *info_used, MPI_Fint *ierr)
{
    MPI_File c_fh = MPI_File_f2c(*fh);
    MPI_Info c_info;

    *ierr = OMPI_INT_2_FINT(MPI_File_get_info(c_fh, &c_info));

    *info_used = MPI_Info_c2f(c_info);
}

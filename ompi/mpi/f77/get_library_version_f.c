/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
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

#include "ompi/mpi/f77/bindings.h"
#include "ompi/mpi/f77/f77_strings.h"

#if OPAL_HAVE_WEAK_SYMBOLS && OMPI_PROFILE_LAYER
#pragma weak PMPI_GET_LIBRARY_VERSION = mpi_get_library_version_f
#pragma weak pmpi_get_library_version = mpi_get_library_version_f
#pragma weak pmpi_get_library_version_ = mpi_get_library_version_f
#pragma weak pmpi_get_library_version__ = mpi_get_library_version_f
#elif OMPI_PROFILE_LAYER
OMPI_GENERATE_F77_BINDINGS (PMPI_GET_LIBRARY_VERSION,
                            pmpi_get_library_version,
                            pmpi_get_library_version_,
                            pmpi_get_library_version__,
                            pmpi_get_library_version_f,
                            (char *version, MPI_Fint *resultlen, MPI_Fint *ierr, MPI_Fint version_len),
                            (version, resultlen, ierr, version_len) )
#endif

#if OPAL_HAVE_WEAK_SYMBOLS
#pragma weak MPI_GET_LIBRARY_VERSION = mpi_get_library_version_f
#pragma weak mpi_get_library_version = mpi_get_library_version_f
#pragma weak mpi_get_library_version_ = mpi_get_library_version_f
#pragma weak mpi_get_library_version__ = mpi_get_library_version_f
#endif

#if ! OPAL_HAVE_WEAK_SYMBOLS && ! OMPI_PROFILE_LAYER
OMPI_GENERATE_F77_BINDINGS (MPI_GET_LIBRARY_VERSION,
                            mpi_get_library_version,
                            mpi_get_library_version_,
                            mpi_get_library_version__,
                            mpi_get_library_version_f,
                            (char *version, MPI_Fint *resultlen, MPI_Fint *ierr, MPI_Fint version_len),
                            (version, resultlen, ierr, version_len) )
#endif


#if OMPI_PROFILE_LAYER && ! OPAL_HAVE_WEAK_SYMBOLS
#include "ompi/mpi/f77/profile/defines.h"
#endif

void mpi_get_library_version_f(char *version, MPI_Fint *resultlen,
                               MPI_Fint *ierr, MPI_Fint version_len)
{
    int err, c_resultlen;
    char c_version[MPI_MAX_LIBRARY_VERSION_STRING];

    err = MPI_Get_library_version(c_version, &c_resultlen);
    if (MPI_SUCCESS == err) {
        ompi_fortran_string_c2f(c_version, version, 
                                OMPI_FINT_2_INT(version_len));
        *resultlen = OMPI_INT_2_FINT(c_resultlen);
        *ierr = OMPI_INT_2_FINT(MPI_SUCCESS);
    } else {
        *ierr = OMPI_INT_2_FINT(err);
    }
}

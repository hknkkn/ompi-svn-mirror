/*
 * $HEADER$
 */

#include "ompi_config.h"

#include <stdio.h>

#include "mpi.h"
#include "mpi/f77/bindings.h"
#include "include/constants.h"
#include "errhandler/errhandler.h"
#include "communicator/communicator.h"
#include "mpi/f77/bindings.h"
#include "mpi/f77/strings.h"

#if OMPI_HAVE_WEAK_SYMBOLS && OMPI_PROFILE_LAYER
#pragma weak PMPI_WIN_SET_NAME = mpi_win_set_name_f
#pragma weak pmpi_win_set_name = mpi_win_set_name_f
#pragma weak pmpi_win_set_name_ = mpi_win_set_name_f
#pragma weak pmpi_win_set_name__ = mpi_win_set_name_f
#elif OMPI_PROFILE_LAYER
OMPI_GENERATE_F77_BINDINGS (PMPI_WIN_SET_NAME,
                           pmpi_win_set_name,
                           pmpi_win_set_name_,
                           pmpi_win_set_name__,
                           pmpi_win_set_name_f,
                           (MPI_Fint *win, char *win_name, MPI_Fint *ierr, int name_len),
                           (win, win_name, ierr, name_len) )
#endif

#if OMPI_HAVE_WEAK_SYMBOLS
#pragma weak MPI_WIN_SET_NAME = mpi_win_set_name_f
#pragma weak mpi_win_set_name = mpi_win_set_name_f
#pragma weak mpi_win_set_name_ = mpi_win_set_name_f
#pragma weak mpi_win_set_name__ = mpi_win_set_name_f
#endif

#if ! OMPI_HAVE_WEAK_SYMBOLS && ! OMPI_PROFILE_LAYER
OMPI_GENERATE_F77_BINDINGS (MPI_WIN_SET_NAME,
                           mpi_win_set_name,
                           mpi_win_set_name_,
                           mpi_win_set_name__,
                           mpi_win_set_name_f,
                           (MPI_Fint *win, char *win_name, MPI_Fint *ierr, int name_len),
                           (win, win_name, ierr, name_len) )
#endif


#if OMPI_PROFILE_LAYER && ! OMPI_HAVE_WEAK_SYMBOLS
#include "mpi/f77/profile/defines.h"
#endif

void mpi_win_set_name_f(MPI_Fint *win, char *win_name, MPI_Fint *ierr,
			int name_len)
{
    int ret, c_err;
    char *c_name;
    MPI_Win c_win;

    c_win = MPI_Win_f2c(*win);

    /* Convert the fortran string */

    if (OMPI_SUCCESS != (ret = ompi_fortran_string_f2c(win_name, name_len,
                                                       &c_name))) {
        c_err = OMPI_ERRHANDLER_INVOKE(MPI_COMM_WORLD, ret,
				       "MPI_WIN_SET_NAME");
	*ierr = OMPI_INT_2_FINT(c_err);
        return;
    }

    /* Call the C function */

    *ierr = OMPI_INT_2_FINT(MPI_Win_set_name(c_win, c_name));

    /* Free the C name */

    free(c_name);
}

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
#include <stdio.h>

#include "ompi/mpi/c/bindings.h"
#include "ompi/win/win.h"
#include "ompi/mca/osc/osc.h"

#if OMPI_HAVE_WEAK_SYMBOLS && OMPI_PROFILING_DEFINES
#pragma weak MPI_Win_lock = PMPI_Win_lock
#endif

#if OMPI_PROFILING_DEFINES
#include "ompi/mpi/c/profile/defines.h"
#endif

static const char FUNC_NAME[] = "MPI_Win_lock";


int MPI_Win_lock(int lock_type, int rank, int assert, MPI_Win win) 
{
    int rc;

    if (MPI_PARAM_CHECK) {
        OMPI_ERR_INIT_FINALIZE(FUNC_NAME);

        if (ompi_win_invalid(win)) {
            return OMPI_ERRHANDLER_INVOKE(win, MPI_ERR_WIN, FUNC_NAME);
        } else if (lock_type != MPI_LOCK_EXCLUSIVE && 
                   lock_type != MPI_LOCK_SHARED) {
            return OMPI_ERRHANDLER_INVOKE(win, MPI_ERR_LOCKTYPE, FUNC_NAME);
        } else if (ompi_win_peer_invalid(win, rank)) {
            return OMPI_ERRHANDLER_INVOKE(win, MPI_ERR_RANK, FUNC_NAME);
        } else if (0 != (assert & ~(MPI_MODE_NOCHECK))) {
            return OMPI_ERRHANDLER_INVOKE(win, MPI_ERR_ASSERT, FUNC_NAME);
        } else if (0 != (ompi_win_get_mode(win) &
                         (OMPI_WIN_ACCESS_EPOCH | OMPI_WIN_EXPOSE_EPOCH))) {
            /* can't be in either an access or exposure epoch (even a lock one)*/
            return OMPI_ERRHANDLER_INVOKE(win, MPI_ERR_RMA_CONFLICT, FUNC_NAME);
        } else if (! ompi_win_allow_locks(win)) {
            return OMPI_ERRHANDLER_INVOKE(win, MPI_ERR_RMA_SYNC, FUNC_NAME);
        }
    }

    rc = win->w_osc_module->osc_lock(lock_type, rank, assert, win);
    OMPI_ERRHANDLER_RETURN(rc, win, rc, FUNC_NAME);
}

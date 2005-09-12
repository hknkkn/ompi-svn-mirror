/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University.
 *                         All rights reserved.
 * Copyright (c) 2004-2005 The Trustees of the University of Tennessee.
 *                         All rights reserved.
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

#include "mpi/c/bindings.h"
#include "ompi/include/constants.h"

#if OMPI_HAVE_WEAK_SYMBOLS && OMPI_PROFILING_DEFINES
#pragma weak MPI_Init_thread = PMPI_Init_thread
#endif

#if OMPI_PROFILING_DEFINES
#include "mpi/c/profile/defines.h"
#endif

static const char FUNC_NAME[] = "MPI_Init_thread";


int MPI_Init_thread(int *argc, char ***argv, int required,
                    int *provided) 
{
  int err;
  MPI_Comm null = NULL;

  /* Ensure that we were not already initialized or finalized */

  if (ompi_mpi_finalized) {
    /* JMS show_help */
    return OMPI_ERRHANDLER_INVOKE(null, MPI_ERR_OTHER, FUNC_NAME);
  } else if (ompi_mpi_initialized) {
    /* JMS show_help */
    return OMPI_ERRHANDLER_INVOKE(MPI_COMM_WORLD, MPI_ERR_OTHER, FUNC_NAME);
  }

  /* Call the back-end initialization function (we need to put as
     little in this function as possible so that if it's profiled, we
     don't lose anything) */

  if (NULL != argc && NULL != argv) {
      err = ompi_mpi_init(*argc, *argv, required, provided);
  } else {
      err = ompi_mpi_init(0, NULL, required, provided);
  }
  OMPI_ERRHANDLER_RETURN(err, null, err, FUNC_NAME);
}

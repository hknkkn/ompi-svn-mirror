/*
 * $HEADERS$
 */
#include "lam_config.h"
#include <stdio.h>

#include "mpi.h"
#include "mpi/interface/c/bindings.h"

#if LAM_HAVE_WEAK_SYMBOLS && LAM_PROFILING_DEFINES
#pragma weak MPI_Graph_get = PMPI_Graph_get
#endif

int MPI_Graph_get(MPI_Comm comm, int maxindex, int maxedges,
                  int *index, int *edges) {
    return MPI_SUCCESS;
}

/*
 * $HEADER$
 */

#include "ompi_config.h"

#include "mpi.h"
#include "mpi/c/bindings.h"
#include "datatype/datatype.h"
#include "errhandler/errhandler.h"
#include "communicator/communicator.h"

#if OMPI_HAVE_WEAK_SYMBOLS && OMPI_PROFILING_DEFINES
#pragma weak MPI_Type_free = PMPI_Type_free
#endif

#if OMPI_PROFILING_DEFINES
#include "mpi/c/profile/defines.h"
#endif

static const char FUNC_NAME[] = "MPI_Type_free";


int MPI_Type_free(MPI_Datatype *type)
{
   int rc;

   if( MPI_PARAM_CHECK ) {
      OMPI_ERR_INIT_FINALIZE(FUNC_NAME);
      if (NULL == type || NULL == *type || MPI_DATATYPE_NULL == *type) {
        return OMPI_ERRHANDLER_INVOKE(MPI_COMM_WORLD, MPI_ERR_TYPE,
                                      FUNC_NAME );
      }
   }

   rc = ompi_ddt_destroy( type );
   if( rc != MPI_SUCCESS ) {
      OMPI_ERRHANDLER_RETURN( MPI_ERR_INTERN, MPI_COMM_WORLD,
                             MPI_ERR_INTERN, FUNC_NAME );
   }
   *type = MPI_DATATYPE_NULL;
   return MPI_SUCCESS;
}

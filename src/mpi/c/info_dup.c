/*
 * $HEADER$
 */

#include "ompi_config.h"

#include "mpi.h"
#include "mpi/c/bindings.h"
#include "class/ompi_list.h"
#include "info/info.h"
#include "errhandler/errhandler.h"
#include "communicator/communicator.h"

#if OMPI_HAVE_WEAK_SYMBOLS && OMPI_PROFILING_DEFINES
#pragma weak MPI_Info_dup = PMPI_Info_dup
#endif

#if OMPI_PROFILING_DEFINES
#include "mpi/c/profile/defines.h"
#endif

static const char FUNC_NAME[] = "MPI_Info_dup";


/**
 *   MPI_Info_dup - Duplicate an 'MPI_Info' object
 *
 *   @param info source info object (handle)
 *   @param newinfo pointer to the new info object (handle)
 *
 *   @retval MPI_SUCCESS
 *   @retval MPI_ERR_INFO
 *   @retval MPI_ERR_NO_MEM
 *
 *   Not only will the (key, value) pairs be duplicated, the order of keys
 *   will be the same in 'newinfo' as it is in 'info'.
 *   When an info object is no longer being used, it should be freed with
 *   'MPI_Info_free'.
 */
int MPI_Info_dup(MPI_Info info, MPI_Info *newinfo) {
    int err;
    /**
     * Here we need to do 2 things
     * 1. Create a newinfo object using MPI_Info_create
     * 2. Fetch all the values from info and copy them to 
     *    newinfo using MPI_Info_set
     * The new implementation facilitates traversal in many ways.
     * I have chosen to get the number of elements on the list 
     * and copy them to newinfo one by one
     */

    if (MPI_PARAM_CHECK) {
        OMPI_ERR_INIT_FINALIZE(FUNC_NAME);
        if (NULL == info || MPI_INFO_NULL == info || NULL == newinfo ||
            ompi_info_is_freed(info)) {
            return OMPI_ERRHANDLER_INVOKE(MPI_COMM_WORLD, MPI_ERR_INFO,
                                          FUNC_NAME);
        }
    }

    *newinfo = OBJ_NEW(ompi_info_t);
    if (NULL == *newinfo) {
        return OMPI_ERRHANDLER_INVOKE(MPI_COMM_WORLD, MPI_ERR_NO_MEM,
                                      FUNC_NAME);
    }

    /*
     * Now to actually duplicate all the values
     */
    err = ompi_info_dup (info, newinfo);
    OMPI_ERRHANDLER_RETURN(err, MPI_COMM_WORLD, err, FUNC_NAME);
}

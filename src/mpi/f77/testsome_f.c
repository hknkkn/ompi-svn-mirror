/*
 * $HEADER$
 */

#include "ompi_config.h"

#include <stdio.h>
#include <stdlib.h>

#include "mpi.h"
#include "mpi/f77/bindings.h"
#include "mpi/f77/constants.h"
#include "errhandler/errhandler.h"
#include "communicator/communicator.h"

#if OMPI_HAVE_WEAK_SYMBOLS && OMPI_PROFILE_LAYER
#pragma weak PMPI_TESTSOME = mpi_testsome_f
#pragma weak pmpi_testsome = mpi_testsome_f
#pragma weak pmpi_testsome_ = mpi_testsome_f
#pragma weak pmpi_testsome__ = mpi_testsome_f
#elif OMPI_PROFILE_LAYER
OMPI_GENERATE_F77_BINDINGS (PMPI_TESTSOME,
                           pmpi_testsome,
                           pmpi_testsome_,
                           pmpi_testsome__,
                           pmpi_testsome_f,
                           (MPI_Fint *incount, MPI_Fint *array_of_requests, MPI_Fint *outcount, MPI_Fint *array_of_indices, MPI_Fint *array_of_statuses, MPI_Fint *ierr),
                           (incount, array_of_requests, outcount, array_of_indices, array_of_statuses, ierr) )
#endif

#if OMPI_HAVE_WEAK_SYMBOLS
#pragma weak MPI_TESTSOME = mpi_testsome_f
#pragma weak mpi_testsome = mpi_testsome_f
#pragma weak mpi_testsome_ = mpi_testsome_f
#pragma weak mpi_testsome__ = mpi_testsome_f
#endif

#if ! OMPI_HAVE_WEAK_SYMBOLS && ! OMPI_PROFILE_LAYER
OMPI_GENERATE_F77_BINDINGS (MPI_TESTSOME,
                           mpi_testsome,
                           mpi_testsome_,
                           mpi_testsome__,
                           mpi_testsome_f,
                           (MPI_Fint *incount, MPI_Fint *array_of_requests, MPI_Fint *outcount, MPI_Fint *array_of_indices, MPI_Fint *array_of_statuses, MPI_Fint *ierr),
                           (incount, array_of_requests, outcount, array_of_indices, array_of_statuses, ierr) )
#endif


#if OMPI_PROFILE_LAYER && ! OMPI_HAVE_WEAK_SYMBOLS
#include "mpi/f77/profile/defines.h"
#endif

static const char FUNC_NAME[] = "MPI_TESTSOME";


void mpi_testsome_f(MPI_Fint *incount, MPI_Fint *array_of_requests, 
		    MPI_Fint *outcount, MPI_Fint *array_of_indices,
		    MPI_Fint *array_of_statuses, MPI_Fint *ierr)
{
    int c_err;
    MPI_Request *c_req;
    MPI_Status *c_status;
    int i;
    OMPI_SINGLE_NAME_DECL(outcount);
    OMPI_ARRAY_NAME_DECL(array_of_indices);

    c_req = malloc(*incount * (sizeof(MPI_Request) + sizeof(MPI_Status)));
    if (NULL == c_req) {
        c_err = OMPI_ERRHANDLER_INVOKE(MPI_COMM_WORLD, MPI_ERR_NO_MEM,
                                       FUNC_NAME);
	*ierr = OMPI_INT_2_FINT(c_err);
        return;
    }
    c_status = (MPI_Status*) c_req + *incount;

    for (i = 0; i < *incount; i++) {
        c_req[i] = MPI_Request_f2c(array_of_requests[i]);
    }

    OMPI_ARRAY_FINT_2_INT_ALLOC(array_of_indices, *incount);
    *ierr = OMPI_INT_2_FINT(MPI_Testsome(OMPI_FINT_2_INT(*incount), c_req, 
				 OMPI_SINGLE_NAME_CONVERT(outcount), 
				 OMPI_ARRAY_NAME_CONVERT(array_of_indices), 
				 c_status));

    OMPI_SINGLE_INT_2_FINT(outcount);
    OMPI_ARRAY_INT_2_FINT(array_of_indices, *incount);

    if (MPI_SUCCESS == *ierr) {
        if (MPI_UNDEFINED != *outcount) {
            for (i = 0; i < *outcount; i++) {
                array_of_indices[i] += 1;
            }
        }
        for (i = 0; i < *outcount; i++) {
            array_of_requests[array_of_indices[i]] = 0;
            if (!OMPI_IS_FORTRAN_STATUSES_IGNORE(array_of_statuses) &&
                !OMPI_IS_FORTRAN_STATUS_IGNORE(&array_of_statuses[i])) {
                MPI_Status_c2f(&c_status[i], &array_of_statuses[i]);
            }
        }
    }

    free(c_req);
}

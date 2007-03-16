/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2006 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2006 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2006      Cisco Systems, Inc.  All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */
#include "ompi_config.h"
#include <stdio.h>

#include "ompi/mpi/c/bindings.h"
#include "ompi/mca/pml/pml.h"
#include "ompi/request/request.h"

#if OMPI_HAVE_WEAK_SYMBOLS && OMPI_PROFILING_DEFINES
#pragma weak MPI_Testsome = PMPI_Testsome
#endif

#if OMPI_PROFILING_DEFINES
#include "ompi/mpi/c/profile/defines.h"
#endif

static const char FUNC_NAME[] = "MPI_Testsome";


int MPI_Testsome(int incount, MPI_Request *requests,
                 int *outcount, int *indices,
                 MPI_Status *statuses)
{
    OPAL_CR_TEST_CHECKPOINT_READY();

    if ( MPI_PARAM_CHECK ) {
        int rc = MPI_SUCCESS;
        OMPI_ERR_INIT_FINALIZE(FUNC_NAME);
        if( 0 != incount ) {
            if( NULL == requests) {
                rc = MPI_ERR_REQUEST;
            } else if ((NULL == outcount) || (NULL == indices)) {
                rc = MPI_ERR_ARG;
            }
        }
        OMPI_ERRHANDLER_CHECK(rc, MPI_COMM_WORLD, rc, FUNC_NAME);
    }

    if (OMPI_SUCCESS == ompi_request_test_some(incount, requests, outcount, 
                                               indices, statuses)) {
        return MPI_SUCCESS;
    }
    if (MPI_SUCCESS !=
        ompi_errhandler_request_invoke(incount, requests, FUNC_NAME)) {
        return MPI_ERR_IN_STATUS;
    }
    return MPI_SUCCESS;
}

/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2006 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2008 High Performance Computing Center Stuttgart, 
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

#include "ompi/mpi/c/bindings.h"
#include "ompi/attribute/attribute.h"
#include "ompi/datatype/datatype.h"
#include "ompi/memchecker.h"

#if OMPI_HAVE_WEAK_SYMBOLS && OMPI_PROFILING_DEFINES
#pragma weak MPI_Type_get_attr = PMPI_Type_get_attr
#endif

#if OMPI_PROFILING_DEFINES
#include "ompi/mpi/c/profile/defines.h"
#endif

static const char FUNC_NAME[] = "MPI_Type_get_attr";


int MPI_Type_get_attr (MPI_Datatype type,
                       int type_keyval,
                       void *attribute_val,
                       int *flag)
{
    int ret;

    MEMCHECKER(
        memchecker_datatype(type);
    );

    if (MPI_PARAM_CHECK) {
        OMPI_ERR_INIT_FINALIZE(FUNC_NAME);
        if (NULL == type || MPI_DATATYPE_NULL == type) {
            return OMPI_ERRHANDLER_INVOKE(MPI_COMM_WORLD, MPI_ERR_TYPE,
                                          FUNC_NAME );
        } else if ((NULL == attribute_val) || (NULL == flag)) {
            return OMPI_ERRHANDLER_INVOKE(MPI_COMM_WORLD, 
                                          MPI_ERR_ARG, 
                                          FUNC_NAME);
        }
    }

    OPAL_CR_ENTER_LIBRARY();

    /* This stuff is very confusing.  Be sure to see
       src/attribute/attribute.c for a lengthy comment explaining Open
       MPI attribute behavior. */

    ret = ompi_attr_get_c(type->d_keyhash, type_keyval, 
                          (void**)attribute_val, flag);
    OMPI_ERRHANDLER_RETURN(ret, MPI_COMM_WORLD,
                           MPI_ERR_OTHER, FUNC_NAME);  
}

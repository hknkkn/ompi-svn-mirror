/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University.
 *                         All rights reserved.
 * Copyright (c) 2004-2005 The Trustees of the University of Tennessee.
 *                         All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

#include "ompi_config.h"

#include <stdio.h>

#include "mpi.h"
#include "mpi/f77/bindings.h"

#if OMPI_HAVE_WEAK_SYMBOLS && OMPI_PROFILE_LAYER
#pragma weak PMPI_UNPACK = mpi_unpack_f
#pragma weak pmpi_unpack = mpi_unpack_f
#pragma weak pmpi_unpack_ = mpi_unpack_f
#pragma weak pmpi_unpack__ = mpi_unpack_f
#elif OMPI_PROFILE_LAYER
OMPI_GENERATE_F77_BINDINGS (PMPI_UNPACK,
                           pmpi_unpack,
                           pmpi_unpack_,
                           pmpi_unpack__,
                           pmpi_unpack_f,
                           (char *inbuf, MPI_Fint *insize, MPI_Fint *position, char *outbuf, MPI_Fint *outcount, MPI_Fint *datatype, MPI_Fint *comm, MPI_Fint *ierr),
                           (inbuf, insize, position, outbuf, outcount, datatype, comm, ierr) )
#endif

#if OMPI_HAVE_WEAK_SYMBOLS
#pragma weak MPI_UNPACK = mpi_unpack_f
#pragma weak mpi_unpack = mpi_unpack_f
#pragma weak mpi_unpack_ = mpi_unpack_f
#pragma weak mpi_unpack__ = mpi_unpack_f
#endif

#if ! OMPI_HAVE_WEAK_SYMBOLS && ! OMPI_PROFILE_LAYER
OMPI_GENERATE_F77_BINDINGS (MPI_UNPACK,
                           mpi_unpack,
                           mpi_unpack_,
                           mpi_unpack__,
                           mpi_unpack_f,
                           (char *inbuf, MPI_Fint *insize, MPI_Fint *position, char *outbuf, MPI_Fint *outcount, MPI_Fint *datatype, MPI_Fint *comm, MPI_Fint *ierr),
                           (inbuf, insize, position, outbuf, outcount, datatype, comm, ierr) )
#endif


#if OMPI_PROFILE_LAYER && ! OMPI_HAVE_WEAK_SYMBOLS
#include "mpi/f77/profile/defines.h"
#endif

void mpi_unpack_f(char *inbuf, MPI_Fint *insize, MPI_Fint *position,
		  char *outbuf, MPI_Fint *outcount, MPI_Fint *datatype,
		  MPI_Fint *comm, MPI_Fint *ierr)
{
    MPI_Comm c_comm;
    MPI_Datatype c_type;
    OMPI_SINGLE_NAME_DECL(position);

    c_comm = MPI_Comm_f2c(*comm);
    c_type = MPI_Type_f2c(*datatype);
    OMPI_SINGLE_FINT_2_INT(position);

    *ierr = OMPI_INT_2_FINT(MPI_Unpack(inbuf, OMPI_FINT_2_INT(*insize),
				       OMPI_SINGLE_NAME_CONVERT(position),
				       outbuf, OMPI_FINT_2_INT(*outcount),
				       c_type, c_comm));
    
    OMPI_SINGLE_INT_2_FINT(position);
}

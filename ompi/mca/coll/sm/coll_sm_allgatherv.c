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

#include "ompi/include/constants.h"
#include "coll_sm.h"


/*
 *	allgatherv_intra
 *
 *	Function:	- allgatherv
 *	Accepts:	- same as MPI_Allgatherv()
 *	Returns:	- MPI_SUCCESS or error code
 */
int mca_coll_sm_allgatherv_intra(void *sbuf, int scount, 
                                 struct ompi_datatype_t *sdtype, 
                                 void * rbuf, int *rcounts, int *disps, 
                                 struct ompi_datatype_t *rdtype, 
                                 struct ompi_communicator_t *comm)
{
    return OMPI_ERR_NOT_IMPLEMENTED;
}

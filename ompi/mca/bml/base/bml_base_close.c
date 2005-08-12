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

#include "mca/btl/base/base.h" 
#include "mca/bml/base/base.h" 

int mca_bml_base_close( void ) {
    int rc; 
    if(OMPI_SUCCESS != (rc = mca_btl_base_close()))
        return rc;
    return OMPI_SUCCESS; 
}


/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University.
 *                         All rights reserved.
 * Copyright (c) 2004-2005 The Trustees of the University of Tennessee.
 *                         All rights reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */
/**
 * @file
 */

#ifndef OMPI_NUMTOSTR_UTIL
#define OMPI_NUMTOSTR_UTIL

#include "ompi_config.h"
/**
 * Convert a long integer to a char* string.  The returned buffer is
 * allocated by calling malloc() and must be freed by the caller.
 *  
 *  @param num (IN)      Input number
 *  @return              String containing number (NULL on failure)
 */
OMPI_DECLSPEC char* ompi_ltostr(long num);


/**
 * Convert a double to a char* string.  The returned buffer is allocated
 * by calling malloc() and must be freed by the caller.
 *
 * @param num (IN)       Input number
 * @return               String containing number (NULL on failure)
 */
OMPI_DECLSPEC char* ompi_dtostr(double num);

#endif /* OMPI_NUMTOSTR_UTIL */

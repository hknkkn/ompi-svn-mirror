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
 *
 * Types for interface into the Open MPI Run Time Environment
 */

#ifndef OMPI_RUNTIME_TYPES_H
#define OMPI_RUNTIME_TYPES_H

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

/*
 * define size of exit codes - should be moved to status monitor framework
 * when that becomes available
 */
    typedef int8_t ompi_exit_code_t;
    typedef int8_t ompi_status_key_t;
    typedef int8_t ompi_node_state_t;


#if defined(c_plusplus) || defined(__cplusplus)
}
#endif
#endif /* OMPI_RUNTIME_TYPES_H */

/*
 * Copyright (c) 2004-2006 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2006 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
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
/** @file:
 */

#ifndef MCA_ODLS_PRIVATE_H
#define MCA_ODLS_PRIVATE_H

/*
 * includes
 */
#include "orte_config.h"

#include "opal/class/opal_list.h"

#include "orte/dss/dss_types.h"
#include "orte/mca/ns/ns_types.h"
#include "orte/mca/rmgr/rmgr_types.h"

#include "orte/mca/odls/odls_types.h"

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

/*
 * General ODLS types
 */
typedef struct orte_odls_globals_t {
    /** Verbose/debug output stream */
    int output;
    /** Time to allow process to forcibly die */
    int timeout_before_sigkill;
} orte_odls_globals_t;

ORTE_DECLSPEC extern orte_odls_globals_t orte_odls_globals;
        
/*
 * data type functions
 */

int orte_odls_compare_daemon_cmd(orte_daemon_cmd_flag_t *value1, orte_daemon_cmd_flag_t *value2, orte_data_type_t type);

int orte_odls_copy_daemon_cmd(orte_daemon_cmd_flag_t **dest, orte_daemon_cmd_flag_t *src, orte_data_type_t type);

int orte_odls_pack_daemon_cmd(orte_buffer_t *buffer, void *src,
                              orte_std_cntr_t num_vals, orte_data_type_t type);

int orte_odls_print_daemon_cmd(char **output, char *prefix, orte_daemon_cmd_flag_t *src, orte_data_type_t type);

void orte_odls_std_release(orte_data_value_t *value);

int orte_odls_size_daemon_cmd(size_t *size, orte_daemon_cmd_flag_t *src, orte_data_type_t type);

int orte_odls_unpack_daemon_cmd(orte_buffer_t *buffer, void *dest,
                                orte_std_cntr_t *num_vals, orte_data_type_t type);

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif
#endif

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

#ifndef ORTE_NAMES_H
#define ORTE_NAMES_H

/*
 * ORTE_DATA_NAME macro
 * This macro is provided so that users can output an intelligible name for a data
 * type during debugging. It is called by passing the data type into the macro and
 * outputing the result via some print variant. For example, you can call it as:
 * ompi_output(0, "data type: %s", ORTE_DATA_NAME(keyval.type));
 * 
 * THE ACTUAL ARRAY IS INSTANTIATED IN runtime/ompi_init.c
 */

#define ORTE_DATA_NAME(n) orte_data_strings[n]
extern char *orte_data_strings[];

/*
 * Similar to the above, this macro and array are used to output intelligible error
 * messages. It is disturbing to think that we are still outputing error numbers and
 * expecting users to look them up in the "big book" to find out what they represent.
 * This macro allows the user to output an actual string representation of the error.
 * 
 *  * THE ACTUAL ARRAY IS INSTANTIATED IN runtime/ompi_init.c
 */

#define ORTE_ERROR_NAME(n)  orte_error_strings[-1*n]
extern char *orte_error_strings[];

/*
 * ORTE SEGMENT NAMES
 * There are some predefined segments that are used across the entire ORTE system.
 * These defines establish those names so everyone can access them, and so they
 * can be easily changed if required.
 */
#define ORTE_JOB_SEGMENT        "orte-job"
#define ORTE_NODE_SEGMENT       "orte-node"
#define ORTE_RESOURCE_SEGMENT   "orte-resources"

/*
 * ORTE-wide key names for storing/retrieving data from the registry.
 * Subsystem-specific keys will be defined in each=/ subsystem's xxx_types.h file.
 */
#define ORTE_NODE_NAME_KEY        "orte-node-name"
#define ORTE_NODE_STATE_KEY       "orte-node-state"
#define ORTE_NODE_SLOTS_KEY       "orte-node-slots"
#define ORTE_NODE_SLOTS_ALLOC_KEY "orte-node-slots-alloc"
#define ORTE_NODE_SLOTS_MAX_KEY   "orte-node-slots-max"
#define ORTE_NODE_ALLOC_KEY       "orte-node-alloc"
#define ORTE_JOBID_KEY            "orte-jobid"
#define ORTE_CELLID_KEY           "orte-cellid"
#define ORTE_IOF_INFO             "orte-iof-info"
#define ORTE_APP_CONTEXT_KEY      "orte-app-context"
#define ORTE_PROC_RANK_KEY        "orte-proc-rank"
#define ORTE_PROC_PID_KEY         "orte-proc-pid"
#define ORTE_PROC_STATUS_KEY      "orte-proc-status"
#define ORTE_PROC_EXIT_CODE_KEY   "orte-proc-exit-code"

#endif

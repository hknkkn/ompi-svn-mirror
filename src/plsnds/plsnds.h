/* -*- C -*-
 *
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
 * Process Launch Subsystem's Name Discovery Service
 *
 * The process launch subsystem's name discovery service allows a process to determine
 * its official name within the ORTE universe. The name of a process can be defined
 * in multiple ways - it can be defaulted to a specific name (in the case of the seed
 * daemon or a singleton process), it can be passed to the process through the
 * environment, or the means of determining it can be be passed through some means
 * (e.g., environmental parameter) that an algorithm uses to derive the name.
 * 
 * Since each launcher/environment can be different, the PLSNDS provides a mechanism
 * by which writers of PLS launchers can provide the necessary corresponding logic
 * for process name discovery. Accordingly, each PLS MUST:
 * 
 * - set the environmental parameter OMPI_MCA_PLS_LAUNCHER to indicate the launcher
 * used to spawn the process. 
 * 
 * - have a corresponding entry in the orte_plsnds table (defined in
 * src/plsnds/plsnds_open_close.c) that identifies the launcher and its associated
 * function for obtaining the process name. This information is stored in an array
 * of orte_plsnds_t structures - each of which contains the string name of the
 * launcher (that must correspond exactly with the value of the environmental
 * parameter OMPI_MCA_PLS_LAUNCHER as set by the launcher) and a function pointer
 * to the necessary name discovery function.
 * 
 * - where necessary, provide a function in the orte_plsnds directory that can
 * define the process name from whatever info that corresponding launcher provided.
 * PLS launcher developers are welcome to use existing functions where possible - e.g.,
 * many launchers are capable of setting environmental parameters found in the
 * plsnds_env.c function and hence can utilize that existing functionality. The
 * prototype for any new functions MUST be entered in the plsnds_fns.h file, and be
 * included in the Makefile.am file.
 * 
 * NOTE: The "ENV" entry in the orte_plsnds array MUST always be in the first position.
 * Please add any additional entries to the end of the array. Outside of the "ENV"
 * requirement, the order does NOT make any difference - the array will be traversed
 * until the corresponding entry is found.
 * 
 * WARNING: Be sure to increment the orte_plsnds_max value so that the name discovery
 * subsystem will correctly search the entire array.
 * 
 */

#ifndef ORTE_PLSNDS_H_
#define ORTE_PLSNDS_H_

#include "orte_config.h"

#include "include/orte_types.h"
#include "include/orte_constants.h"

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

/*
 * Define the plsnds function pointer
 */
typedef int (*orte_plsnds_base_discover_fn_t)(void);

/*
 * Define the plsnds structure
 */
typedef struct {
    char *launcher;
    orte_plsnds_base_discover_fn_t discover;
} orte_plsnds_t;

/*
 * PLSNDS initialization function
 * In dynamic libraries, declared objects and functions don't get loaded
 * until called. We need to ensure that the orte_dps function structure
 * gets loaded, so we provide an "open" call that is executed as part of
 * the program startup. It simply checks for debug parameters - good enough
 * to ensure that the PLSNDS gets loaded!
 */
OMPI_DECLSPEC int orte_plsnds_open(void);

/*
 * PLSNDS finalize function
 */
OMPI_DECLSPEC int orte_plsnds_close(void);

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif

OMPI_DECLSPEC extern orte_plsnds_t orte_plsnds[];  /* holds plsnds function pointers */
OMPI_DECLSPEC extern int orte_plsnds_max;  /* number of entries in array */

#endif /* ORTE_PLSNDS_H */

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
 * Process Launch Subsystem's Name Discovery Services
 * THIS IS WHERE EACH FUNCTION IS PROTOTYPED
 *
 *
 */

#ifndef ORTE_PLSNDS_FNS_H_
#define ORTE_PLSNDS_FNS_H_

#include "orte_config.h"

#include "plsnds.h"

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

/*
 * List the functions here
 */
int orte_plsnds_env(void);

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif


#endif /* ORTE_PLSNDS_FNS_H */

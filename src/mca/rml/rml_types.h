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
/** @file:
 *
 * Contains the typedefs for the use of the rml
 */

#ifndef MCA_RML_TYPES_H_
#define MCA_RML_TYPES_H_

#include "orte_config.h"
#include "include/orte_constants.h"
#include <limits.h>


typedef uint32_t orte_rml_tag_t;

#define ORTE_RML_TAG_MAX UINT32_MAX

/**
 * The wildcard for receives from any peer.
 */
#define ORTE_RML_NAME_ANY  &orte_rml_name_any
/**
 * Process name of seed
 */
#define ORTE_RML_NAME_SEED &orte_rml_name_seed

#endif  /* RML_TYPES */

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
 * the oob framework
 */

#ifndef _MCA_OOB_TYPES_H_
#define _MCA_OOB_TYPES_H_

#include "orte_config.h"

#include "include/orte_constants.h"

#include <limits.h>

/*
 * Other constants
 */

typedef uint32_t orte_oob_tag_t;

#define ORTE_OOB_TAG_MAX UINT32_MAX

/**
 * Receive from any tag.
 */
#define MCA_OOB_TAG_ANY 0
/**
 * Service tags
 */
#define MCA_OOB_TAG_NS           (orte_oob_tag_t)   1
#define MCA_OOB_TAG_GPR          (orte_oob_tag_t)   2
#define MCA_OOB_TAG_GPR_NOTIFY   (orte_oob_tag_t)   3
#define MCA_OOB_TAG_RTE          (orte_oob_tag_t)   4
#define MCA_OOB_TAG_EXEC         (orte_oob_tag_t)   5
#define MCA_OOB_TAG_DAEMON       (orte_oob_tag_t)   6
#define MCA_OOB_TAG_STDIO        (orte_oob_tag_t)   7
#define MCA_OOB_TAG_SCHED        (orte_oob_tag_t)   8
#define MCA_OOB_TAG_PCM_KILL     (orte_oob_tag_t)   9
#define MCA_OOB_TAG_XCAST        (orte_oob_tag_t)  10
#define MCA_OOB_TAG_PCM_KILL_ACK (orte_oob_tag_t)  11
#define ORTE_OOB_TAG_START_LIST  (orte_oob_tag_t) 100  /* starting point for tag server assignments */

/**
 * The wildcard for receives from any peer.
 */
#define MCA_OOB_NAME_ANY  &mca_oob_name_any
/**
 * Process name of self
 */
#define MCA_OOB_NAME_SELF &mca_oob_name_self
/**
 * Process name of seed
 */
#define MCA_OOB_NAME_SEED &mca_oob_name_seed

#endif  /* MCA_OOB_TYPES_H */

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

#ifndef ORTE_SOH_TYPES_H
#define ORTE_SOH_TYPES_H

#include "orte_config.h"

/*
 * Process exit codes
 */

typedef int8_t orte_exit_code_t;

#define ORTE_PROC_EXIT_CODE_NORMAL      0x00
#define ORTE_PROC_EXIT_CODE_ABNORMAL    0x01

/*
 * Process status keys
 */

typedef int8_t orte_status_key_t;

#define ORTE_PROC_STATUS_RUNNING        0x00
#define ORTE_PROC_STATUS_FINALIZING     0x01
#define ORTE_PROC_STATUS_INIT           0x02
#define ORTE_PROC_STATUS_TERMINATING    0x03
#define ORTE_PROC_STATUS_STARTING       0x04
#define ORTE_PROC_STATUS_LAUNCHING      0x05


/**
 * Node State, correspondinding to the ORTE_NODE_STATE_* #defines,
 * below.  These are #defines instead of an enum because the thought
 * is that we may have lots and lots of entries of these in the
 * registry and by making this an int8_t, it's only 1 byte, whereas an
 * enum defaults to an int (probably 4 bytes).  So it's a bit of a
 * space savings.
 */
typedef int8_t orte_node_state_t;

/** Node is in an unknown state (see orte_node_state_t) */
#define ORTE_NODE_STATE_UNKNOWN  0x00
/** Node is down (see orte_node_state_t) */
#define ORTE_NODE_STATE_DOWN     0x01
/** Node is up / available for use (see orte_node_state_t) */
#define ORTE_NODE_STATE_UP       0x02
/** Node is rebooting (only some systems will support this; see
    orte_node_state_t) */
#define ORTE_NODE_STATE_REBOOT   0x03

#endif

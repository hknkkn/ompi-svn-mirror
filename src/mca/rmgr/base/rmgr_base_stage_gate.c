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
 */

/*
 * includes
 */
#include "orte_config.h"
#include "include/orte_constants.h"
#include "include/orte_types.h"

#include "class/ompi_object.h"
#include "mca/mca.h"
#include "mca/gpr/gpr.h"
#include "mca/rmgr/base/base.h"


int orte_rmgr_base_proc_stage_gate_init(orte_jobid_t job)
{
    return ORTE_SUCCESS;
}


void orte_rmgr_base_proc_stage_gate_mgr(orte_gpr_notify_message_t *notify_msg,
                                        void *user_tag)
{
    OBJ_RELEASE(notify_msg);
}

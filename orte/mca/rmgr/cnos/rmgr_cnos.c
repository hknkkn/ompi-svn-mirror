/*
 * Copyright (c) 2004-2006 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */
#include "orte_config.h"

#include <stdlib.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif  /* HAVE_UNISTD_H */
#ifdef HAVE_STRING_H
#include <string.h>
#endif  /* HAVE_STRING_H */
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
#ifdef HAVE_CNOS_PM_BARRIER
#include <catamount/cnos_mpi_os.h>
#endif

#include "opal/class/opal_list.h"

#include "orte/orte_constants.h"
#include "orte/mca/rmgr/base/base.h"
#include "orte/mca/ns/ns.h"
#include "orte/util/proc_info.h"
#include "rmgr_cnos.h"


static int orte_rmgr_cnos_setup_job(
    orte_app_context_t** app_context,
    orte_std_cntr_t num_context,
    orte_jobid_t* jobid);

static int orte_rmgr_cnos_spawn_job(
    orte_app_context_t** app_context,
    orte_std_cntr_t num_context,
    orte_jobid_t* jobid,
    orte_std_cntr_t num_connect,
    orte_process_name_t *connect,
    orte_rmgr_cb_fn_t cbfn,
    orte_proc_state_t cb_conditions,
    opal_list_t *attributes);

static int orte_rmgr_cnos_connect(orte_std_cntr_t num_connect,
                                  orte_process_name_t *connect);

static int orte_rmgr_cnos_disconnect(orte_std_cntr_t num_connect,
                                     orte_process_name_t *connect);

static int orte_rmgr_cnos_finalize(void);

static int orte_rmgr_cnos_get_app_context(orte_jobid_t jobid,
                                          orte_app_context_t*** app_context,
                                          orte_std_cntr_t* num_context);

static int orte_rmgr_cnos_put_app_context(orte_jobid_t jobid,
                                          orte_app_context_t** app_context,
                                          orte_std_cntr_t num_context);

static int orte_rmgr_cnos_check_context_app(orte_app_context_t *context);

static int orte_rmgr_cnos_check_context_cwd(orte_app_context_t *context,
                                            bool want_chdir);

static int orte_rmgr_cnos_set_vpid_range(orte_jobid_t jobid,
                                         orte_vpid_t start,
                                         orte_vpid_t range);

static int orte_rmgr_cnos_get_vpid_range(orte_jobid_t jobid,
                                         orte_vpid_t *start,
                                         orte_vpid_t *range);

orte_rmgr_base_module_t orte_rmgr_cnos_module = {
    NULL, /* don't need special init */
    orte_rmgr_cnos_setup_job,
    orte_rmgr_cnos_spawn_job,
    orte_rmgr_cnos_connect,
    orte_rmgr_cnos_disconnect,
    orte_rmgr_cnos_finalize,
    /**   SUPPORT FUNCTIONS   ***/
    orte_rmgr_cnos_get_app_context,
    orte_rmgr_cnos_put_app_context,
    orte_rmgr_cnos_check_context_cwd,
    orte_rmgr_cnos_check_context_app,
    orte_rmgr_cnos_set_vpid_range,
    orte_rmgr_cnos_get_vpid_range
};


/*
 *  Create the job segment and initialize the application context.
 */
static int orte_rmgr_cnos_setup_job(
    orte_app_context_t** app_context,
    orte_std_cntr_t num_context,
    orte_jobid_t* jobid)
{
    return ORTE_ERR_NOT_SUPPORTED;
}


static int orte_rmgr_cnos_spawn_job(
    orte_app_context_t** app_context,
    orte_std_cntr_t num_context,
    orte_jobid_t* jobid,
    orte_rmgr_cb_fn_t cbfunc,
    orte_proc_state_t cb_conditions)
{
    return ORTE_ERR_NOT_SUPPORTED;
}

static int orte_rmgr_cnos_connect(orte_std_cntr_t num_connect,
                                  orte_process_name_t *connect)
{
    return ORTE_ERR_NOT_SUPPORTED;
}

static int orte_rmgr_cnos_disconnect(orte_std_cntr_t num_connect,
                                     orte_process_name_t *connect)
{
    return ORTE_ERR_NOT_SUPPORTED;
}


static int orte_rmgr_cnos_finalize(void)
{
#ifdef HAVE_CNOS_PM_BARRIER
    /* register with the process manager so that everyone aborts if
       any one process aborts.  This is a bit slower than it needs to
       be, but useful. */
    cnos_pm_barrier(1);
#endif

    return ORTE_SUCCESS;
}

static int orte_rmgr_cnos_get_app_context(orte_jobid_t jobid,
                                          orte_app_context_t*** app_context,
                                          orte_std_cntr_t* num_context)
{
    return ORTE_ERR_NOT_SUPPORTED;
}

static int orte_rmgr_cnos_put_app_context(orte_jobid_t jobid,
                                          orte_app_context_t** app_context,
                                          orte_std_cntr_t num_context)
{
    return ORTE_ERR_NOT_SUPPORTED;
}

static int orte_rmgr_cnos_check_context_app(orte_app_context_t *context)
{
    return ORTE_ERR_NOT_SUPPORTED;
}

static int orte_rmgr_cnos_check_context_cwd(orte_app_context_t *context,
                                            bool want_chdir)
{
    return ORTE_ERR_NOT_SUPPORTED;
}

static int orte_rmgr_cnos_set_vpid_range(orte_jobid_t jobid,
                                         orte_vpid_t start,
                                         orte_vpid_t range)
{
    return ORTE_ERR_NOT_SUPPORTED;
}

static int orte_rmgr_cnos_get_vpid_range(orte_jobid_t jobid,
                                         orte_vpid_t *start,
                                         orte_vpid_t *range)
{
    return ORTE_ERR_NOT_SUPPORTED;
}

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


#include "ompi_config.h"
#include "pcmclient-rms-version.h"

#include "include/constants.h"
#include "include/types.h"
#include "mca/mca.h"
#include "mca/pcmclient/pcmclient.h"
#include "mca/pcmclient/rms/pcmclient_rms.h"
#include "util/proc_info.h"
#include "mca/base/mca_base_param.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/*
 * Struct of function pointers and all that to let us be initialized
 */
mca_pcmclient_base_component_1_0_0_t mca_pcmclient_rms_component = {
  {
    MCA_PCMCLIENT_BASE_VERSION_1_0_0,

    "rms", /* MCA component name */
    MCA_pcmclient_rms_MAJOR_VERSION,  /* MCA component major version */
    MCA_pcmclient_rms_MINOR_VERSION,  /* MCA component minor version */
    MCA_pcmclient_rms_RELEASE_VERSION,  /* MCA component release version */
    mca_pcmclient_rms_open,  /* component open */
    mca_pcmclient_rms_close /* component close */
  },
  {
    false /* checkpoint / restart */
  },
  mca_pcmclient_rms_init,    /* component init */
  mca_pcmclient_rms_finalize
};


struct mca_pcmclient_base_module_1_0_0_t mca_pcmclient_rms_1_0_0 = {
    mca_pcmclient_rms_init_cleanup,
    mca_pcmclient_rms_get_self,
    mca_pcmclient_rms_get_peers,
};

/*
 * component-global variables
 */
int mca_pcmclient_rms_num_procs;
int mca_pcmclient_rms_procid;

ompi_process_name_t *mca_pcmclient_rms_procs = NULL;

/*
 * local variables
 */

static int rms_jobid_handle;
static int rms_start_vpid_handle;
static int rms_cellid_handle;

int mca_pcmclient_rms_cellid;
int mca_pcmclient_rms_jobid;

int
mca_pcmclient_rms_open(void)
{
    rms_jobid_handle = 
        mca_base_param_register_int("pcmclient", "rms", "jobid", NULL, -1);
    rms_start_vpid_handle = 
        mca_base_param_register_int("pcmclient", "rms", "start_vpid", NULL, 0);
    rms_cellid_handle =
        mca_base_param_register_int("pcmclient", "rms", "cellid", NULL, 0);

    return OMPI_SUCCESS;
}


int
mca_pcmclient_rms_close(void)
{
  return OMPI_SUCCESS;
}


struct mca_pcmclient_base_module_1_0_0_t *
mca_pcmclient_rms_init(int *priority, 
                             bool *allow_multiple_user_threads, 
                             bool *have_hidden_threads)
{
    int i;
    char *tmp;
    int start_vpid;

    *priority = 5; /* make sure we are above env / singleton */
    *allow_multiple_user_threads = true;
    *have_hidden_threads = false;

    mca_base_param_lookup_int(rms_jobid_handle, &mca_pcmclient_rms_jobid);
    mca_base_param_lookup_int(rms_cellid_handle, &mca_pcmclient_rms_cellid);
    mca_base_param_lookup_int(rms_start_vpid_handle, &start_vpid);

    if (mca_pcmclient_rms_jobid < 0) {
        tmp = getenv("RMS_JOBID");
        if (NULL == tmp) return NULL;
        mca_pcmclient_rms_jobid = atoi(tmp);
    }

    tmp = getenv("RMS_RANK");
    if (NULL == tmp) return NULL;
    mca_pcmclient_rms_procid = atoi(tmp);

    tmp = getenv("RMS_NPROCS");
    if (NULL == tmp) return NULL;
    mca_pcmclient_rms_num_procs = atoi(tmp);

    mca_pcmclient_rms_procs = 
        (ompi_process_name_t*) malloc(sizeof(ompi_process_name_t) * 
                                      mca_pcmclient_rms_num_procs);
    if (NULL == mca_pcmclient_rms_procs) return NULL;

    for ( i = 0 ; i < mca_pcmclient_rms_num_procs ; ++i) {
        mca_pcmclient_rms_procs[i].cellid = mca_pcmclient_rms_cellid;
        mca_pcmclient_rms_procs[i].jobid = mca_pcmclient_rms_jobid;
        mca_pcmclient_rms_procs[i].vpid = start_vpid + i;
    }
    
    return &mca_pcmclient_rms_1_0_0;
}


int
mca_pcmclient_rms_finalize(void)
{
    if (NULL != mca_pcmclient_rms_procs) {
        free(mca_pcmclient_rms_procs);
        mca_pcmclient_rms_procs = NULL;
        mca_pcmclient_rms_num_procs = 0;
    }

    return OMPI_SUCCESS;
}



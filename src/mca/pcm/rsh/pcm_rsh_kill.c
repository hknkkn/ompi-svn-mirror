/* -*- C -*-
 * 
 * $HEADER$
 *
 */

#include "ompi_config.h"

#include "pcm_rsh.h"
#include "include/constants.h"
#include "mca/pcm/pcm.h"
#include "mca/pcm/base/base_job_track.h"


int
mca_pcm_rsh_kill_proc(struct mca_pcm_base_module_1_0_0_t* me,
                      ompi_process_name_t *name, int flags)
{
    return OMPI_ERROR;
}


int
mca_pcm_rsh_kill_job(struct mca_pcm_base_module_1_0_0_t* me, 
                     mca_ns_base_jobid_t jobid, int flags)
{
    /* BWB - do stuff */
        
    mca_pcm_base_remove_job(jobid);

    return OMPI_ERROR;
}

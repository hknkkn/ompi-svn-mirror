/* -*- C -*-
 * 
 * $HEADER$
 *
 */

#include "ompi_config.h"

#include "include/constants.h"
#include "mca/pcm/pcm.h"
#include "mca/pcm/rsh/src/pcm_rsh.h"


int
mca_pcm_rsh_kill_proc(ompi_process_name_t *name, int flags)
{
    return OMPI_ERROR;
}


int
mca_pcm_rsh_kill_job(int jobid, int flags)
{
    return OMPI_ERROR;
}

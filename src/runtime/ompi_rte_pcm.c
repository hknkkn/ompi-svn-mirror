/*
 * $HEADER$
 */

#include "ompi_config.h"

#include "include/constants.h"
#include "runtime/runtime.h"
#include "runtime/runtime_types.h"
#include "mca/pcm/pcm.h"
#include "mca/pcmclient/pcmclient.h"
#include "mca/pcmclient/base/base.h"

extern mca_pcm_base_module_t *mca_pcm;

bool
ompi_rte_can_spawn(void)
{
    if (NULL == mca_pcm) {
        return OMPI_ERROR;
    }

    /* BWB - fix me, fix me, fix me */
    return true;
}


int
ompi_rte_spawn_procs(mca_ns_base_jobid_t jobid, ompi_list_t *schedule_list)
{
    if (NULL == mca_pcm->pcm_spawn_procs) {
        return OMPI_ERROR;
    }

    return mca_pcm->pcm_spawn_procs(mca_pcm, jobid, schedule_list);
}


ompi_process_name_t*
ompi_rte_get_self(void)
{
    if (NULL == mca_pcmclient.pcmclient_get_self) {
        return NULL;
    }

    return mca_pcmclient.pcmclient_get_self();
}


int
ompi_rte_get_peers(ompi_process_name_t **peers, size_t *npeers)
{
    if (NULL == mca_pcmclient.pcmclient_get_peers) {
        return OMPI_ERROR;
    }

    return mca_pcmclient.pcmclient_get_peers(peers, npeers);
}


int
ompi_rte_kill_proc(ompi_process_name_t *name, int flags)
{
    if (NULL == mca_pcm->pcm_kill_proc) {
        return OMPI_ERROR;
    }

    return mca_pcm->pcm_kill_proc(mca_pcm, name, flags);
}


int
ompi_rte_kill_job(mca_ns_base_jobid_t jobid, int flags)
{
    if (NULL == mca_pcm->pcm_kill_job) {
        return OMPI_ERROR;
    }

    return mca_pcm->pcm_kill_job(mca_pcm, jobid, flags);
}

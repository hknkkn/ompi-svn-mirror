/*
 * $HEADER$
 */

/** @file **/

/* #define _GNU_SOURCE */

#include "ompi_config.h"

#include "include/constants.h"
#include "event/event.h"
#include "util/output.h"
#include "threads/mutex.h"
#include "mca/mca.h"
#include "mca/base/base.h"
#include "mca/base/mca_base_param.h"
#include "mca/pcm/base/base.h"
#include "mca/pcmclient/base/base.h"
#include "mca/llm/base/base.h"
#include "mca/oob/oob.h"
#include "mca/ns/base/base.h"
#include "mca/gpr/base/base.h"
#include "util/proc_info.h"
#include "util/session_dir.h"
#include "util/sys_info.h"

#include "runtime/runtime.h"

int ompi_rte_init_stage2(bool *allow_multi_user_threads, bool *have_hidden_threads)
{
    int ret;
    bool user_threads, hidden_threads;
    char *jobid_str=NULL, *procid_str=NULL;

    /*
     * Name Server
     */
    if (OMPI_SUCCESS != (ret = mca_ns_base_open())) {
	/* JMS show_help */
	printf("show_help: ompi_rte_init failed in ns_base_open\n");
	return ret;
    }
    user_threads = true;
    hidden_threads = false;
    if (OMPI_SUCCESS != (ret = mca_ns_base_select(&user_threads,
						  &hidden_threads))) {
	/* JMS show_help */
	printf("show_help: ompi_rte_init failed in ns_base_select\n");
	return ret;
    }
    *allow_multi_user_threads &= user_threads;
    *have_hidden_threads |= hidden_threads;

    /*
     * Process Control and Monitoring Client
     */
    if (OMPI_SUCCESS != (ret = mca_pcmclient_base_open())) {
	/* JMS show_help */
	printf("show_help: ompi_rte_init failed in pcmclient_base_open\n");
	return ret;
    }
    user_threads = true;
    hidden_threads = false;
    if (OMPI_SUCCESS != (ret = mca_pcmclient_base_select(&user_threads, 
							 &hidden_threads))) {
	printf("show_help: ompi_rte_init failed in pcmclient_base_select\n");
	/* JMS show_help */
	return ret;
    }
    *allow_multi_user_threads &= user_threads;
    *have_hidden_threads |= hidden_threads;

    /*
     * Allocation code - open only.  pcm will init if needed
     */
    if (OMPI_SUCCESS != (ret = mca_llm_base_open())) {
	/* JMS show_help */
	printf("show_help: ompi_rte_init failed in llm_base_open\n");
	return ret;
    }

    /*
     * Process Control and Monitoring
     */
    if (OMPI_SUCCESS != (ret = mca_pcm_base_open())) {
	/* JMS show_help */
	printf("show_help: ompi_rte_init failed in pcm_base_open\n");
	return ret;
    }
    user_threads = true;
    hidden_threads = false;
    if (OMPI_SUCCESS != (ret = mca_pcm_base_select(&user_threads, 
						   &hidden_threads))) {
	printf("show_help: ompi_rte_init failed in pcm_base_select\n");
	/* JMS show_help */
	return ret;
    }
    *allow_multi_user_threads &= user_threads;
    *have_hidden_threads |= hidden_threads;

    /*
     * Registry 
     */
    if (OMPI_SUCCESS != (ret = mca_gpr_base_open())) {
	/* JMS show_help */
	printf("show_help: ompi_rte_init failed in mca_gpr_base_open()\n");
	return ret;
    }
    user_threads = true;
    hidden_threads = false;
    if (OMPI_SUCCESS != (ret = mca_gpr_base_select(&user_threads, 
						   &hidden_threads))) {
	/* JMS show_help */
	printf("show_help: ompi_rte_init failed in mca_gpr_base_select()\n");
	return ret;
    }
    *allow_multi_user_threads &= user_threads;
    *have_hidden_threads |= hidden_threads;

    /*
     * Fill in the various important structures
     */
    /* proc structure startup */
    ompi_proc_info();  

    if (ompi_rte_debug_flag) {
	ompi_output(0, "proc info for proc %s", ompi_name_server.get_proc_name_string(ompi_process_info.name));
    }

    /* session directory */
    jobid_str = ompi_name_server.get_jobid_string(ompi_process_info.name);
    procid_str = ompi_name_server.get_vpid_string(ompi_process_info.name);
 
    if (ompi_rte_debug_flag) {
	ompi_output(0, "[%d,%d,%d] setting up session dir with", ompi_process_info.name->cellid, ompi_process_info.name->jobid, ompi_process_info.name->vpid);
	if (NULL != ompi_process_info.tmpdir_base) {
	    ompi_output(0, "\ttmpdir %s", ompi_process_info.tmpdir_base);
	}
	ompi_output(0, "\tuniverse %s", ompi_process_info.my_universe);
	ompi_output(0, "\tuser %s", ompi_system_info.user);
	ompi_output(0, "\thost %s", ompi_system_info.nodename);
	ompi_output(0, "\tjobid %s", jobid_str);
	ompi_output(0, "\tprocid %s", procid_str);
    }
   if (OMPI_ERROR == ompi_session_dir(true,
				       ompi_process_info.tmpdir_base,
				       ompi_system_info.user,
				       ompi_system_info.nodename, NULL, 
				       ompi_process_info.my_universe,
				       jobid_str, procid_str)) {
	if (jobid_str != NULL) free(jobid_str);
	if (procid_str != NULL) free(procid_str);
	return OMPI_ERROR;
    }

   /*
    *  Register process info we/ seed daemon.
    */
   if (OMPI_SUCCESS != (ret = ompi_rte_register())) {
       ompi_output(0, "ompi_rte_init: failed in ompi_rte_register()\n");
       return ret;
   }

    /*
     * Call back into OOB to allow do any final initialization
     * (e.g. put contact info in register).
     */
    if (OMPI_SUCCESS != (ret = mca_oob_base_module_init())) {
       ompi_output(0, "ompi_rte_init: failed in mca_oob_base_module_init()\n");
       return ret;
    }

    /* 
     * All done 
     */
    return OMPI_SUCCESS;
}


/*
 *  interface type support
 */

/** constructor for \c ompi_rte_node_schedule_t */
static
void
ompi_rte_int_node_schedule_construct(ompi_object_t *obj)
{
    ompi_rte_node_schedule_t *sched = (ompi_rte_node_schedule_t*) obj;
    sched->nodelist = OBJ_NEW(ompi_list_t);
}


/** destructor for \c ompi_rte_node_schedule_t */
static
void
ompi_rte_int_node_schedule_destruct(ompi_object_t *obj)
{
    ompi_rte_node_schedule_t *sched = (ompi_rte_node_schedule_t*) obj;
    ompi_rte_node_allocation_t *node;
    ompi_list_item_t *item;

    if (NULL == sched->nodelist) return;

    while (NULL != (item = ompi_list_remove_first(sched->nodelist))) {
        node = (ompi_rte_node_allocation_t*) item;
        OBJ_RELEASE(node);
    }

    OBJ_RELEASE(sched->nodelist);
}


/** constructor for \c ompi_rte_node_allocation_t */
static
void
ompi_rte_int_node_allocation_construct(ompi_object_t *obj)
{
    ompi_rte_node_allocation_t *node = (ompi_rte_node_allocation_t*) obj;
    node->start = 0;
    node->nodes = 0;
    node->count = 0;
    node->data = NULL;
}


/** destructor for \c ompi_rte_node_allocation_t */
static
void
ompi_rte_int_node_allocation_destruct(ompi_object_t *obj)
{
    ompi_rte_node_allocation_t *node = (ompi_rte_node_allocation_t*) obj;

    if (NULL == node->data) return;

    OBJ_RELEASE(node->data);
}


/** constructor for \c ompi_rte_valuepair_t */
static
void
ompi_rte_int_valuepair_construct(ompi_object_t *obj)
{
    ompi_rte_valuepair_t *valpair = (ompi_rte_valuepair_t*) obj;
    valpair->key = NULL;
    valpair->value = NULL;
}


/** destructor for \c ompi_rte_valuepair_t */
static
void
ompi_rte_int_valuepair_destruct(ompi_object_t *obj)
{
    ompi_rte_valuepair_t *valpair = (ompi_rte_valuepair_t*) obj;
    if (NULL != valpair->key) free(valpair->key);
    if (NULL != valpair->value) free(valpair->value);
}

/** create instance information for \c ompi_rte_node_schedule_t */
OBJ_CLASS_INSTANCE(ompi_rte_node_schedule_t, ompi_list_item_t,
                   ompi_rte_int_node_schedule_construct,
                   ompi_rte_int_node_schedule_destruct);
/** create instance information for \c ompi_rte_node_allocation_t */
OBJ_CLASS_INSTANCE(ompi_rte_node_allocation_t, ompi_list_item_t, 
                   ompi_rte_int_node_allocation_construct, 
                   ompi_rte_int_node_allocation_destruct);
/** create instance information for \c ompi_rte_valuepair_t */
OBJ_CLASS_INSTANCE(ompi_rte_valuepair_t, ompi_list_item_t, 
                   ompi_rte_int_valuepair_construct,
                   ompi_rte_int_valuepair_destruct);
/** create instance information for \c ompi_rte_node_allocation_data_t */
OBJ_CLASS_INSTANCE(ompi_rte_node_allocation_data_t, ompi_object_t, 
                   NULL, NULL);

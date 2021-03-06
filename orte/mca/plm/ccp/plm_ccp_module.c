/*
 * Copyright (c) 2004-2007 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2010 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2011-2012 Los Alamos National Security, LLC.  All rights
 *                         reserved. 
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 *
 */

#include "orte_config.h"
#include "orte/constants.h"
#include "orte/types.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <signal.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifdef HAVE_SCHED_H
#include <sched.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <errno.h>
#include <comutil.h>

#include "opal/mca/installdirs/installdirs.h"
#include "opal/mca/event/event.h"
#include "opal/util/argv.h"
#include "opal/util/output.h"
#include "opal/util/opal_environ.h"
#include "opal/util/basename.h"
#include "opal/mca/base/mca_base_param.h"

#include "orte/util/name_fns.h"
#include "orte/runtime/orte_globals.h"
#include "orte/runtime/orte_wait.h"
#include "orte/mca/errmgr/errmgr.h"
#include "orte/mca/rmaps/rmaps.h"
#include "orte/mca/state/state.h"

#include "orte/mca/plm/plm.h"
#include "orte/mca/plm/base/plm_private.h"
#include "plm_ccp.h"


/* Import the Windows CCP API. */
#import "ccpapi.tlb" named_guids no_namespace raw_interfaces_only   \
    rename("SetEnvironmentVariable","SetEnvVar")                    \
    rename("GetJob", "GetSingleJob")                                \
    rename("AddJob", "AddSingleJob")

/* Include the library for ::ConvertBSTRToString */
#pragma comment(lib, "comsuppw.lib")

/*
 * Local functions
 */
static int plm_ccp_init(void);
static int plm_ccp_launch_job(orte_job_t *jdata);
static int plm_ccp_terminate_orteds(void);
static int plm_ccp_signal_job(orte_jobid_t jobid, int32_t signal);
static int plm_ccp_finalize(void);

static int plm_ccp_connect(ICluster* pCluster);
static int plm_ccp_disconnect(void);

void plm_get_cluster_message(ICluster* pCluster);
static char *plm_ccp_commandline(char *prefix, char *node_name, int argc, char **argv);

/*
 * Global variable
 */
orte_plm_base_module_t orte_plm_ccp_module = {
    plm_ccp_init,
    orte_plm_base_set_hnp_name,
    plm_ccp_launch_job,
    NULL,
    orte_plm_base_orted_terminate_job,
    plm_ccp_terminate_orteds,
    orte_plm_base_orted_kill_local_procs,
    plm_ccp_signal_job,
    plm_ccp_finalize
};

static void launch_daemons(int fd, short args, void *cbdata);

/**
* Init the module
 */
static int plm_ccp_init(void)
{
    int rc;
    
    /* point to our launch command */
    if (ORTE_SUCCESS != (rc = orte_state.add_job_state(ORTE_JOB_STATE_LAUNCH_DAEMONS,
                                                       launch_daemons, ORTE_SYS_PRI))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    
    if (ORTE_SUCCESS != (rc = orte_plm_base_comm_start())) {
        ORTE_ERROR_LOG(rc);
    }

    /* we assign daemon nodes at launch */
    orte_plm_globals.daemon_nodes_assigned_at_launch = true;

    return rc;
}


static int plm_ccp_launch_job(orte_job_t *jdata)
{  
    if (ORTE_JOB_CONTROL_RESTART & jdata->controls) {
        /* this is a restart situation - skip to the mapping stage */
        ORTE_ACTIVATE_JOB_STATE(jdata, ORTE_JOB_STATE_MAP);
    } else {
        /* new job - set it up */
        ORTE_ACTIVATE_JOB_STATE(jdata, ORTE_JOB_STATE_INIT);
    }
    return ORTE_SUCCESS;
}

/* When working in this function, ALWAYS jump to "cleanup" if
 * you encounter an error so that orterun will be woken up and
 * the job can cleanly terminate
 */
static void launch_daemons(int fd, short args, void *cbdata)
{
    orte_state_caddy_t *state = (orte_state_caddy_t*)cbdata;
    orte_job_t *jdata = state->jdata;

    orte_app_context_t *app;
    orte_node_t *node;
    orte_std_cntr_t launched = 0, i; 

    orte_job_map_t *map = NULL;
    int argc, rc, proc_vpid_index;
    char *param, **env = NULL, *var, **argv = NULL;
    bool connected = false;
    char *bin_base = NULL, *lib_base = NULL, *command_line;
    
    struct timeval completionstop, launchstart, launchstop;
    struct timeval jobstart, jobstop;
    int maxtime=0, mintime=99999999, maxiter = 0, miniter = 0, deltat;
    float avgtime=0.0;
    bool failed_launch = true;
    mode_t current_umask;
    IClusterEnumerable* pNodesCollection = NULL;
    IEnumVARIANT* pNodes = NULL;
    VARIANT v;

    INode* pNode = NULL;
    HRESULT hr = S_OK;
    ICluster* pCluster = NULL;
    IJob* pJob = NULL;
    long job_id, num_processors = 0, idle_processors = 0;
    IClusterCounter* pClusterCounter = NULL;
    ITask* pTask = NULL;
    JobPriority job_priority = JobPriority_Normal;

    orte_jobid_t failed_job; 
    orte_job_state_t job_state = ORTE_JOB_STATE_NEVER_LAUNCHED;
    orte_job_t *daemons;

    /* default to declaring the daemon launch failed */ 
    failed_job = ORTE_PROC_MY_NAME->jobid; 

    /* check for timing request - get start time if so */
    if (orte_timing) {
        if (0 != gettimeofday(&jobstart, NULL)) {
            OPAL_OUTPUT_VERBOSE((1, orte_plm_globals.output,
                                 "plm_ccp: could not obtain job start time"));
        }
    }

    /* start by launching the virtual machine */
    daemons = orte_get_job_data_object(ORTE_PROC_MY_NAME->jobid);
    if (ORTE_SUCCESS != (rc = orte_plm_base_setup_virtual_machine(jdata))) {
        ORTE_ERROR_LOG(rc);
        goto cleanup;
    }

    /* if we don't want to launch, then don't attempt to
     * launch the daemons - the user really wants to just
     * look at the proposed process map
     */
    if (orte_do_not_launch) {
        /* set the state to indicate the daemons reported - this
         * will trigger the daemons_reported event and cause the
         * job to move to the following step
         */
        jdata->state = ORTE_JOB_STATE_DAEMONS_LAUNCHED;
        ORTE_ACTIVATE_JOB_STATE(jdata, ORTE_JOB_STATE_DAEMONS_REPORTED);
        OBJ_RELEASE(state);
        return;
    }
    
    OPAL_OUTPUT_VERBOSE((1, orte_plm_globals.output,
                         "%s plm:rsh: launching vm",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME)));
    
    /* Get the map for this job */
    if (NULL == (map = daemons->map)) {
        ORTE_ERROR_LOG(ORTE_ERR_NOT_FOUND);
        rc = ORTE_ERR_NOT_FOUND;
        goto cleanup;
    }

    if (0 == map->num_new_daemons) {
        /* set the state to indicate the daemons reported - this
         * will trigger the daemons_reported event and cause the
         * job to move to the following step
         */
        state->jdata->state = ORTE_JOB_STATE_DAEMONS_LAUNCHED;
        if (ORTE_JOB_STATE_DAEMONS_REPORTED == daemons->state) {
            ORTE_ACTIVATE_JOB_STATE(state->jdata, ORTE_JOB_STATE_DAEMONS_REPORTED);
        }
        OBJ_RELEASE(state);
        return;
    }
    
    /* add the daemon command (as specified by user) */
    argc = 0;
    argv = NULL;
    orte_plm_base_setup_orted_cmd(&argc, &argv);

    /* Add basic orted command line options */
    orte_plm_base_orted_append_basic_args(&argc, &argv, "env",
                                          &proc_vpid_index,
                                          NULL);

    if (0 < opal_output_get_verbosity(orte_plm_globals.output)) {
        param = opal_argv_join(argv, ' ');
        OPAL_OUTPUT_VERBOSE((1, orte_plm_globals.output,
                             "%s plm:ccp: final top-level argv:\n\t%s",
                             ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                             (NULL == param) ? "NULL" : param));
        if (NULL != param) free(param);
    }

    /* CCP is not thread safe. Use the apartment model. */
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    /* Create the Cluster object. */
    hr = CoCreateInstance( __uuidof(Cluster),
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           __uuidof(ICluster),
                           reinterpret_cast<void **> (&pCluster) );
    if (FAILED(hr)) {
        opal_output(orte_plm_globals.output,
                    "plm:ccp: failed to create cluster object!");
        goto cleanup;
    }

    /* Connect to the head node. */
    rc = plm_ccp_connect(pCluster);
    if (ORTE_SUCCESS != rc) {
        goto cleanup;
    }
    connected = true;

    hr = pCluster->CreateJob(&pJob);
    if (FAILED(hr)) {
        plm_get_cluster_message(pCluster);
        opal_output(orte_plm_globals.output,
                    "plm:ccp:failed to create cluster object!");
        goto cleanup;
    }
    /* Figure out the basenames for the libdir and bindir.  There is a
       lengthy comment about this in plm_rsh_module.c explaining all
       the rationale for how / why we're doing this. */
    lib_base = opal_basename(opal_install_dirs.libdir);
    bin_base = opal_basename(opal_install_dirs.bindir);

    /* setup environment */
    env = opal_argv_copy(orte_launch_environ);

    /* add our umask -- see big note in orted.c */
    current_umask = _umask(0);
    _umask(current_umask);
    asprintf(&var, "0%o", current_umask);
    opal_setenv("ORTE_DAEMON_UMASK_VALUE", var, true, &env);
    free(var);
    
    /* If we have a prefix, then modify the PATH and
       LD_LIBRARY_PATH environment variables. We only allow
       a single prefix to be specified. Since there will
       always be at least one app_context, we take it from
       there
    */
    app = (orte_app_context_t*)opal_pointer_array_get_item(jdata->apps, 0);
    if (NULL != app->prefix_dir) {
        char *newenv;
        
        for (i = 0; NULL != env && NULL != env[i]; ++i) {
            /* Reset PATH */
            if (0 == strncmp("PATH=", env[i], 5)) {
                asprintf(&newenv, "%s/%s:%s", 
                         app->prefix_dir, bin_base, env[i] + 5);
                OPAL_OUTPUT_VERBOSE((1, orte_plm_globals.output,
                                     "%s plm:ccp: resetting PATH: %s",
                                     ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                                     newenv));
                opal_setenv("PATH", newenv, true, &env);
                free(newenv);
            } 
            
            /* Reset LD_LIBRARY_PATH */
            else if (0 == strncmp("LD_LIBRARY_PATH=", env[i], 16)) {
                asprintf(&newenv, "%s/%s:%s", 
                         app->prefix_dir, lib_base, env[i] + 16);
                OPAL_OUTPUT_VERBOSE((1, orte_plm_globals.output,
                                     "%s plm:ccp: resetting LD_LIBRARY_PATH: %s",
                                     ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                                     newenv));
                opal_setenv("LD_LIBRARY_PATH", newenv, true, &env);
                free(newenv);
            } 
        }
    }

    /* This has already been done in RAS, but I have to do it again here. 
     * Because the node structure doesn't have num_processor member. */

    /* Get the collection of nodes. */
    hr = pCluster->get_ComputeNodes(&pNodesCollection);

    /* Get the enumerator used to iterate through the collection. */
    hr = pNodesCollection->GetEnumerator(&pNodes);

    VariantInit(&v);

    int *num_procs;
    num_procs = (int *) malloc(sizeof(int)*map->num_nodes);

    /* Loop through the collection. */
    while (hr = pNodes->Next(1, &v, NULL) == S_OK) {
        v.pdispVal->QueryInterface(IID_INode, reinterpret_cast<void **> (&pNode));

        /* Iterate through each of the nodes and check to sum up all the processors. */
        for (i = 0; i < map->nodes->size; i++) {
            if (NULL == (node = (orte_node_t*)opal_pointer_array_get_item(map->nodes, i))) {
                continue;
            }

            BSTR node_name;
            hr = pNode->get_Name(&node_name);
            
            if( 0 == strcmp(_com_util::ConvertBSTRToString(node_name), node->name)) {
                /* Get available number of processors on required node. */
                hr = pNode->get_NumberOfIdleProcessors(&idle_processors);
                num_procs[i] = idle_processors;
                num_processors += idle_processors;
            }
        }
    }

    if(NULL != mca_plm_ccp_component.job_name){
        pJob->put_Name(_bstr_t(mca_plm_ccp_component.job_name));
    } else {
        pJob->put_Name(_bstr_t((*app).app));
    }

    pJob->put_MinimumNumberOfProcessors(num_processors);
    if (FAILED(hr)) {
        OPAL_OUTPUT_VERBOSE((1, orte_plm_globals.output,
                             "plm:ccp:failed to put min num of processors!"));
        goto cleanup;
    }

    pJob->put_MaximumNumberOfProcessors(num_processors);
    if (FAILED(hr)) {
        OPAL_OUTPUT_VERBOSE((1, orte_plm_globals.output,
                             "plm:ccp:failed to put max num of processors!"));
        goto cleanup;
    }

    hr = pJob->put_Priority(job_priority);
    if (FAILED(hr)) {
        OPAL_OUTPUT_VERBOSE((1, orte_plm_globals.output,
                             "plm:ccp:failed to set proiority!"));
        goto cleanup;
    }

    hr = pJob->SetExtendedJobTerm(_bstr_t(L"extended terms"), _bstr_t(L"TermValue"));
  
    /* set the job state to indicate we attempted to launch */
    job_state = ORTE_JOB_STATE_FAILED_TO_START;
    
    /* Iterate through each of the nodes and spin
     * up a daemon.
     */
    for (i = 0; i < map->nodes->size; i++) {
        char* vpid_string;
        if (NULL == (node = (orte_node_t*)opal_pointer_array_get_item(map->nodes, i))) {
            continue;
        }

        /* if this daemon already exists, don't launch it! */
        if (node->daemon_launched) {
            continue;
        }

        OPAL_OUTPUT_VERBOSE((1, orte_plm_globals.output,
                             "%s plm:ccp: launching on node %s",
                             ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                             node->name));
        
        /* setup process name */
        rc = orte_util_convert_vpid_to_string(&vpid_string, node->daemon->name.vpid);
        if (ORTE_SUCCESS != rc) {
            OPAL_OUTPUT_VERBOSE((1, orte_plm_globals.output,
                                 "plm:ccp: unable to get daemon vpid as string"));
            exit(-1);
        }
        free(argv[proc_vpid_index]);
        argv[proc_vpid_index] = strdup(vpid_string);
        free(vpid_string);

        /* exec the daemon */
        if (0 < opal_output_get_verbosity(orte_plm_globals.output)) {
            param = opal_argv_join(argv, ' ');
            OPAL_OUTPUT_VERBOSE((1, orte_plm_globals.output,
                                 "%s plm:ccp: executing:\n\t%s",
                                 ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                                 (NULL == param) ? "NULL" : param));
            if (NULL != param) free(param);
        }

        /* check for timing request - get start time if so */
        if (orte_timing) {
            if (0 != gettimeofday(&launchstart, NULL)) {
                OPAL_OUTPUT_VERBOSE((1, orte_plm_globals.output,
                                     "plm_ccp: could not obtain start time"));
                launchstart.tv_sec = 0;
                launchstart.tv_usec = 0;
            }
        }
        /* Set terms for task. */
        hr = pCluster->CreateTask(&pTask);
        if (FAILED(hr)) {
            plm_get_cluster_message(pCluster);
            OPAL_OUTPUT_VERBOSE((1, orte_plm_globals.output,
                                 "plm:ccp:failed to create task object!"));
            goto cleanup;
        }
        
        pTask->put_MinimumNumberOfProcessors(num_procs[i]);
        if (FAILED(hr)) {
            OPAL_OUTPUT_VERBOSE((1, orte_plm_globals.output,
                                 "plm:ccp:failed to create task object!"));
            goto cleanup;
        }

        pTask->put_MaximumNumberOfProcessors(num_procs[i]);
        if (FAILED(hr)) {
            OPAL_OUTPUT_VERBOSE((1, orte_plm_globals.output,
                                 "plm:ccp:failed to create task object!"));
            goto cleanup;
        }

        pTask->put_RequiredNodes(_bstr_t(node->name));
        if (FAILED(hr)) {
            OPAL_OUTPUT_VERBOSE((1, orte_plm_globals.output,
                                 "plm:ccp:failed to set required nodes!"));
            goto cleanup;
        }
        
        /* Prepare the command line a little bit. */
        command_line = plm_ccp_commandline(app->prefix_dir, node->name, argc, argv);

        hr = pTask->put_CommandLine(_bstr_t(command_line));
        if (FAILED(hr)) {
            OPAL_OUTPUT_VERBOSE((1, orte_plm_globals.output,
                                 "plm:ccp:failed to put command line!"));
            goto cleanup;
        }

        if( NULL != mca_plm_ccp_component.stdout_file ) {
            hr = pTask->put_Stdout(_bstr_t(mca_plm_ccp_component.stdout_file));
            if (FAILED(hr)) {
                OPAL_OUTPUT_VERBOSE((1, orte_plm_globals.output,
                                     "plm:ccp:failed to set stdout!"));
                goto cleanup;
            }
        }

        if( NULL != mca_plm_ccp_component.stderr_file) {
            hr = pTask->put_Stderr(_bstr_t(mca_plm_ccp_component.stderr_file));
            if (FAILED(hr)) {
                OPAL_OUTPUT_VERBOSE((1, orte_plm_globals.output,
                                     "plm:ccp:failed to set stderr!"));
                goto cleanup;
            }
        }

        hr = pJob->AddTask(pTask);
        if (FAILED(hr)) {
            OPAL_OUTPUT_VERBOSE((1, orte_plm_globals.output,
                                 "plm:ccp:failed to add task!"));
            goto cleanup;
        }

#if 0
        /* RHC: you definitely cannot do that here!! Use
        * nanosleep if you need to wait a little while
        */
        /* Allow some progress to occur */
        opal_event_loop(orte_event_base, OPAL_EVLOOP_NONBLOCK);
#endif
        launched++;

        pTask->Release();
    }
 
    /* Add job to the queue. */
    hr = pCluster->QueueJob(pJob, NULL, NULL, VARIANT_TRUE, 0, &job_id);
    if (SUCCEEDED(hr)) {
        OPAL_OUTPUT_VERBOSE((1, orte_plm_globals.output,
                             "Added job %d to scheduling queue.\n", job_id));
    }else {
        plm_get_cluster_message(pCluster);
    }

    OPAL_OUTPUT_VERBOSE((1, orte_plm_globals.output,
                         "%s plm:ccp:launch: finished spawning orteds",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME)));

    /* set the job state to indicate the daemons are launched */
    jdata->state = ORTE_JOB_STATE_DAEMONS_LAUNCHED;
    
    /* set the job state to indicate the daemons are launched */
    state->jdata->state = ORTE_JOB_STATE_DAEMONS_LAUNCHED;
    
 cleanup:
    if (NULL != argv) {
        opal_argv_free(argv);
    }

    if (NULL != env) {
        opal_argv_free(env);
    }
    
    if (connected) {
        plm_ccp_disconnect();
    }
    
    if (NULL != lib_base) {
        free(lib_base);
    }

    if (NULL != bin_base) {
        free(bin_base);
    }

    /* check for failed launch - if so, force terminate */
    if (failed_launch) {
        ORTE_TERMINATE(ORTE_ERROR_DEFAULT_EXIT_CODE);
    }
        
    OPAL_OUTPUT_VERBOSE((1, orte_plm_globals.output,
                         "%s plm:ccp:launch: finished",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME)));

    OBJ_RELEASE(state);
    return;
}


/**
 * Terminate the orteds for a given job
 */
int plm_ccp_terminate_orteds()
{
    int rc;
    
    /* now tell them to die */
    if (orte_abnormal_term_ordered) {
        /* cannot know if a daemon is able to
         * tell us it died, so just ensure they
         * all terminate
         */
        if (ORTE_SUCCESS != (rc = orte_plm_base_orted_exit(ORTE_DAEMON_HALT_VM_CMD))) {
            ORTE_ERROR_LOG(rc);
        }
    } else {
        /* we need them to "phone home", though,
         * so we can know that they have exited
         */
        if (ORTE_SUCCESS != (rc = orte_plm_base_orted_exit(ORTE_DAEMON_EXIT_CMD))) {
            ORTE_ERROR_LOG(rc);
        }
    }

    return rc;
}


static int plm_ccp_signal_job(orte_jobid_t jobid, int32_t signal)
{
    int rc;
    
    /* order them to pass this signal to their local procs */
    if (ORTE_SUCCESS != (rc = orte_plm_base_orted_signal_local_procs(jobid, signal))) {
        ORTE_ERROR_LOG(rc);
    }
    
    return rc;
}


/*
 * Free stuff
 */
static int plm_ccp_finalize(void)
{
    int rc;
    
    /* cleanup any pending recvs */
    if (ORTE_SUCCESS != (rc = orte_plm_base_comm_stop())) {
        ORTE_ERROR_LOG(rc);
    }

    return ORTE_SUCCESS;
}


static int plm_ccp_connect(ICluster* pCluster)
{
    size_t i, len;
    char *cluster_head = NULL;
    HRESULT hr = S_OK;

    if (NULL == orte_ccp_headnode) {
        /* Get the cluster head nodes name */
        _dupenv_s(&cluster_head, &len, "LOGONSERVER");

        if(cluster_head == NULL) {
            OPAL_OUTPUT_VERBOSE((1, orte_plm_globals.output,
                                "plm:ccp:allocate: connot find cluster head node!"));
            return ORTE_ERROR;
        }

        /* Get rid of the beginning '//'. */
        for( i = 0; i < len; i++){
            cluster_head[i] = cluster_head[i+2];
            cluster_head[i+2] = '\0';
        }
    } else {
        cluster_head = orte_ccp_headnode;
    }

    /* Connect to the cluster's head node */
    hr = pCluster->Connect(_bstr_t(cluster_head));
    if (FAILED(hr)) {
        OPAL_OUTPUT_VERBOSE((1, orte_plm_globals.output,
                            "plm:ccp:allocate: connection failed!"));
        return ORTE_ERROR;
    }
    OPAL_OUTPUT_VERBOSE((1, orte_plm_globals.output,
                        "Connected to Cluster: %s. \n", cluster_head));
    return ORTE_SUCCESS;
}


static int plm_ccp_disconnect(void)
{
    return ORTE_SUCCESS;
}


/* Generate the proper command line according to the env. */
static char *plm_ccp_commandline(char *prefix, char *node_name, int argc, char **argv)
{
    char *commandline;
    size_t i, len = 0;

    for( i = 0; i < argc; i++ ) {
        len += strlen(argv[i]) + 1;
    }

    if(NULL != prefix) {
        commandline = (char*)malloc(len + strlen(prefix) + 8);
        memset(commandline, 0, len + strlen(prefix) + 8);
        commandline[0] = '"';
        strcat(commandline, prefix);
        strcat(commandline, "\\bin\"\\");
    } else {
        commandline = (char*)malloc(len + 1);
        memset(commandline, 0, len + 1);
    }


    for(i=0;i<argc;i++) {

        /* Append command args, and separate them with spaces. */
        strcat(commandline, argv[i]);

        commandline[strlen(commandline)]=' ';
    }
    return commandline;
}


void plm_get_cluster_message(ICluster* pCluster)
{
    HRESULT hr = S_OK;
    BSTR message = NULL;

    hr = pCluster->get_ErrorMessage(&message);
    if (SUCCEEDED(hr)) {
        OPAL_OUTPUT_VERBOSE((1, orte_plm_globals.output,
                            _com_util::ConvertBSTRToString(message)));
        SysFreeString(message);
    }
    else {
        OPAL_OUTPUT_VERBOSE((1, orte_plm_globals.output,
                            "pCluster->get_ErrorMessage failed.\n"));
    }
}

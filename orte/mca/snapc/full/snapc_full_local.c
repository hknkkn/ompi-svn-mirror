/*
 * Copyright (c) 2004-2009 The Trustees of Indiana University.
 *                         All rights reserved.
 * Copyright (c) 2004-2005 The Trustees of the University of Tennessee.
 *                         All rights reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2007      Evergrid, Inc. All rights reserved.
 *
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

#include "orte_config.h"

#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif  /* HAVE_UNISTD_H */
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif  /* HAVE_FCNTL_H */
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif  /* HAVE_SYS_TYPES_H */
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>  /* for mkfifo */
#endif  /* HAVE_SYS_STAT_H */
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#include "opal/runtime/opal_progress.h"
#include "opal/runtime/opal_cr.h"
#include "opal/util/output.h"
#include "opal/util/opal_environ.h"
#include "opal/util/os_dirpath.h"
#include "opal/util/basename.h"
#include "opal/mca/mca.h"
#include "opal/mca/base/base.h"
#include "opal/mca/base/mca_base_param.h"
#include "opal/mca/crs/crs.h"
#include "opal/mca/crs/base/base.h"

#include "orte/util/show_help.h"
#include "orte/util/name_fns.h"
#include "orte/runtime/orte_wait.h"
#include "orte/runtime/orte_globals.h"
#include "orte/mca/rml/rml.h"
#include "orte/mca/rml/rml_types.h"
#include "orte/mca/odls/odls.h"
#include "orte/mca/odls/base/odls_private.h"
#include "orte/mca/errmgr/errmgr.h"

#include "orte/mca/snapc/snapc.h"
#include "orte/mca/snapc/base/base.h"

#include "snapc_full.h"

/************************************
 * Locally Global vars & functions :)
 ************************************/
static orte_jobid_t current_local_jobid = 0;
static opal_list_t snapc_local_vpids;
static int current_job_ckpt_state = ORTE_SNAPC_CKPT_STATE_NONE;


static bool snapc_local_hnp_recv_issued = false;
static int  snapc_full_local_start_hnp_listener(void);
static int  snapc_full_local_stop_hnp_listener(void);
static void snapc_full_local_hnp_cmd_recv(int status,
                                          orte_process_name_t* sender,
                                          opal_buffer_t* buffer,
                                          orte_rml_tag_t tag,
                                          void* cbdata);
static void snapc_full_process_hnp_request_cmd(int fd, short event, void *cbdata);

static bool snapc_local_app_recv_issued = false;
static int  snapc_full_local_start_app_listener(void);
static int  snapc_full_local_stop_app_listener(void);
static void snapc_full_local_app_cmd_recv(int status,
                                       orte_process_name_t* sender,
                                       opal_buffer_t* buffer,
                                       orte_rml_tag_t tag,
                                       void* cbdata);
static void snapc_full_local_process_app_update_cmd(int fd, short event, void *cbdata);

static orte_snapc_full_app_snapshot_t *find_vpid_snapshot(orte_process_name_t *name );
static int snapc_full_local_get_vpids(void);

static void snapc_full_local_process_job_update_cmd(orte_process_name_t* sender,
                                                    opal_buffer_t* buffer,
                                                    bool quick);

int local_coord_job_state_update(orte_jobid_t jobid,
                                 int    job_ckpt_state,
                                 char **job_ckpt_ref,
                                 char **job_ckpt_loc);

static int local_coord_job_state_update_finished_local(void);

static int snapc_full_local_setup_snapshot_dir(char * snapshot_ref, char * sugg_dir, char **actual_dir);
#if 0
static int snapc_full_establish_dir(void);
#endif
static int snapc_full_get_min_state(void);

static int snapc_full_local_update_coord(int state, bool quick);

static int snapc_full_local_start_checkpoint_all(int ckpt_state);
static int snapc_full_local_start_ckpt_open_comm(orte_snapc_full_app_snapshot_t *vpid_snapshot);
static int snapc_full_local_start_ckpt_handshake_term(orte_snapc_full_app_snapshot_t *vpid_snapshot, bool term);
static int snapc_full_local_start_ckpt_handshake(orte_snapc_full_app_snapshot_t *vpid_snapshot);
static int snapc_full_local_end_ckpt_handshake(orte_snapc_full_app_snapshot_t *vpid_snapshot);
static void snapc_full_local_comm_read_event(int fd, short flags, void *arg);


/************************
 * Function Definitions
 ************************/
int local_coord_init( void )
{
    current_local_jobid = -1;
    current_job_ckpt_state = ORTE_SNAPC_CKPT_STATE_NONE;

    return ORTE_SUCCESS;
}

int local_coord_finalize( void )
{
    if( current_local_jobid >= 0 ) {
        return local_coord_release_job(current_local_jobid);
    }

    current_job_ckpt_state = ORTE_SNAPC_CKPT_STATE_NONE;
    return ORTE_SUCCESS;
}

int local_coord_setup_job(orte_jobid_t jobid)
{
    int ret, exit_status = ORTE_SUCCESS;

    /*
     * Set the jobid that we are responsible for
     */
    current_local_jobid = jobid;
    OPAL_OUTPUT_VERBOSE((10, mca_snapc_full_component.super.output_handle,
                         "Local) Setting up jobid %s\n",
                         ORTE_JOBID_PRINT(current_local_jobid)));

    /*
     * Get the list of vpid's that we care about
     */
    OBJ_CONSTRUCT(&snapc_local_vpids, opal_list_t);
    if( ORTE_SUCCESS != (ret = snapc_full_local_get_vpids()) ) {
        ORTE_ERROR_LOG(ret);
        exit_status = ret;
        goto cleanup;
    }

    /*
     * Wait for the snapshot directory to be established before registering
     * the callbacks since they use the same tag.
     */
#if 0
    if(orte_snapc_base_establish_global_snapshot_dir) {
        if( ORTE_SUCCESS != (ret = snapc_full_establish_dir() ) ) {
            ORTE_ERROR_LOG(ret);
            exit_status = ret;
            goto cleanup;
        }
    }
#endif

    /*
     * Setup Global Coordinator listener
     */
    if( ORTE_SUCCESS != (ret = snapc_full_local_start_hnp_listener() ) ) {
        ORTE_ERROR_LOG(ret);
        exit_status = ret;
        goto cleanup;
    }

    /*
     * Setup Global Coordinator listener for Application updates
     */
    if( ORTE_SUCCESS != (ret = snapc_full_local_start_app_listener() ) ) {
        ORTE_ERROR_LOG(ret);
        exit_status = ret;
        goto cleanup;
    }


    OPAL_OUTPUT_VERBOSE((10, mca_snapc_full_component.super.output_handle,
                         "Local) Finished setup of job %s",
                         ORTE_JOBID_PRINT(current_local_jobid) ));

 cleanup:
    return exit_status;
}

int local_coord_release_job(orte_jobid_t jobid)
{
    int ret, exit_status = ORTE_SUCCESS;
    opal_list_item_t* item = NULL;
    bool is_done = true;

    /*
     * Wait around until we hear back from the checkpoint requests that
     * we have outstanding.
     */
    do {
        is_done = true;

        for(item  = opal_list_get_first(&snapc_local_vpids);
            item != opal_list_get_end(&snapc_local_vpids);
            item  = opal_list_get_next(item) ) {
            orte_snapc_full_app_snapshot_t *vpid_snapshot;
            vpid_snapshot = (orte_snapc_full_app_snapshot_t*)item;
            
            if(ORTE_SNAPC_CKPT_STATE_NONE     != vpid_snapshot->super.state &&
               ORTE_SNAPC_CKPT_STATE_ERROR    != vpid_snapshot->super.state &&
               ORTE_SNAPC_CKPT_STATE_FINISHED != vpid_snapshot->super.state ) {
                is_done = false;
                break;
            }
        }
        if( !is_done )
            opal_progress();
    } while(!is_done);

    OBJ_DESTRUCT(&snapc_local_vpids);

    /*
     * Stop Global Coordinator listeners
     */
    if( ORTE_SUCCESS != (ret = snapc_full_local_stop_app_listener() ) ) {
        ORTE_ERROR_LOG(ret);
        exit_status = ret;
    }

    if( ORTE_SUCCESS != (ret = snapc_full_local_stop_hnp_listener() ) ) {
        ORTE_ERROR_LOG(ret);
        exit_status = ret;
    }

    return exit_status;
}

/******************
 * Local functions
 ******************/

/******************
 * Setup Listeners
 ******************/
static int snapc_full_local_start_hnp_listener(void)
{
    int ret, exit_status = ORTE_SUCCESS;

    /*
     * Global Coordinator: Do not register a Local listener
     */
    if( ORTE_SNAPC_GLOBAL_COORD_TYPE == (orte_snapc_coord_type & ORTE_SNAPC_GLOBAL_COORD_TYPE)) {
        return ORTE_SUCCESS;
    }

    if (snapc_local_hnp_recv_issued ) {
        return ORTE_SUCCESS;
    }
    
    OPAL_OUTPUT_VERBOSE((5, mca_snapc_full_component.super.output_handle,
                         "Local) Startup Coordinator Channel"));

    /*
     * Coordinator command listener
     */
    if (ORTE_SUCCESS != (ret = orte_rml.recv_buffer_nb(ORTE_NAME_WILDCARD,
                                                       ORTE_RML_TAG_SNAPC_FULL,
                                                       ORTE_RML_PERSISTENT,
                                                       snapc_full_local_hnp_cmd_recv,
                                                       NULL))) {
        ORTE_ERROR_LOG(ret);
        exit_status = ret;
        goto cleanup;
    }

    snapc_local_hnp_recv_issued = true;
    
 cleanup:
    return exit_status;
}

static int snapc_full_local_stop_hnp_listener(void)
{
    int ret, exit_status = ORTE_SUCCESS;

    /*
     * Global Coordinator: Does not register a Local listener
     */
    if( ORTE_SNAPC_GLOBAL_COORD_TYPE == (orte_snapc_coord_type & ORTE_SNAPC_GLOBAL_COORD_TYPE)) {
        return ORTE_SUCCESS;
    }

    if (!snapc_local_hnp_recv_issued ) {
        return ORTE_SUCCESS;
    }

    OPAL_OUTPUT_VERBOSE((5, mca_snapc_full_component.super.output_handle,
                         "Local) Shutdown Coordinator Channel"));
    
    if (ORTE_SUCCESS != (ret = orte_rml.recv_cancel(ORTE_NAME_WILDCARD,
                                                    ORTE_RML_TAG_SNAPC_FULL))) {
        ORTE_ERROR_LOG(ret);
        exit_status = ret;
        goto cleanup;
    }

    snapc_local_hnp_recv_issued = false;
    
 cleanup:
    return exit_status;
}

static int snapc_full_local_start_app_listener(void)
{
    int ret, exit_status = ORTE_SUCCESS;

    if (snapc_local_app_recv_issued) {
        return ORTE_SUCCESS;
    }

    OPAL_OUTPUT_VERBOSE((5, mca_snapc_full_component.super.output_handle,
                         "Local) Startup Application State Channel"));

    /*
     * Coordinator command listener
     */
    if (ORTE_SUCCESS != (ret = orte_rml.recv_buffer_nb(ORTE_NAME_WILDCARD,
                                                       ORTE_RML_TAG_SNAPC,
                                                       ORTE_RML_PERSISTENT,
                                                       snapc_full_local_app_cmd_recv,
                                                       NULL))) {
        ORTE_ERROR_LOG(ret);
        exit_status = ret;
        goto cleanup;
    }

    snapc_local_app_recv_issued = true;

 cleanup:
    return exit_status;
}

static int snapc_full_local_stop_app_listener(void)
{
    int ret, exit_status = ORTE_SUCCESS;
    
    if (!snapc_local_app_recv_issued ) {
        return ORTE_SUCCESS;
    }
    
    OPAL_OUTPUT_VERBOSE((5, mca_snapc_full_component.super.output_handle,
                         "Local) Shutdown Application State Channel"));
    
    if (ORTE_SUCCESS != (ret = orte_rml.recv_cancel(ORTE_NAME_WILDCARD,
                                                    ORTE_RML_TAG_SNAPC))) {
        ORTE_ERROR_LOG(ret);
        exit_status = ret;
        goto cleanup;
    }

    snapc_local_app_recv_issued = false;
    
 cleanup:
    return exit_status;
}

/******************
 * Listener Callbacks
 ******************/
void snapc_full_local_app_cmd_recv(int status,
                                   orte_process_name_t* sender,
                                   opal_buffer_t* buffer,
                                   orte_rml_tag_t tag,
                                   void* cbdata)
{
    if( ORTE_RML_TAG_SNAPC != tag ) {
        opal_output(mca_snapc_full_component.super.output_handle,
                    "Local) Error: Unknown tag: Received a command message from %s (tag = %d).",
                    ORTE_NAME_PRINT(sender), tag);
        ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
        return;
    }

    /*
     * This is the local process contacting us with its updated pid information
     */
    OPAL_OUTPUT_VERBOSE((10, mca_snapc_full_component.super.output_handle,
                         "Local) Application: Update pid operation."));

    /*
     * Do not handle here, use the event engine to queue this until we are out
     * of the RML
     */
    ORTE_MESSAGE_EVENT(sender, buffer, tag, snapc_full_local_process_app_update_cmd);

    return;
}

void snapc_full_local_hnp_cmd_recv(int status,
                                   orte_process_name_t* sender,
                                   opal_buffer_t* buffer,
                                   orte_rml_tag_t tag,
                                   void* cbdata)
{
    if( ORTE_RML_TAG_SNAPC_FULL != tag ) {
        opal_output(mca_snapc_full_component.super.output_handle,
                    "Local) Error: Unknown tag: Received a command message from %s (tag = %d).",
                    ORTE_NAME_PRINT(sender), tag);
        ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
        return;
    }

    /*
     * This is a Global Coordinator message.
     */
    OPAL_OUTPUT_VERBOSE((5, mca_snapc_full_component.super.output_handle,
                         "Local) Receive a command message."));

    /*
     * Do not process this right away - we need to get out of the recv before
     * we process the message to avoid performing the rest of the job while
     * inside this receive! Instead, setup an event so that the message gets processed
     * as soon as we leave the recv.
     *
     * The macro makes a copy of the buffer, which we release above - the incoming
     * buffer, however, is NOT released here, although its payload IS transferred
     * to the message buffer for later processing
     *
     */
    ORTE_MESSAGE_EVENT(sender, buffer, tag, snapc_full_process_hnp_request_cmd);

    return;
}

/******************
 * Listener Handlers
 ******************/
static void snapc_full_local_process_app_update_cmd(int fd, short event, void *cbdata)
{
    int ret;
    orte_message_event_t *mev = (orte_message_event_t*)cbdata;
    orte_snapc_full_app_snapshot_t *vpid_snapshot = NULL;
    orte_snapc_cmd_flag_t command;
    orte_process_name_t proc;
    pid_t proc_pid = 0;
    orte_std_cntr_t count;

    /*
     * Verify the command
     */
    count = 1;
    if (ORTE_SUCCESS != (ret = opal_dss.unpack(mev->buffer, &command, &count, ORTE_SNAPC_CMD))) {
        ORTE_ERROR_LOG(ret);
        goto cleanup;
    }

    if( ORTE_SNAPC_LOCAL_UPDATE_CMD != command ) {
        OPAL_OUTPUT_VERBOSE((10, mca_snapc_full_component.super.output_handle,
                             "Local) Warning: Expected an application command (%d) but received (%d)\n",
                             ORTE_SNAPC_LOCAL_UPDATE_CMD, command));
        goto cleanup;
    }

    /*
     * Unpack the data
     * - process name
     * - PID
     */
    count = 1;
    if (ORTE_SUCCESS != (ret = opal_dss.unpack(mev->buffer, &proc, &count, ORTE_NAME))) {
        ORTE_ERROR_LOG(ret);
        goto cleanup;
    }
    count = 1;
    if (ORTE_SUCCESS != (ret = opal_dss.unpack(mev->buffer, &proc_pid, &count, OPAL_PID))) {
        ORTE_ERROR_LOG(ret);
        goto cleanup;
    }

    if( NULL == (vpid_snapshot = find_vpid_snapshot(&proc)) ) {
        ORTE_ERROR_LOG(ORTE_ERR_NOT_FOUND);
        goto cleanup;
    }

    /* JJH: Maybe we should save the old and the newly restarted pid? */
    vpid_snapshot->process_pid = proc_pid;
    
 cleanup:
    /* release the message event */
    OBJ_RELEASE(mev);
    return;
}

static void snapc_full_process_hnp_request_cmd(int fd, short event, void *cbdata)
{
    int ret;
    orte_message_event_t *mev = (orte_message_event_t*)cbdata;
    orte_process_name_t *sender = NULL;
    orte_snapc_full_cmd_flag_t command;
    orte_std_cntr_t count;

    sender = &(mev->sender);

    count = 1;
    if (ORTE_SUCCESS != (ret = opal_dss.unpack(mev->buffer, &command, &count, ORTE_SNAPC_FULL_CMD))) {
        ORTE_ERROR_LOG(ret);
        goto cleanup;
    }
    
    switch (command) {
        case ORTE_SNAPC_FULL_UPDATE_JOB_STATE_QUICK_CMD:
            OPAL_OUTPUT_VERBOSE((10, mca_snapc_full_component.super.output_handle,
                                 "Local) Command: Update Job state command (quick)"));

            snapc_full_local_process_job_update_cmd(sender, mev->buffer, true);
            break;

        case ORTE_SNAPC_FULL_UPDATE_JOB_STATE_CMD:
            OPAL_OUTPUT_VERBOSE((10, mca_snapc_full_component.super.output_handle,
                                 "Local) Command: Update Job state command"));

            snapc_full_local_process_job_update_cmd(sender, mev->buffer, false);
            break;

        default:
            ORTE_ERROR_LOG(ORTE_ERR_VALUE_OUT_OF_BOUNDS);
    }

 cleanup:
    /* release the message event */
    OBJ_RELEASE(mev);
    return;
}

static void snapc_full_local_process_job_update_cmd(orte_process_name_t* sender,
                                                    opal_buffer_t* buffer,
                                                    bool quick)
{
    int ret, exit_status = ORTE_SUCCESS;
    orte_jobid_t jobid;
    int job_ckpt_state;
    char *job_ckpt_ref = NULL;
    char *job_ckpt_loc = NULL;
    orte_std_cntr_t count;

    /*
     * Unpack the data
     * - jobid
     * - ckpt_state
     * - ckpt_reference
     * - ckpt_location
     */
    count = 1;
    if (ORTE_SUCCESS != (ret = opal_dss.unpack(buffer, &jobid, &count, ORTE_JOBID))) {
        ORTE_ERROR_LOG(ret);
        exit_status = ret;
        goto cleanup;
    }

    count = 1;
    if (ORTE_SUCCESS != (ret = opal_dss.unpack(buffer, &job_ckpt_state, &count, OPAL_INT))) {
        ORTE_ERROR_LOG(ret);
        exit_status = ret;
        goto cleanup;
    }

    if( !quick ) {
        count = 1;
        if (ORTE_SUCCESS != (ret = opal_dss.unpack(buffer, &job_ckpt_ref, &count, OPAL_STRING))) {
            ORTE_ERROR_LOG(ret);
            exit_status = ret;
            goto cleanup;
        }

        count = 1;
        if (ORTE_SUCCESS != (ret = opal_dss.unpack(buffer, &job_ckpt_loc, &count, OPAL_STRING))) {
            ORTE_ERROR_LOG(ret);
            exit_status = ret;
            goto cleanup;
        }
    }

    if( ORTE_SUCCESS != (ret = local_coord_job_state_update(jobid, job_ckpt_state, &job_ckpt_ref, &job_ckpt_loc)) ) {
        ORTE_ERROR_LOG(ret);
        exit_status = ret;
        goto cleanup;
    }

 cleanup:
    return;
}


int local_coord_job_state_update(orte_jobid_t jobid,
                                 int    job_ckpt_state,
                                 char **job_ckpt_ref,
                                 char **job_ckpt_loc)
{
    int ret, exit_status = ORTE_SUCCESS;
    orte_snapc_full_app_snapshot_t *vpid_snapshot = NULL;
    opal_list_item_t* item = NULL;
    char * state_str = NULL;

    OPAL_OUTPUT_VERBOSE((20, mca_snapc_full_component.super.output_handle,
                         "Local) Job %s: Changed to state to:\n",
                         ORTE_JOBID_PRINT(jobid)));
    orte_snapc_ckpt_state_str(&state_str, job_ckpt_state);
    OPAL_OUTPUT_VERBOSE((20, mca_snapc_full_component.super.output_handle,
                         "Local)    Job State:        %d (%s)\n",
                         (int)job_ckpt_state, state_str ));
    free(state_str);
    state_str = NULL;

    if( NULL != *job_ckpt_ref ) {
        OPAL_OUTPUT_VERBOSE((20, mca_snapc_full_component.super.output_handle,
                             "Local)    Snapshot Ref:    (%s)\n",
                             *job_ckpt_ref));
    }
    if( NULL != *job_ckpt_loc ) {
        OPAL_OUTPUT_VERBOSE((20, mca_snapc_full_component.super.output_handle,
                             "Local)    Remote Location: (%s)\n",
                             *job_ckpt_loc));
    }

    /*
     * Update the vpid structure if we need to.
     * Really only need to if we don't have valid information (PID) 
     * for the application.
     */
    if( ORTE_SUCCESS != (ret = snapc_full_local_get_vpids() ) ) {
        ORTE_ERROR_LOG(ret);
        exit_status = ret;
        goto cleanup;
    }

    current_job_ckpt_state = job_ckpt_state;

    /*
     * If we have been asked to checkpoint do so
     */
    if( ORTE_SNAPC_CKPT_STATE_PENDING      == job_ckpt_state ||
        ORTE_SNAPC_CKPT_STATE_PENDING_TERM == job_ckpt_state ) {
        /*
         * For each of the processes we are tasked with, start their checkpoints
         */
        for(item  = opal_list_get_first(&snapc_local_vpids);
            item != opal_list_get_end(&snapc_local_vpids);
            item  = opal_list_get_next(item) ) {
            vpid_snapshot = (orte_snapc_full_app_snapshot_t*)item;

            vpid_snapshot->super.state = job_ckpt_state;

            if( ORTE_SNAPC_CKPT_STATE_PENDING_TERM == job_ckpt_state ) {
                vpid_snapshot->term = true;
            }
            else {
                vpid_snapshot->term = false;
            }

            /*
             * Update it's local information
             */
            if( NULL != vpid_snapshot->super.reference_name ) { 
                free(vpid_snapshot->super.reference_name);
                vpid_snapshot->super.reference_name = NULL;
            }
            vpid_snapshot->super.reference_name = opal_crs_base_unique_snapshot_name(vpid_snapshot->super.process_name.vpid);
            
            /* global_directory/local_snapshot_vpid/... */
            if( NULL != vpid_snapshot->super.local_location ) {
                free(vpid_snapshot->super.local_location);
                vpid_snapshot->super.local_location = NULL;
            }

            if( orte_snapc_base_store_in_place ) {
                asprintf(&(vpid_snapshot->super.local_location), 
                         "%s/%s", 
                         *job_ckpt_loc,
                         vpid_snapshot->super.reference_name);
            }
            else {
                /* Use the OPAL CRS base snapshot dir
                 * JJH: Do we want to do something more interesting?
                 */
                asprintf(&(vpid_snapshot->super.local_location), 
                         "%s/%s",
                         opal_crs_base_snapshot_dir,
                         vpid_snapshot->super.reference_name);
            }
            
            if( NULL != vpid_snapshot->super.remote_location ) {
                free(vpid_snapshot->super.remote_location);
                vpid_snapshot->super.remote_location = NULL;
            }

            asprintf(&(vpid_snapshot->super.remote_location), 
                     "%s/%s", 
                     *job_ckpt_loc,
                     vpid_snapshot->super.reference_name);

        }

        /*
         * Start checkpointing all local processes
         */
        if( ORTE_SUCCESS != (ret = snapc_full_local_start_checkpoint_all(job_ckpt_state) ) ) {
            ORTE_ERROR_LOG(ret);
            exit_status = ret;
            goto cleanup;
        }
    }
    /*
     * Release all checkpointed processes now that the checkpoint is complete
     * If the request was to checkpoint then terminate this command will tell
     * the application to do so upon release.
     */
    else if( ORTE_SNAPC_CKPT_STATE_FINISHED_LOCAL  == job_ckpt_state ) {
        if( ORTE_SUCCESS != (ret = local_coord_job_state_update_finished_local() ) ) {
            ORTE_ERROR_LOG(ORTE_ERROR);
            exit_status = ORTE_ERROR;
            goto cleanup;
        }
    }
    /*
     * Once we get the FINISHED state then the checkpoint is all done, and we
     * reset our state to NONE.
     */
    else if( ORTE_SNAPC_CKPT_STATE_FINISHED  == job_ckpt_state ) {
        OPAL_OUTPUT_VERBOSE((15, mca_snapc_full_component.super.output_handle,
                             "Local) Job Ckpt finished - Cleanup\n"));

        for(item  = opal_list_get_first(&snapc_local_vpids);
            item != opal_list_get_end(&snapc_local_vpids);
            item  = opal_list_get_next(item) ) {
            vpid_snapshot = (orte_snapc_full_app_snapshot_t*)item;

            /* Forgot to close the pipes to the application
             * This can happen if we never received a FINISHED_LOCAL, but only
             * a FINISHED
             */
            if( vpid_snapshot->comm_pipe_w_fd > 0  ) {
                if( ORTE_SUCCESS != (ret = local_coord_job_state_update_finished_local() ) ) {
                    ORTE_ERROR_LOG(ORTE_ERROR);
                    exit_status = ORTE_ERROR;
                    goto cleanup;
                }
            }

            vpid_snapshot->super.state = ORTE_SNAPC_CKPT_STATE_NONE;
        }
    }
    /*
     * States not handled
     */
    else if( ORTE_SNAPC_CKPT_STATE_FILE_XFER == job_ckpt_state ) {
        ;
    }
    else {
        ;
    }
    
 cleanup:
    if( NULL != state_str ) {
        free(state_str);
        state_str = NULL;
    }

    return exit_status;
}

static int local_coord_job_state_update_finished_local(void)
{
    int ret;
    orte_snapc_full_app_snapshot_t *vpid_snapshot = NULL;
    opal_list_item_t* item = NULL;

    OPAL_OUTPUT_VERBOSE((15, mca_snapc_full_component.super.output_handle,
                         "Local) Job Ckpt finished tell all processes\n"));

    for(item  = opal_list_get_first(&snapc_local_vpids);
        item != opal_list_get_end(&snapc_local_vpids);
        item  = opal_list_get_next(item) ) {
        vpid_snapshot = (orte_snapc_full_app_snapshot_t*)item;

        OPAL_OUTPUT_VERBOSE((15, mca_snapc_full_component.super.output_handle,
                             "Local)    Tell process %s\n",
                             ORTE_NAME_PRINT(&vpid_snapshot->super.process_name)));

        if( ORTE_SUCCESS != (ret = snapc_full_local_end_ckpt_handshake(vpid_snapshot) ) ) {
            opal_output(mca_snapc_full_component.super.output_handle,
                        "Local) Error: Unable to finish the handshake with peer %s. %d\n", 
                        ORTE_NAME_PRINT(&vpid_snapshot->super.process_name), ret);
            ORTE_ERROR_LOG(ORTE_ERROR);
            return ORTE_ERROR;
        }
    }

    return ORTE_SUCCESS;
}

/************************
 * Start the checkpoint
 ************************/
static int snapc_full_local_start_checkpoint_all(int ckpt_state)
{
    int ret, exit_status = ORTE_SUCCESS;
    orte_snapc_full_app_snapshot_t *vpid_snapshot;
    opal_list_item_t* item = NULL;
    char * actual_local_dir = NULL;
    bool ckpt_n_term = false;
    char *tmp_pid = NULL;

    /*
     * Cannot let opal-checkpoint be passed the --term flag
     * since the HNP needs to talk to the app to get
     * information for FileM. HNP will issue the termination.
     * JJH: Eventually release the contraint that the app needs to 
     *      be alive for FileM to properly work.
     *      However if we are storing in place, then we don't use
     *      the FileM framework and can terminate the application
     *      from this command.
     */
    if ( !orte_snapc_base_store_in_place ) {
        ckpt_n_term = false;
    }
    else if( ORTE_SNAPC_CKPT_STATE_PENDING_TERM == ckpt_state ) {
        ckpt_n_term = true;
    }
    else {
        ckpt_n_term = false;
    }

    /*
     * Pass 1: Setup snapshot directory
     */
    for(item  = opal_list_get_first(&snapc_local_vpids);
        item != opal_list_get_end(&snapc_local_vpids);
        item  = opal_list_get_next(item) ) {
        vpid_snapshot = (orte_snapc_full_app_snapshot_t*)item;

        /*
         * Set up the snapshot directory per suggestion from 
         * the Global Snapshot Coordinator
         * If we can't create the suggested local directory, do what we can and update
         * local directory reference in the GPR
         */
        if( ORTE_SUCCESS != (ret = snapc_full_local_setup_snapshot_dir(vpid_snapshot->super.reference_name,
                                                                       vpid_snapshot->super.local_location,
                                                                       &actual_local_dir) ) ) {
            ORTE_ERROR_LOG(ret);
            exit_status = ret;
            goto cleanup;
        }

        OPAL_OUTPUT_VERBOSE((20, mca_snapc_full_component.super.output_handle,
                             "Local) Using directory (%s)\n",
                             vpid_snapshot->super.local_location));

        /* Dummy check */
        if( vpid_snapshot->process_pid == 0 ) {
            ret = snapc_full_local_get_vpids();
            if( ORTE_SUCCESS != ret || vpid_snapshot->process_pid == 0 ) {
                opal_output( mca_snapc_full_component.super.output_handle,
                             "local) Cannot checkpoint an invalid pid (%d)\n", 
                             vpid_snapshot->process_pid);
                ORTE_ERROR_LOG(ORTE_ERROR);
                exit_status = ORTE_ERROR;
                goto cleanup;
            }
        }
    }

    /*
     * Pass 2: Start process of opening communication channels
     */
    for(item  = opal_list_get_first(&snapc_local_vpids);
        item != opal_list_get_end(&snapc_local_vpids);
        item  = opal_list_get_next(item) ) {
        vpid_snapshot = (orte_snapc_full_app_snapshot_t*)item;

        /*
         * Create named pipe references for this process
         */
        if( NULL == vpid_snapshot->comm_pipe_w ||
            NULL == vpid_snapshot->comm_pipe_r ) {
            if( NULL != tmp_pid ) {
                free(tmp_pid);
                tmp_pid = NULL;
            }
            asprintf(&tmp_pid, "%d", vpid_snapshot->process_pid);
            asprintf(&(vpid_snapshot->comm_pipe_w), "%s/%s.%s", opal_cr_pipe_dir, OPAL_CR_NAMED_PROG_R, tmp_pid);
            asprintf(&(vpid_snapshot->comm_pipe_r), "%s/%s.%s", opal_cr_pipe_dir, OPAL_CR_NAMED_PROG_W, tmp_pid);
        }

        OPAL_OUTPUT_VERBOSE((20, mca_snapc_full_component.super.output_handle,
                             "Local) Signal process (%d) with signal %d\n",
                             (int) vpid_snapshot->process_pid,
                             opal_cr_entry_point_signal));

        /*
         * Signal the application
         */
        if( 0 != (ret = kill(vpid_snapshot->process_pid, opal_cr_entry_point_signal) ) ) {
            opal_output(mca_snapc_full_component.super.output_handle,
                        "local) Error: Failed to signal process %d with signal %d. %d\n", 
                        (int) vpid_snapshot->process_pid,
                        opal_cr_entry_point_signal,
                        ret);
            exit_status = ret;
            goto cleanup;
        }
    }

    /*
     * Pass 3: Wait for channels to open up
     */
    for(item  = opal_list_get_first(&snapc_local_vpids);
        item != opal_list_get_end(&snapc_local_vpids);
        item  = opal_list_get_next(item) ) {
        vpid_snapshot = (orte_snapc_full_app_snapshot_t*)item;

        if( ORTE_SUCCESS != (ret = snapc_full_local_start_ckpt_open_comm(vpid_snapshot) ) ) {
            opal_output(mca_snapc_full_component.super.output_handle,
                        "local) Error: Unable to initiate the handshake with peer %s. %d\n", 
                        ORTE_NAME_PRINT(&vpid_snapshot->super.process_name), ret);
            ORTE_ERROR_LOG(ORTE_ERROR);
            exit_status = ORTE_ERROR;
            goto cleanup;
        }
        vpid_snapshot->super.state = ORTE_SNAPC_CKPT_STATE_RUNNING;
    }

    /*
     * Progress Update to Global Coordinator
     */
    if( ORTE_SUCCESS != (ret = snapc_full_local_update_coord(ORTE_SNAPC_CKPT_STATE_RUNNING, true) ) ) {
        ORTE_ERROR_LOG(ret);
        exit_status = ret;
        goto cleanup;
    }

    /*
     * Pass 3: Start Handshake, send term argument
     */
    for(item  = opal_list_get_first(&snapc_local_vpids);
        item != opal_list_get_end(&snapc_local_vpids);
        item  = opal_list_get_next(item) ) {
        vpid_snapshot = (orte_snapc_full_app_snapshot_t*)item;

        if( ORTE_SUCCESS != (ret = snapc_full_local_start_ckpt_handshake_term(vpid_snapshot, ckpt_n_term) ) ) {
            opal_output(mca_snapc_full_component.super.output_handle,
                        "local) Error: Unable to initiate the handshake with peer %s. %d\n", 
                        ORTE_NAME_PRINT(&vpid_snapshot->super.process_name), ret);
            ORTE_ERROR_LOG(ORTE_ERROR);
            exit_status = ORTE_ERROR;
            goto cleanup;
        }
    }

    /*
     * Pass 4: Start Handshake, send snapshot reference/location arguments
     */
    for(item  = opal_list_get_first(&snapc_local_vpids);
        item != opal_list_get_end(&snapc_local_vpids);
        item  = opal_list_get_next(item) ) {
        vpid_snapshot = (orte_snapc_full_app_snapshot_t*)item;

        if( ORTE_SUCCESS != (ret = snapc_full_local_start_ckpt_handshake(vpid_snapshot) ) ) {
            opal_output(mca_snapc_full_component.super.output_handle,
                        "local) Error: Unable to initiate the handshake with peer %s. %d\n", 
                        ORTE_NAME_PRINT(&vpid_snapshot->super.process_name), ret);
            ORTE_ERROR_LOG(ORTE_ERROR);
            exit_status = ORTE_ERROR;
            goto cleanup;
        }
    }

 cleanup:
    if( NULL != tmp_pid ) {
        free(tmp_pid);
        tmp_pid = NULL;
    }

    if( ORTE_SUCCESS != exit_status ) {
        ckpt_state = ORTE_SNAPC_CKPT_STATE_ERROR;
    }

    return exit_status;
}

static int snapc_full_local_update_coord(int state, bool quick)
{
    int ret, exit_status = ORTE_SUCCESS;
    opal_buffer_t buffer;
    orte_snapc_full_cmd_flag_t command;
    opal_list_item_t* item = NULL;
    orte_snapc_full_app_snapshot_t *vpid_snapshot = NULL;
    char *crs_agent = NULL;
    size_t sz = 0;
    char *loc_location = NULL;

    /*
     * Local Coordinator: Send Global Coordinator state information
     */
    OBJ_CONSTRUCT(&buffer, opal_buffer_t);

    if( quick ) {
        command = ORTE_SNAPC_FULL_UPDATE_ORTED_STATE_QUICK_CMD;
    } else {
        command = ORTE_SNAPC_FULL_UPDATE_ORTED_STATE_CMD;
    }
    if (ORTE_SUCCESS != (ret = opal_dss.pack(&buffer, &command, 1, ORTE_SNAPC_FULL_CMD )) ) {
        ORTE_ERROR_LOG(ret);
        exit_status = ret;
        goto cleanup;
    }

    if (ORTE_SUCCESS != (ret = opal_dss.pack(&buffer, &state, 1, OPAL_INT))) {
        ORTE_ERROR_LOG(ret);
        exit_status = ret;
        goto cleanup;
    }

    /* Optionally send only an abbreviated message to improve scalability */
    if( quick ) {
        goto send_data;
    }

    crs_agent = strdup(opal_crs_base_selected_component.base_version.mca_component_name);
    if (ORTE_SUCCESS != (ret = opal_dss.pack(&buffer, &(crs_agent), 1, OPAL_STRING))) {
        ORTE_ERROR_LOG(ret);
        exit_status = ret;
        goto cleanup;
    }

    sz = opal_list_get_size(&snapc_local_vpids);
    if (ORTE_SUCCESS != (ret = opal_dss.pack(&buffer, &sz, 1, OPAL_SIZE))) {
        ORTE_ERROR_LOG(ret);
        exit_status = ret;
        goto cleanup;
    }

    for(item  = opal_list_get_first(&snapc_local_vpids);
        item != opal_list_get_end(&snapc_local_vpids);
        item  = opal_list_get_next(item) ) {
        vpid_snapshot = (orte_snapc_full_app_snapshot_t*)item;

        if (ORTE_SUCCESS != (ret = opal_dss.pack(&buffer, &(vpid_snapshot->super.process_name), 1, ORTE_NAME))) {
            ORTE_ERROR_LOG(ret);
            exit_status = ret;
            goto cleanup;
        }

        if (ORTE_SUCCESS != (ret = opal_dss.pack(&buffer, &(vpid_snapshot->super.reference_name), 1, OPAL_STRING))) {
            ORTE_ERROR_LOG(ret);
            exit_status = ret;
            goto cleanup;
        }

        loc_location = opal_dirname(vpid_snapshot->super.local_location);
        if (ORTE_SUCCESS != (ret = opal_dss.pack(&buffer, &(loc_location), 1, OPAL_STRING))) {
            ORTE_ERROR_LOG(ret);
            exit_status = ret;
            goto cleanup;
        }

    }

 send_data:
    if (0 > (ret = orte_rml.send_buffer(ORTE_PROC_MY_HNP, &buffer, ORTE_RML_TAG_SNAPC_FULL, 0))) {
        ORTE_ERROR_LOG(ret);
        exit_status = ret;
        goto cleanup;
    }

 cleanup:
    OBJ_DESTRUCT(&buffer);

    if( NULL != crs_agent ) {
        free(crs_agent);
        crs_agent = NULL;
    }
    if( NULL != loc_location ) {
        free(loc_location);
        loc_location = NULL;
    }

    return exit_status;
}

static int snapc_full_local_start_ckpt_open_comm(orte_snapc_full_app_snapshot_t *vpid_snapshot)
{
    int ret, exit_status = ORTE_SUCCESS;
    int usleep_time = 1000;
    int s_time = 0, max_wait_time;

    max_wait_time = 20 * (1000000/usleep_time); /* wait time before giving up on the checkpoint */

    /*
     * Wait for the named pipes to be created
     */
    OPAL_OUTPUT_VERBOSE((10, mca_snapc_full_component.super.output_handle,
                         "Local) Waiting for process %s's pipes (%s) (%s)\n",
                         ORTE_NAME_PRINT(&vpid_snapshot->super.process_name),
                         vpid_snapshot->comm_pipe_w,
                         vpid_snapshot->comm_pipe_r));
    for( s_time = 0; s_time < max_wait_time; ++s_time) {
        /*
         * See if the named pipe exists yet for the PID in question
         */
        if( 0 > (ret = access(vpid_snapshot->comm_pipe_r, F_OK) )) {
            /* File doesn't exist yet, keep waiting */
            if( s_time >= max_wait_time - 5 ) {
                OPAL_OUTPUT_VERBOSE((15, mca_snapc_full_component.super.output_handle,
                                     "Local) File does not exist yet: <%s> rtn = %d (waited %d/%d usec)\n",
                                     vpid_snapshot->comm_pipe_r, ret, s_time, max_wait_time));
            }
            usleep(usleep_time);
            opal_event_loop(OPAL_EVLOOP_NONBLOCK);
            continue;
        }
        else if( 0 > (ret = access(vpid_snapshot->comm_pipe_w, F_OK) )) {
            /* File doesn't exist yet, keep waiting */
            if( s_time >= max_wait_time - 5 ) {
                OPAL_OUTPUT_VERBOSE((15, mca_snapc_full_component.super.output_handle,
                                     "Local) File does not exist yet: <%s> rtn = %d (waited %d/%d usec)\n",
                                     vpid_snapshot->comm_pipe_w, ret, s_time, max_wait_time));
            }
            usleep(usleep_time);
            opal_event_loop(OPAL_EVLOOP_NONBLOCK);
            continue;
        }
        else {
            break;
        }
    }
    if( s_time == max_wait_time ) { 
        /* The file doesn't exist, 
         * This means that the process didn't open up a named pipe for us
         * to access their checkpoint notification routine. Therefore,
         * the application either:
         *  - Doesn't exist
         *  - Isn't checkpointable
         * In either case there is nothing we can do.
         */
        orte_show_help("help-opal-checkpoint.txt", "pid_does_not_exist", true,
                       vpid_snapshot->process_pid,
                       vpid_snapshot->comm_pipe_r,
                       vpid_snapshot->comm_pipe_w);

        exit_status = OPAL_ERROR;
        goto cleanup;
    }

    /*
     * Open Pipes...
     *  - prog_named_write_pipe:
     *    prog makes this file and opens Read Only
     *    this app. opens it Write Only
     *  - prog_named_read_pipe:
     *    prog makes this file and opens Write Only
     *    this app. opens it Read Only
     */
    vpid_snapshot->comm_pipe_w_fd = open(vpid_snapshot->comm_pipe_w, O_WRONLY);
    if(vpid_snapshot->comm_pipe_w_fd < 0) {
        opal_output(mca_snapc_full_component.super.output_handle,
                    "local) Error: Unable to open name pipe (%s). %d\n", 
                    vpid_snapshot->comm_pipe_w, vpid_snapshot->comm_pipe_w_fd);
        exit_status = OPAL_ERROR;
        goto cleanup;
    }

    vpid_snapshot->comm_pipe_r_fd = open(vpid_snapshot->comm_pipe_r, O_RDWR);
    if(vpid_snapshot->comm_pipe_r_fd < 0) {
        opal_output(mca_snapc_full_component.super.output_handle,
                    "local) Error: Unable to open name pipe (%s). %d\n", 
                    vpid_snapshot->comm_pipe_r, vpid_snapshot->comm_pipe_r_fd);
        exit_status = OPAL_ERROR;
        goto cleanup;
    }

 cleanup:
    return exit_status;
}

static int snapc_full_local_start_ckpt_handshake_term(orte_snapc_full_app_snapshot_t *vpid_snapshot, bool term)
{
    int ret, exit_status = ORTE_SUCCESS;
    int term_rep;

    /*
     * Start the handshake: Send term argument
     */
    term_rep = (int)term;

    if( term ) {
        OPAL_OUTPUT_VERBOSE((10, mca_snapc_full_component.super.output_handle,
                             "Local) Tell app to TERMINATE after completion of checkpoint. [%s (%d)]\n",
                             (term ? "True" : "False"), term_rep));
    }

    if( sizeof(int) != (ret = write(vpid_snapshot->comm_pipe_w_fd, &term_rep, sizeof(int))) ) {
        opal_output(mca_snapc_full_component.super.output_handle,
                    "local) Error: Unable to write term (%d) to named pipe (%s), %d\n", 
                    term, vpid_snapshot->comm_pipe_w, ret);
        exit_status = OPAL_ERROR;
        goto cleanup;
    }

 cleanup:
    return exit_status;
}

static int snapc_full_local_start_ckpt_handshake(orte_snapc_full_app_snapshot_t *vpid_snapshot)
{
    int ret, exit_status = ORTE_SUCCESS;
    char *local_dir = NULL;
    int len, value;
    ssize_t tmp_size = 0;

    /*
     * Wait for the appliation to respond
     */
    if( sizeof(int) != (ret = read(vpid_snapshot->comm_pipe_r_fd, &value, sizeof(int))) ) {
        opal_output(mca_snapc_full_component.super.output_handle,
                    "local) Error: Unable to read length from named pipe (%s). %d\n", 
                    vpid_snapshot->comm_pipe_r, ret);
        exit_status = OPAL_ERROR;
        goto cleanup;
    }

    /* Check the response to make sure we can checkpoint this process */
    if( OPAL_CHECKPOINT_CMD_IN_PROGRESS == value ) {
        orte_show_help("help-opal-checkpoint.txt",
                       "ckpt:in_progress", 
                       true,
                       vpid_snapshot->process_pid);
        exit_status = OPAL_ERROR;
        goto cleanup;
    }
    else if( OPAL_CHECKPOINT_CMD_NULL == value ) {
        orte_show_help("help-opal-checkpoint.txt",
                       "ckpt:req_null", 
                       true,
                       vpid_snapshot->process_pid);
        exit_status = OPAL_ERROR;
        goto cleanup;
    }
    else if ( OPAL_CHECKPOINT_CMD_ERROR == value ) {
        orte_show_help("help-opal-checkpoint.txt",
                       "ckpt:req_error", 
                       true,
                       vpid_snapshot->process_pid);
        exit_status = OPAL_ERROR;
        goto cleanup;
    }

    opal_event_set(&(vpid_snapshot->comm_pipe_r_eh),
                   vpid_snapshot->comm_pipe_r_fd,
                   OPAL_EV_READ|OPAL_EV_PERSIST,
                   snapc_full_local_comm_read_event,
                   vpid_snapshot);
    vpid_snapshot->is_eh_active = true;
    opal_event_add(&(vpid_snapshot->comm_pipe_r_eh), NULL);

    /*
     * Send: Snapshot Name
     */
    len = strlen(vpid_snapshot->super.reference_name) + 1;
    if( sizeof(int) != (ret = write(vpid_snapshot->comm_pipe_w_fd, &len, sizeof(int))) ) {
        opal_output(mca_snapc_full_component.super.output_handle,
                    "local) Error: Unable to write snapshot name len (%d) to named pipe (%s). %d\n", 
                    len, vpid_snapshot->comm_pipe_w, ret);
        exit_status = OPAL_ERROR;
        goto cleanup;
    }

    tmp_size = sizeof(char) * len;
    if( tmp_size != (ret = write(vpid_snapshot->comm_pipe_w_fd, (vpid_snapshot->super.reference_name), (sizeof(char) * len))) ) {
        opal_output(mca_snapc_full_component.super.output_handle,
                    "local) Error: Unable to write snapshot name (%s) to named pipe (%s). %d\n", 
                    vpid_snapshot->super.reference_name, vpid_snapshot->comm_pipe_w, ret);
        exit_status = OPAL_ERROR;
        goto cleanup;
    }

    /*
     * Send: Snapshot Location
     */
    local_dir = strdup(vpid_snapshot->super.local_location);
    local_dir = opal_dirname(local_dir);
    len = strlen(local_dir) + 1;
    if( sizeof(int) != (ret = write(vpid_snapshot->comm_pipe_w_fd, &len, sizeof(int))) ) {
        opal_output(mca_snapc_full_component.super.output_handle,
                    "local) Error: Unable to write snapshot location len (%d) to named pipe (%s). %d\n", 
                    len, vpid_snapshot->comm_pipe_w, ret);
        exit_status = OPAL_ERROR;
        goto cleanup;
    }

    tmp_size = sizeof(char) * len;
    if( tmp_size != (ret = write(vpid_snapshot->comm_pipe_w_fd, (local_dir), (sizeof(char) * len))) ) {
        opal_output(mca_snapc_full_component.super.output_handle,
                    "local) Error: Unable to write snapshot location (%s) to named pipe (%s). %d\n", 
                    local_dir, vpid_snapshot->comm_pipe_w, ret);
        exit_status = OPAL_ERROR;
        goto cleanup;
    }

 cleanup:
    if( NULL != local_dir ) {
        free(local_dir);
        local_dir = NULL;
    }

    return exit_status;
}

static int snapc_full_local_end_ckpt_handshake(orte_snapc_full_app_snapshot_t *vpid_snapshot)
{
    int ret, exit_status = ORTE_SUCCESS;
    int last_cmd = 0;

    /*
     * Make sure the pipe is open, so we do not try to do this twice
     */
    if( 0 > vpid_snapshot->comm_pipe_w_fd ) {
        return exit_status;
    }

    if( vpid_snapshot->term ) {
        last_cmd = 999;
    } else {
        last_cmd = 0;
    }

    /*
     * Finish the handshake.
     */
    if( sizeof(int) != (ret = write(vpid_snapshot->comm_pipe_w_fd, &last_cmd, sizeof(int))) ) {
        opal_output(mca_snapc_full_component.super.output_handle,
                    "local) Error: Unable to release process %s (%d)\n", 
                    ORTE_NAME_PRINT(&vpid_snapshot->super.process_name), ret);
        exit_status = OPAL_ERROR;
        goto cleanup;
    }

 cleanup:
    /*
     * Close all pipes
     */
    close(vpid_snapshot->comm_pipe_w_fd);
    close(vpid_snapshot->comm_pipe_r_fd);
    vpid_snapshot->comm_pipe_w_fd = -1;
    vpid_snapshot->comm_pipe_r_fd = -1;

    return exit_status;
}

static void snapc_full_local_comm_read_event(int fd, short flags, void *arg)
{
    int ret, exit_status = ORTE_SUCCESS;
    orte_snapc_full_app_snapshot_t *vpid_snapshot = NULL;
    int ckpt_state;
    int loc_min_state;
    bool send_quick = true;
    char * state_str = NULL;

    vpid_snapshot = (orte_snapc_full_app_snapshot_t *)arg;

    OPAL_OUTPUT_VERBOSE((10, mca_snapc_full_component.super.output_handle,
                         "Local) Read Event: Process %s done checkpointing...\n",
                         ORTE_NAME_PRINT(&vpid_snapshot->super.process_name)));

    /*
     * Get the final state of the checkpoint from the checkpointing process
     */
    if( sizeof(int) != (ret = read(vpid_snapshot->comm_pipe_r_fd, &ckpt_state, sizeof(int))) ) {
        opal_output(mca_snapc_full_component.super.output_handle,
                    "local) Error: Unable to read state from named pipe (%s). %d\n",
                    vpid_snapshot->comm_pipe_r, ret);
        ORTE_ERROR_LOG(ORTE_ERROR);
        exit_status = ORTE_ERROR;
        goto cleanup;
    }

    /*
     * Now that the checkpoint is finished
     * Update our status information
     */
    if( ckpt_state == OPAL_CRS_ERROR ) {
        vpid_snapshot->super.state = ORTE_SNAPC_CKPT_STATE_ERROR;
    } else {
        vpid_snapshot->super.state = ORTE_SNAPC_CKPT_STATE_FINISHED_LOCAL;
    }

    /*
     * If error, then exit early
     */
    if( ORTE_SNAPC_CKPT_STATE_ERROR == vpid_snapshot->super.state ) {
        /* JJH - The error path needs some more work */
        if( ORTE_SUCCESS != (ret = snapc_full_local_update_coord(ORTE_SNAPC_CKPT_STATE_ERROR, true) ) ) {
            ORTE_ERROR_LOG(ret);
            exit_status = ret;
            goto cleanup;
        }
        goto cleanup;
    }

    /*
     * If all processes have finished locally, notify Global Coordinator
     * if(FIN_LOCAL) {
     *     -- wait for the FIN from Global Coord --
     * }
     */
    loc_min_state = snapc_full_get_min_state();
    if( loc_min_state > current_job_ckpt_state &&
        ORTE_SNAPC_CKPT_STATE_FINISHED_LOCAL == loc_min_state ) {

        orte_snapc_ckpt_state_str(&state_str, loc_min_state);
        OPAL_OUTPUT_VERBOSE((10, mca_snapc_full_component.super.output_handle,
                             "Local) Daemon State Changed: %d (%s)",
                             (int)loc_min_state, state_str ));
        free(state_str);
        state_str = NULL;

        send_quick = false;

        current_job_ckpt_state = loc_min_state;
        if( ORTE_SUCCESS != (ret = snapc_full_local_update_coord(loc_min_state, send_quick) ) ) {
            ORTE_ERROR_LOG(ret);
            exit_status = ret;
            goto cleanup;
        }
    }

    /*
     * If file transfer required just set state
     * Global Coordinator does not need to be notified again since it can see
     * these variables and knows what to do.
     */
    if( !orte_snapc_base_store_in_place && !orte_snapc_full_skip_filem ) {
        vpid_snapshot->super.state = ORTE_SNAPC_CKPT_STATE_FILE_XFER;
    }

 cleanup:
    /*
     * Disable events
     */
    opal_event_del(&(vpid_snapshot->comm_pipe_r_eh));
    vpid_snapshot->is_eh_active = false;

    if( NULL != state_str ) {
        free(state_str);
        state_str = NULL;
    }

    return;
}

static int snapc_full_get_min_state(void)
{
    int min_state = ORTE_SNAPC_CKPT_MAX;
    orte_snapc_full_app_snapshot_t *vpid_snapshot = NULL;
    opal_list_item_t* item = NULL;
    char * state_str_a = NULL;
    char * state_str_b = NULL;

    for(item  = opal_list_get_first(&snapc_local_vpids);
        item != opal_list_get_end(&snapc_local_vpids);
        item  = opal_list_get_next(item) ) {
        vpid_snapshot = (orte_snapc_full_app_snapshot_t*)item;

        if( NULL != state_str_a ) {
            free(state_str_a);
        }
        if( NULL != state_str_b ) {
            free(state_str_b);
        }

        orte_snapc_ckpt_state_str(&state_str_a, vpid_snapshot->super.state);
        orte_snapc_ckpt_state_str(&state_str_b, min_state);

        OPAL_OUTPUT_VERBOSE((10, mca_snapc_full_component.super.output_handle,
                             "Local) ... Checking [%d %s] vs [%d %s]",
                             (int)vpid_snapshot->super.state, state_str_a,
                             (int)min_state, state_str_b ));
        if( min_state > vpid_snapshot->super.state ) {
            min_state = vpid_snapshot->super.state;
        }
    }

    if( NULL != state_str_b ) {
        free(state_str_b);
        state_str_b = NULL;
    }
    orte_snapc_ckpt_state_str(&state_str_b, min_state);
    OPAL_OUTPUT_VERBOSE((10, mca_snapc_full_component.super.output_handle,
                         "Local) ... Min State [%d %s]",
                         (int)min_state, state_str_b ));

    if( NULL != state_str_a ) {
        free(state_str_a);
        state_str_a = NULL;
    }
    if( NULL != state_str_b ) {
        free(state_str_b);
        state_str_b = NULL;
    }

    return min_state;
}

static int snapc_full_local_setup_snapshot_dir(char * snapshot_ref, char * sugg_dir, char **actual_dir)
{
    int ret, exit_status = ORTE_SUCCESS;
    mode_t my_mode = S_IRWXU;

    /* See if we can use the suggested directory */
    if(OPAL_SUCCESS != (ret = opal_os_dirpath_create(sugg_dir, my_mode) ) ) {
        /* Can't use that directory, try the default directory from OPAL CRS */
        *actual_dir = strdup(opal_crs_base_get_snapshot_directory(snapshot_ref));

        if(OPAL_SUCCESS != (ret = opal_os_dirpath_create(*actual_dir, my_mode) ) ) {
            /* Can't use that either, so let's give up */
            ORTE_ERROR_LOG(ret);
            exit_status = ret;
            goto cleanup;
        }
    }
    else {
        /* We are able to use that directory */
        *actual_dir = strdup(sugg_dir);
    }

 cleanup:
    return exit_status;
}


static int snapc_full_local_get_vpids(void)
{
    opal_list_item_t *item = NULL;
    orte_snapc_full_app_snapshot_t *vpid_snapshot = NULL;
    orte_odls_child_t *child = NULL;
    size_t list_len = 0;

    /*
     * If the list is populated, and has updated pid information then
     * there is nothing to update.
     */
    list_len = opal_list_get_size(&snapc_local_vpids);
    if( list_len > 0 ) {
        vpid_snapshot = (orte_snapc_full_app_snapshot_t*)opal_list_get_first(&snapc_local_vpids);
        if( 0 < vpid_snapshot->process_pid ) {
            return ORTE_SUCCESS;
        }
    }

    /*
     * Otherwise update or populate the list
     */
    for (item = opal_list_get_first(&orte_local_children);
         item != opal_list_get_end(&orte_local_children);
         item = opal_list_get_next(item)) {
        child = (orte_odls_child_t*)item;

        /* if the list is empty or this child is not in the list then add it */
        if( 0    >= list_len ||
            NULL == (vpid_snapshot = find_vpid_snapshot(child->name)) ) {
            vpid_snapshot = OBJ_NEW(orte_snapc_full_app_snapshot_t);
            opal_list_append(&snapc_local_vpids, &(vpid_snapshot->super.super));
        }

        vpid_snapshot->process_pid              = child->pid;
        vpid_snapshot->super.process_name.jobid = child->name->jobid;
        vpid_snapshot->super.process_name.vpid  = child->name->vpid;
    }

    return ORTE_SUCCESS;
}

static orte_snapc_full_app_snapshot_t *find_vpid_snapshot(orte_process_name_t *name )
{
    opal_list_item_t* item = NULL;
    orte_snapc_full_app_snapshot_t *vpid_snapshot = NULL;

    for(item  = opal_list_get_first(&snapc_local_vpids);
        item != opal_list_get_end(&snapc_local_vpids);
        item  = opal_list_get_next(item) ) {
        vpid_snapshot = (orte_snapc_full_app_snapshot_t*)item;

        if( name->jobid == vpid_snapshot->super.process_name.jobid &&
            name->vpid  == vpid_snapshot->super.process_name.vpid ) {
            return vpid_snapshot;
        }
    }

    return NULL;
}


/* -*- C -*-
 * 
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
 *
 */

#include "ompi_config.h"

#include <sys/types.h>
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#include <sys/time.h>
#include <sys/wait.h>
#include <pwd.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "pcm_rsh.h"
#include "include/constants.h"
#include "mca/pcm/pcm.h"
#include "mca/pcm/base/base.h"
#include "mca/pcm/base/base_data_store.h"
#include "mca/pcm/base/base_kill_track.h"
#include "runtime/runtime.h"
#include "runtime/runtime_types.h"
#include "runtime/ompi_rte_wait.h"
#include "event/event.h"
#include "util/output.h"
#include "util/argv.h"
#include "util/numtostr.h"
#include "mca/ns/base/base.h"
#include "util/proc_info.h"
#include "util/show_help.h"
#include "util/if.h"
#include "threads/condition.h"
#include "threads/mutex.h"

/*
 * Internal constants
 */
#define BOOTAGENT "mca_pcm_rsh_bootproxy"
#define PRS_BUFSIZE 1024

struct spawn_procs_data_t {
    struct mca_pcm_base_module_1_0_0_t* me;
    mca_ns_base_jobid_t jobid;
    ompi_list_t *schedlist;
    int ret;
    ompi_mutex_t mutex;
    ompi_condition_t cond;
    volatile bool done;
};
typedef struct spawn_procs_data_t spawn_procs_data_t;


/*
 * Internal functions
 */
static int internal_spawn_proc(mca_pcm_rsh_module_t *me,
                               mca_ns_base_jobid_t jobid, 
                               ompi_rte_node_schedule_t *sched,
                               ompi_list_t *hostlist, 
                               int my_start_vpid, int global_start_vpid,
                               int num_procs);
static void internal_wait_cb(pid_t pid, int status, void *data);
static int internal_start_spawn_procs(struct mca_pcm_base_module_1_0_0_t* me_super, 
                                      mca_ns_base_jobid_t jobid, ompi_list_t *schedlist);
#if OMPI_THREADS_HAVE_DIFFERENT_PIDS
static void spawn_procs_callback(int fd, short flags, void *data);
#endif

int
mca_pcm_rsh_spawn_procs(struct mca_pcm_base_module_1_0_0_t* me_super, 
                        mca_ns_base_jobid_t jobid, ompi_list_t *schedlist)
{
#if OMPI_THREADS_HAVE_DIFFERENT_PIDS
    spawn_procs_data_t data;
    struct timeval tv;
    struct ompi_event ev;

    if (ompi_event_progress_thread()) {
        return internal_start_spawn_procs(me_super, jobid, schedlist);
    }

    data.me = me_super;
    data.jobid = jobid;
    data.schedlist = schedlist;
    data.ret = 0;
    data.done = false;
    OBJ_CONSTRUCT(&(data.mutex), ompi_mutex_t);
    OBJ_CONSTRUCT(&(data.cond), ompi_condition_t);

    OMPI_THREAD_LOCK(&(data.mutex));

    tv.tv_sec = 0;
    tv.tv_usec = 0;

    ompi_evtimer_set(&ev, spawn_procs_callback, &data);
    ompi_evtimer_add(&ev, &tv);

    while (data.done == false) {
        ompi_condition_wait(&(data.cond), &(data.mutex));
    }
    OMPI_THREAD_UNLOCK(&(data.mutex));

    OBJ_DESTRUCT(&(data.mutex));
    OBJ_DESTRUCT(&(data.cond));

    return data.ret;
#else
    return internal_start_spawn_procs(me_super, jobid, schedlist);
#endif
}


#if OMPI_THREADS_HAVE_DIFFERENT_PIDS
static void
spawn_procs_callback(int fd, short flags, void *data)
{
    spawn_procs_data_t *procs_data = (spawn_procs_data_t*) data;

    procs_data->ret = internal_start_spawn_procs(procs_data->me,
                                                 procs_data->jobid,
                                                 procs_data->schedlist);
    procs_data->done = true;
    ompi_condition_signal(&(procs_data->cond));
}
#endif

/*
 * This function just iterates through the schedule list, slicing it
 * up into launchable pieces.  While this infrastructure supports a
 * tree-based or similar launching mechanism, the bootproxy used on
 * the other side of the ssh tunnel does not yet support non-local
 * booting.  So some of this code (just the logic in the looping
 * variable) is perhaps a bit overkill.  The real work is done by
 * internal_spawn_proc, which starts one process via RSH/SSH and then
 * sends it the information passed from this function.
 */
int
internal_start_spawn_procs(struct mca_pcm_base_module_1_0_0_t* me_super, 
                           mca_ns_base_jobid_t jobid, ompi_list_t *schedlist)
{
    mca_pcm_rsh_module_t *me = (mca_pcm_rsh_module_t*) me_super;
    ompi_list_item_t *sched_item, *node_item, *host_item;
    ompi_rte_node_schedule_t *sched;
    ompi_rte_node_allocation_t *node;
    mca_llm_base_hostfile_data_t *data;
    mca_llm_base_hostfile_node_t *host;
    ompi_list_t launch;
    ompi_list_t done;
    int ret, i;
    int width = 1;          /* number of procs to send down the tree
                               at a time.  Currently must be 1.
                               bootproxy must be fixed before
                               this can be changed. */
    int local_offset = 0;
    int global_start_vpid = 0;
    int num_procs = 0;
    int tmp_count;
    unsigned int sleep_time;

    OBJ_CONSTRUCT(&launch, ompi_list_t);
    OBJ_CONSTRUCT(&done, ompi_list_t);

    /* figure out how many procs we have been allocated */
    for (sched_item = ompi_list_get_first(schedlist) ;
         sched_item != ompi_list_get_end(schedlist) ;
         sched_item = ompi_list_get_next(sched_item)) {
        sched = (ompi_rte_node_schedule_t*) sched_item;

        for (node_item = ompi_list_get_first(sched->nodelist) ;
             node_item != ompi_list_get_end(sched->nodelist) ;
             node_item = ompi_list_get_next(node_item)) {
            node = (ompi_rte_node_allocation_t*) node_item;
            if (node->nodes > 0) {
                num_procs += (node->count * node->nodes);
            } else {
                num_procs += node->count;
            }
        }
    }
    
    /* reserve a chunk of vpids */
    local_offset = 0;
    global_start_vpid = (int) ompi_name_server.reserve_range(jobid, num_procs);

    for (sched_item = ompi_list_get_first(schedlist) ;
         sched_item != ompi_list_get_end(schedlist) ;
         sched_item = ompi_list_get_next(sched_item)) {
        sched = (ompi_rte_node_schedule_t*) sched_item;

        for (node_item = ompi_list_get_first(sched->nodelist) ;
             node_item != ompi_list_get_end(sched->nodelist) ;
             node_item = ompi_list_get_next(node_item) ) {

            node = (ompi_rte_node_allocation_t*) node_item;
            data = (mca_llm_base_hostfile_data_t*) node->data;
            host_item = ompi_list_get_first(data->hostlist);

            while (host_item != ompi_list_get_end(data->hostlist)) {
                /* find enough entries for this slice to go */
                tmp_count = 0;
                for (i = 0 ;
                     i < width && 
                         host_item != ompi_list_get_end(data->hostlist) ;
                     host_item = ompi_list_get_next(host_item), ++i) { 
                    host = (mca_llm_base_hostfile_node_t*) host_item;
                    tmp_count += host->count;
                }
                /* if we don't have anyone, get us out of here.. */
                if (i ==  0) {
                    continue;
                }

                /* make a launch list */
                ompi_list_splice(&launch, ompi_list_get_end(&launch),
                                 data->hostlist,
                                 ompi_list_get_first(data->hostlist),
                                 host_item);

                /* do the launch to the first node in the list, passing
                   him the rest of the list */
                ret = internal_spawn_proc(me, jobid, sched, &launch, 
                                          local_offset, global_start_vpid, 
                                          num_procs);
                if  (OMPI_SUCCESS != ret) {
                    /* well, crap!  put ourselves back together, I guess.
                       Should call killjob */
                    ompi_list_join(&done, ompi_list_get_end(&done), &launch);
                    ompi_list_join(data->hostlist, 
                                   ompi_list_get_first(data->hostlist),
                                   &done);
                    return ret;
                }
                local_offset += tmp_count;

                /* copy the list over to the done part */
                ompi_list_join(&done, ompi_list_get_end(&done), &launch);

                /* delay before the next start */
                sleep_time = me->delay_time;
                while (sleep_time > 0) {
                    sleep_time = sleep(sleep_time);
                }
            }

            /* put the list back where we found it... */
            ompi_list_join(data->hostlist, ompi_list_get_end(data->hostlist), 
                           &done);
        }
    }

    OBJ_DESTRUCT(&done);
    OBJ_DESTRUCT(&launch);

    return OMPI_SUCCESS;
}


static int
internal_need_profile(mca_pcm_rsh_module_t *me,
                      mca_llm_base_hostfile_node_t *start_node,
                      int stderr_is_error, bool *needs_profile)
{
    struct passwd *p=NULL;
    char shellpath[PRS_BUFSIZE];
    char** cmdv = NULL;
    char *cmd0 = NULL;
    int cmdc = 0;
    char *printable = NULL;
    char *username = NULL;
    int ret=0;

    /*
     * Figure out if we need to source the .profile on the other side.
     *
     * The following logic is used:
     *
     * if me->no_profile is 1, don't do profile
     * if me->fast_boot is 1, remote shell is assumed same as local
     * if shell is sh/ksh, run profile, otherwise don't
     */
    if (1 == me->no_profile) {
        *needs_profile = false;
        return OMPI_SUCCESS;
    }

    if (1 == me->fast_boot) {
        p = getpwuid(getuid());
        if (NULL == p) return OMPI_ERROR;
            
        ompi_output_verbose(5, mca_pcm_base_output, 
                            "fast boot mode - "
                            "assuming same shell on remote nodes");
        ompi_output_verbose(5, mca_pcm_base_output, 
                            "getpwuid: got local shell %s", p->pw_shell);
        strncpy(shellpath, p->pw_shell, PRS_BUFSIZE - 1);
        shellpath[PRS_BUFSIZE - 1] = '\0';
    } else {
        /* we have to look at the other side  and get our shell */
        username = mca_pcm_base_get_username(start_node);

        cmdv = ompi_argv_split(me->rsh_agent, ' ');
        cmdc = ompi_argv_count(cmdv);

        ompi_argv_append(&cmdc, &cmdv, start_node->hostname);
        if (NULL != username) {
            ompi_argv_append(&cmdc, &cmdv, "-l");
            ompi_argv_append(&cmdc, &cmdv, username);
        }

        ompi_argv_append(&cmdc, &cmdv, "echo $SHELL");
        printable = ompi_argv_join(cmdv, ' ');
        ompi_output_verbose(5, mca_pcm_base_output,
                            "attempting to execute: %s", printable);

        cmd0 = strdup(cmdv[0]);
        shellpath[sizeof(shellpath) - 1] = '\0';
        if (mca_pcm_rsh_ioexecvp(cmdv, 0, shellpath,
                                  sizeof(shellpath) - 1, 
                                  stderr_is_error)) {
            if (errno == EFAULT) {
                ompi_show_help("help-mca-pcm-rsh.txt",
                               "spawn:stderr-output", true, 
                               cmd0, start_node->hostname, printable,
                               strerror(errno));
            } else {
                ompi_show_help("help-mca-pcm-rsh.txt",
                               "spawn:rsh-failed", true,
                               cmd0, printable);
            }

            ret = OMPI_ERROR;
            goto cleanup;
        }

        if ('\n' == shellpath[strlen(shellpath) - 1]) {
            shellpath[strlen(shellpath) - 1] = '\0';
        }
        ompi_output_verbose(5, mca_pcm_base_output,
                            "remote shell %s", shellpath);

        if (NULL == strstr(p->pw_shell, "csh") &&
            NULL == strstr(p->pw_shell, "bash")) {
            /* we are neither csh-derived nor bash.  This probably
               means old-school sh or ksh.  Either way, we
               probably want to run .profile... */
            *needs_profile = true;
        }
    }

    ret = OMPI_SUCCESS;

cleanup:

    /* free up everything we used on the way */
    if (NULL != printable) free(printable);
    if (NULL != cmd0) free(cmd0);
    if (NULL != username) free(username);
    ompi_argv_free(cmdv);
    cmdv = NULL;
    cmdc = 0;

    return ret;
}


static int
internal_spawn_proc(mca_pcm_rsh_module_t *me,
                    mca_ns_base_jobid_t jobid, ompi_rte_node_schedule_t *sched,
                    ompi_list_t *hostlist, int my_start_vpid, 
                    int global_start_vpid, int num_procs)
{
    int kidstdin[2];            /* child stdin pipe */
    bool needs_profile = false;
    mca_llm_base_hostfile_node_t *start_node;
    char** cmdv = NULL;
    char *cmd0 = NULL;
    int cmdc = 0;
    char *printable = NULL;
    int stderr_is_error = me->ignore_stderr == 0 ? 1 : 0;
    char *start_username = NULL;
    int ret;
    pid_t pid;
    FILE *fp;
    int status;			/* exit status */
    int i;
    char *tmp;
    bool high_qos = (0 != (me->constraints & OMPI_RTE_SPAWN_HIGH_QOS));
    bool is_local;

    start_node = (mca_llm_base_hostfile_node_t*) ompi_list_get_first(hostlist);
    start_username = mca_pcm_base_get_username(start_node);

    /* add all the startup stuff if needed */
    is_local = ompi_ifislocal(start_node->hostname) && 
        start_username == NULL;

    if (!is_local) {
        /*
         * Check to see if we need to do the .profile thing
         */
        ret = internal_need_profile(me, start_node, stderr_is_error,
                                    &needs_profile);
        if (OMPI_SUCCESS != ret) {
            goto cleanup;
        }

        /*
         * Build up start array
         */

        /* build up the rsh command part */
        cmdv = ompi_argv_split(me->rsh_agent, ' ');
        cmdc = ompi_argv_count(cmdv);

        ompi_argv_append(&cmdc, &cmdv, start_node->hostname);
        if (NULL != start_username) {
            ompi_argv_append(&cmdc, &cmdv, "-l");
            ompi_argv_append(&cmdc, &cmdv, start_username);
        }

        /* add the start of .profile thing if required */
        if (needs_profile) {
            ompi_argv_append(&cmdc, &cmdv, "( ! [ -e ./.profile ] || . ./.profile;");
        }
    }

    /* build the command to start */
    ompi_argv_append(&cmdc, &cmdv, BOOTAGENT);

    /* starting vpid for launchee's procs */
    tmp = ompi_ltostr(my_start_vpid);
    ompi_argv_append(&cmdc, &cmdv, "--local_offset");
    ompi_argv_append(&cmdc, &cmdv, tmp);
    free(tmp);

    /* keep stdio open? */
    if (high_qos) {
        ompi_argv_append(&cmdc, &cmdv, "--high_qos");
    }

    /* add the end of the .profile thing if required */
    if (!is_local && needs_profile) {
        ompi_argv_append(&cmdc, &cmdv, ")");
    }

    /*
     * Start the process already
     */
    
    if (pipe(kidstdin)) {
        ret = OMPI_ERROR;
        goto cleanup;
    }

    if ((pid = fork()) < 0) {
        ret = OMPI_ERROR;
        goto cleanup;
    } else if (pid == 0) {
        /* child */
        setpgid(0, 0);

        if ((dup2(kidstdin[0], 0) < 0)) {
            /* BWB - XXX - FIX ME to use show help */
            perror(cmdv[0]);
            exit(errno);
        }

        if (close(kidstdin[0]) || close(kidstdin[1])) {
            /* BWB - XXX - FIX ME to use show help */
            perror(cmdv[0]);
            exit(errno);
        }

        /* Ensure that we close all other file descriptors */

        for (i = 3; i < FD_SETSIZE; i++)
            close(i);

        execvp(cmdv[0], cmdv);
        exit(errno);

    } else {
        int comm_fd;

        setpgid(pid, 0);
        /* parent */
        close(kidstdin[0]);

        /* send our stuff down the wire - dup so that we can close the
           fp without killing the other side's stdin, since stdin
           going away is how the remote side knows that it's time to
           kill everyone. */
        comm_fd = dup(kidstdin[1]);
        fp = fdopen(comm_fd, "a");
        if (fp == NULL) { 
            ompi_show_help("help-mca-pcm-rsh.txt",
                           "spawn:application-send", true,
                           strerror(errno));
            ret = OMPI_ERROR;
            kill(pid, SIGTERM);
            goto proc_cleanup;
        }
        /* BWB - XXX - FIX ME - CELLID */
        ret = mca_pcm_base_send_schedule(fp, 0, jobid, 
                                         global_start_vpid,
                                         num_procs,          
                                         sched, 
                                         start_node->count);
        fclose(fp);
        if (OMPI_SUCCESS != ret) {
            ompi_show_help("help-mca-pcm-rsh.txt",
                           "spawn:application-send", true,
                           strerror(errno));
            kill(pid, SIGTERM);
            goto proc_cleanup;
        }
    }
    
    ret = OMPI_SUCCESS;

proc_cleanup:

    if (high_qos) {
      for (i = 0 ; i < start_node->count ; ++i) {
	ompi_process_name_t *name;

	name = ompi_name_server.create_process_name(0, jobid, my_start_vpid + i);

	/* register job info */
	ret = mca_pcm_base_data_store_add_pid(me->data_store,
					name, pid);
	ret = mca_pcm_base_kill_register((mca_pcm_base_module_t*) me, name);

	ompi_name_server.free_name(name);
      }

      ret = ompi_rte_wait_cb(pid, internal_wait_cb, me);
      if (ret != OMPI_SUCCESS) goto cleanup;
    } else {
        /* Wait for the command to exit.  */
        while (1) {
            int rc = ompi_rte_waitpid(pid, &status, 0);
            if (! (rc == -1 && errno == EINTR)) {
                break;
            }
        }

        if (WEXITSTATUS(status)) {
            errno = WEXITSTATUS(status);
            ret = OMPI_ERROR;
        }
    }

 cleanup:
    /* free up everything we used on the way */
    if (NULL != printable) free(printable);
    if (NULL != cmd0) free(cmd0);
    if (NULL != start_username) free(start_username);
    ompi_argv_free(cmdv);
    cmdv = NULL;
    cmdc = 0;

    return ret;
}


static void
internal_wait_cb(pid_t pid, int status, void *data)
{
    mca_pcm_rsh_module_t *me = (mca_pcm_rsh_module_t*) data;
    ompi_process_name_t **procs;
    size_t procs_len, i;
    ompi_rte_process_status_t *proc_status;
    volatile int spin = 0;
    int ret;

    ompi_output_verbose(10, mca_pcm_base_output, 
                        "process %d exited with status %d", pid, status);

    if (me->debug_callback) {
        printf("internal_wait_cb in pid %d spinning for attach.\n", getpid());
	printf("use \"set variable spin = 0\" to stop spinning\n");
	spin = 1;
	while (spin != 0) ;
    }

    ret = mca_pcm_base_data_store_get_procs(me->data_store, pid, &procs,
                                            &procs_len, true);
    if (ret != OMPI_SUCCESS) {
        ompi_show_help("help-mca-pcm-rsh.txt",
                       "spawn:no-process-record", true, pid, status);
        return;
    }

    /* unregister all the procs */
    for (i = 0 ; i < procs_len ; ++i) {
        proc_status = ompi_rte_get_process_status(procs[i]);
        if (NULL != proc_status) {
            proc_status->status_key = OMPI_PROC_KILLED;
            proc_status->exit_code = (ompi_exit_code_t)status;
            ompi_rte_set_process_status(proc_status, procs[i]);
        }
        free(procs[i]);
    }

    free(procs);
}

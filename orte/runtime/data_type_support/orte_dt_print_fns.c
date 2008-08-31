/*
 * Copyright (c) 2004-2006 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "orte_config.h"
#include "orte/types.h"

#include <sys/types.h>

#include "opal/util/argv.h"
#include "opal/class/opal_list.h"

#include "orte/mca/errmgr/errmgr.h"
#include "opal/dss/dss.h"
#include "orte/util/name_fns.h"

#include "orte/runtime/data_type_support/orte_dt_support.h"

static void orte_dt_quick_print(char **output, char *type_name, char *prefix, void *src, opal_data_type_t real_type)
{
    int8_t *i8;
    int16_t *i16;
    int32_t *i32;
    int64_t *i64;
    uint8_t *ui8;
    uint16_t *ui16;
    uint32_t *ui32;
    uint64_t *ui64;
    
    /* set default result */
    *output = NULL;
    
    /* check for NULL ptr */
    if (NULL == src) {
        asprintf(output, "%sData type: %s\tData size: 8-bit\tValue: NULL pointer",
                 (NULL == prefix) ? "" : prefix, type_name);
        return;
    }
    
    switch(real_type) {
        case OPAL_INT8:
            i8 = (int8_t*)src;
            asprintf(output, "%sData type: %s\tData size: 8-bit\tValue: %d",
                     (NULL == prefix) ? "" : prefix, type_name, (int) *i8);
            break;
            
        case OPAL_UINT8:
            ui8 = (uint8_t*)src;
            asprintf(output, "%sData type: %s\tData size: 8-bit\tValue: %u",
                     (NULL == prefix) ? "" : prefix, type_name, (unsigned int)*ui8);
            break;

        case OPAL_INT16:
            i16 = (int16_t*)src;
            asprintf(output, "%sData type: %s\tData size: 16-bit\tValue: %d", 
                     (NULL == prefix) ? "" : prefix, type_name, (int) *i16);
            break;
            
        case OPAL_UINT16:
            ui16 = (uint16_t*)src;
            asprintf(output, "%sData type: %s\tData size: 16-bit\tValue: %u", 
                     (NULL == prefix) ? "" : prefix, type_name, (unsigned int) *ui16);
            break;
            
        case OPAL_INT32:
            i32 = (int32_t*)src;
            asprintf(output, "%sData type: %s\tData size: 32-bit\tValue: %ld",
                     (NULL == prefix) ? "" : prefix, type_name, (long) *i32);
            break;
            
        case OPAL_UINT32:
            ui32 = (uint32_t*)src;
            asprintf(output, "%sData type: %s\tData size: 32-bit\tValue: %lu",
                     (NULL == prefix) ? "" : prefix, type_name, (unsigned long) *ui32);
            break;
            
        case OPAL_INT64:
            i64 = (int64_t*)src;
            asprintf(output, "%sData type: %s\tData size: 64-bit\tValue: %ld",
                     (NULL == prefix) ? "" : prefix, type_name, (long) *i64);
            break;
            
        case OPAL_UINT64:
            ui64 = (uint64_t*)src;
            asprintf(output, "%sData type: %s\tData size: 64-bit\tValue: %lu",
                     (NULL == prefix) ? "" : prefix, type_name, (unsigned long) *ui64);
            break;
            
        default:
            return;
    }

    return;
}

/*
 * STANDARD PRINT FUNCTION - WORKS FOR EVERYTHING NON-STRUCTURED
 */
int orte_dt_std_print(char **output, char *prefix, void *src, opal_data_type_t type)
{
    /* set default result */
    *output = NULL;
    
    switch(type) {
        case ORTE_STD_CNTR:
            orte_dt_quick_print(output, "ORTE_STD_CNTR", prefix, src, ORTE_STD_CNTR_T);
            break;
        case ORTE_VPID:
            orte_dt_quick_print(output, "ORTE_VPID", prefix, src, ORTE_VPID_T);
            break;
            
        case ORTE_JOBID:
            asprintf(output, "%sData Type: ORTE_JOBID\tData size: %lu\tValue: %s",
                     (NULL == prefix) ? "" : prefix, (unsigned long)sizeof(orte_jobid_t),
                     ORTE_JOBID_PRINT(*(orte_jobid_t*)src));
            break;
            
        case ORTE_PROC_STATE:
            orte_dt_quick_print(output, "ORTE_PROC_STATE", prefix, src, ORTE_PROC_STATE_T);
            break;
            
        case ORTE_JOB_STATE:
            orte_dt_quick_print(output, "ORTE_JOB_STATE", prefix, src, ORTE_JOB_STATE_T);
            break;
            
        case ORTE_NODE_STATE:
            orte_dt_quick_print(output, "ORTE_NODE_STATE", prefix, src, ORTE_NODE_STATE_T);
            break;
            
        case ORTE_EXIT_CODE:
            orte_dt_quick_print(output, "ORTE_EXIT_CODE", prefix, src, ORTE_EXIT_CODE_T);
            break;
        
        case ORTE_RML_TAG:
            orte_dt_quick_print(output, "ORTE_RML_TAG", prefix, src, ORTE_RML_TAG_T);
            break;
        
        case ORTE_DAEMON_CMD:
            orte_dt_quick_print(output, "ORTE_DAEMON_CMD", prefix, src, ORTE_DAEMON_CMD_T);
            break;

        case ORTE_GRPCOMM_MODE:
            orte_dt_quick_print(output, "ORTE_GRPCOMM_MODE", prefix, src, ORTE_GRPCOMM_MODE_T);
            break;
            
        default:
            ORTE_ERROR_LOG(ORTE_ERR_UNKNOWN_DATA_TYPE);
            return ORTE_ERR_UNKNOWN_DATA_TYPE;
    }
    
    return ORTE_SUCCESS;
}

/*
 * NAME
 */
int orte_dt_print_name(char **output, char *prefix, orte_process_name_t *name, opal_data_type_t type)
{
    /* set default result */
    *output = NULL;
    
    if (NULL == name) {
        asprintf(output, "%sData type: ORTE_PROCESS_NAME\tData Value: NULL",
                 (NULL == prefix ? " " : prefix));
    } else {
        asprintf(output, "%sData type: ORTE_PROCESS_NAME\tData Value: %s",
                 (NULL == prefix ? " " : prefix), ORTE_NAME_PRINT(name));
    }
    
    return ORTE_SUCCESS;
}


/*
 * JOB
 */
int orte_dt_print_job(char **output, char *prefix, orte_job_t *src, opal_data_type_t type)
{
    char *tmp, *tmp2, *tmp3, *pfx2, *pfx;
    int32_t i;
    int rc;

    /* set default result */
    *output = NULL;

    /* protect against NULL prefix */
    if (NULL == prefix) {
        asprintf(&pfx2, " ");
    } else {
        asprintf(&pfx2, "%s", prefix);
    }

    asprintf(&tmp, "\n%sData for job: %s\tNum apps: %ld\tControls: %0x\tState: %0x\tAbort: %s", pfx2,
             ORTE_JOBID_PRINT(src->jobid),
             (long)src->num_apps, src->controls,
             src->state, src->abort ? "True" : "False");

    asprintf(&pfx, "%s\t", pfx2);
    free(pfx2);
    
    for (i=0; i < src->num_apps; i++) {
        opal_dss.print(&tmp2, pfx, src->apps->addr[i], ORTE_APP_CONTEXT);
        asprintf(&tmp3, "%s\n%s", tmp, tmp2);
        free(tmp);
        free(tmp2);
        tmp = tmp3;
    }
    
    if (NULL != src->map) {
        if (ORTE_SUCCESS != (rc = opal_dss.print(&tmp2, pfx, src->map, ORTE_JOB_MAP))) {
            ORTE_ERROR_LOG(rc);
            return rc;
        }
        asprintf(&tmp3, "%s%s", tmp, tmp2);
        free(tmp);
        free(tmp2);
        tmp = tmp3;
    } else {
        asprintf(&tmp2, "%s\n%sNo Map", tmp, pfx);
        free(tmp);
        tmp = tmp2;
    }
    
    asprintf(&tmp2, "%s\n%sNum procs: %ld", tmp, pfx, (long)src->num_procs);
    free(tmp);
    tmp = tmp2;

    for (i=0; i < src->procs->size; i++) {
        if (NULL != src->procs->addr[i]) {
            if (ORTE_SUCCESS != (rc = opal_dss.print(&tmp2, pfx, src->procs->addr[i], ORTE_PROC))) {
                ORTE_ERROR_LOG(rc);
                return rc;
            }
            asprintf(&tmp3, "%s%s", tmp, tmp2);
            free(tmp);
            free(tmp2);
            tmp = tmp3;
        }
    }

    asprintf(&tmp2, "%s\n%s\tNum launched: %ld\tNum reported: %ld\n%s\tNum terminated: %ld\tOversubscribe override?: %s",
             tmp, pfx, (long)src->num_launched, (long)src->num_reported, pfx,
             (long)src->num_terminated, src->oversubscribe_override ? "True" : "False");
    free(tmp);
    tmp = tmp2;
    
    /* set the return */
    *output = tmp;
    free(pfx);

    return ORTE_SUCCESS;
}

/*
 * NODE
 */
int orte_dt_print_node(char **output, char *prefix, orte_node_t *src, opal_data_type_t type)
{
    char *tmp, *tmp2, *tmp3, *pfx2, *pfx;
    int32_t i;
    int rc;
    
    /* set default result */
    *output = NULL;
    
    /* protect against NULL prefix */
    if (NULL == prefix) {
        asprintf(&pfx2, " ");
    } else {
        asprintf(&pfx2, "%s", prefix);
    }

#if !ORTE_DISABLE_FULL_SUPPORT
    if (orte_xml_output) {
        /* need to create the output in XML format */
        asprintf(output, "%s<host name=\"%s\" slots=\"%d\" max_slots=\"%d\">\n", pfx2,
                 (NULL == src->name) ? "UNKNOWN" : src->name,
                 (int)src->slots, (int)src->slots_max);
        free(pfx2);
        return ORTE_SUCCESS;
    }
#endif
    
    asprintf(&tmp, "\n%sData for node: Name: %s\t%s\tLaunch id: %ld\tArch: %0x\tState: %0x",
             pfx2, src->name,
             pfx2, (long)src->launch_id,
             src->arch, src->state);
    
    if (NULL == src->daemon) {
        asprintf(&tmp2, "%s\n%s\tDaemon: %s\tDaemon launched: %s", tmp, pfx2,
                 "Not defined", src->daemon_launched ? "True" : "False");
    } else {
        asprintf(&tmp2, "%s\n%s\tDaemon: %s\tDaemon launched: %s", tmp, pfx2,
                 ORTE_NAME_PRINT(&(src->daemon->name)), src->daemon_launched ? "True" : "False");
    }
    free(tmp);
    tmp = tmp2;
    
    asprintf(&tmp2, "%s\n%s\tNum slots: %ld\tSlots in use: %ld", tmp, pfx2,
             (long)src->slots, (long)src->slots_inuse);
    free(tmp);
    tmp = tmp2;
    
    asprintf(&tmp2, "%s\n%s\tNum slots allocated: %ld\tMax slots: %ld", tmp, pfx2,
             (long)src->slots_alloc, (long)src->slots_max);
    free(tmp);
    tmp = tmp2;
    
    asprintf(&tmp2, "%s\n%s\tUsername on node: %s", tmp, pfx2,
             (NULL == src->username) ? "NULL" : src->username);
    free(tmp);
    tmp = tmp2;
    
    asprintf(&tmp2, "%s\n%s\tNum procs: %ld\tNext node_rank: %ld", tmp, pfx2,
             (long)src->num_procs, (long)src->next_node_rank);
    free(tmp);
    tmp = tmp2;
    
    asprintf(&pfx, "%s\t", pfx2);
    free(pfx2);
    
    for (i=0; i < src->procs->size; i++) {
        if (NULL != src->procs->addr[i]) {
            if (ORTE_SUCCESS != (rc = opal_dss.print(&tmp2, pfx, src->procs->addr[i], ORTE_PROC))) {
                ORTE_ERROR_LOG(rc);
                return rc;
            }
            asprintf(&tmp3, "%s%s", tmp, tmp2);
            free(tmp);
            free(tmp2);
            tmp = tmp3;
        }
    }
    free(pfx);

    /* set the return */
    *output = tmp;
    
    return ORTE_SUCCESS;
}

/*
 * PROC
 */
static char* orte_dt_print_proc_state(orte_proc_state_t state)
{
    switch(state) {
        case ORTE_PROC_STATE_INIT:
            return "init";
        case ORTE_PROC_STATE_LAUNCHED:
            return "launched";
        case ORTE_PROC_STATE_RUNNING:
            return "running";
        case ORTE_PROC_STATE_TERMINATED:
            return "terminated";
        case ORTE_PROC_STATE_ABORTED:
            return "aborted";
        case ORTE_PROC_STATE_FAILED_TO_START:
            return "failed-to-start";
        case ORTE_PROC_STATE_ABORTED_BY_SIG:
            return "aborted-by-signal";
        case ORTE_PROC_STATE_TERM_WO_SYNC:
            return "terminated-without-sync";
        default:
            return NULL;
    }
}

int orte_dt_print_proc(char **output, char *prefix, orte_proc_t *src, opal_data_type_t type)
{
    char *tmp, *tmp2, *pfx2;
    
    /* set default result */
    *output = NULL;
    
    /* protect against NULL prefix */
    if (NULL == prefix) {
        asprintf(&pfx2, " ");
    } else {
        asprintf(&pfx2, "%s", prefix);
    }
    
#if !ORTE_DISABLE_FULL_SUPPORT
    if (orte_xml_output) {
        /* need to create the output in XML format */
        tmp = orte_dt_print_proc_state(src->state);
        if (NULL == tmp) {
            if (0 == src->pid) {
                asprintf(output, "%s<process rank=\"%s\"/>\n", pfx2, ORTE_VPID_PRINT(src->name.vpid));
            } else {
                asprintf(output, "%s<process rank=\"%s\" pid=\"%d\"/>\n", pfx2,
                         ORTE_VPID_PRINT(src->name.vpid), (int)src->pid);
            }
        } else {
            if (0 == src->pid) {
                asprintf(output, "%s<process rank=\"%s\" status=\"%s\"/>\n", pfx2,
                         ORTE_VPID_PRINT(src->name.vpid), tmp);
            } else {
                asprintf(output, "%s<process rank=\"%s\" pid=\"%d\" status=\"%s\"/>\n", pfx2,
                         ORTE_VPID_PRINT(src->name.vpid), (int)src->pid, tmp);
            }
        }
        free(pfx2);
        return ORTE_SUCCESS;
    }
#endif

    asprintf(&tmp, "\n%sData for proc: %s", pfx2, ORTE_NAME_PRINT(&src->name));
    
    asprintf(&tmp2, "%s\n%s\tPid: %ld\tLocal rank: %ld\tNode rank: %ld", tmp, pfx2,
             (long)src->pid, (long)src->local_rank, (long)src->node_rank);
    free(tmp);
    tmp = tmp2;
    
    asprintf(&tmp2, "%s\n%s\tState: %0x\tApp_context: %ld\tSlot list: %s", tmp, pfx2,
             src->state, (long)src->app_idx,
             (NULL == src->slot_list) ? "NULL" : src->slot_list);
    free(tmp);
    
    /* set the return */
    *output = tmp2;
    
    free(pfx2);
    return ORTE_SUCCESS;
}

/*
 * APP CONTEXT
 */
int orte_dt_print_app_context(char **output, char *prefix, orte_app_context_t *src, opal_data_type_t type)
{
    char *tmp, *tmp2, *pfx2;
    int i, count;
    
    /* set default result */
    *output = NULL;
    
    /* protect against NULL prefix */
    if (NULL == prefix) {
        asprintf(&pfx2, " ");
    } else {
        asprintf(&pfx2, "%s", prefix);
    }
    
    asprintf(&tmp, "\n%sData for app_context: index %lu\tapp: %s\n%s\tNum procs: %lu",
             pfx2, (unsigned long)src->idx, src->app,
             pfx2, (unsigned long)src->num_procs);
    
    count = opal_argv_count(src->argv);
    for (i=0; i < count; i++) {
        asprintf(&tmp2, "%s\n%s\tArgv[%d]: %s", tmp, pfx2, i, src->argv[i]);
        free(tmp);
        tmp = tmp2;
    }
    
    count = opal_argv_count(src->env);
    for (i=0; i < count; i++) {
        asprintf(&tmp2, "%s\n%s\tEnv[%lu]: %s", tmp, pfx2, (unsigned long)i, src->env[i]);
        free(tmp);
        tmp = tmp2;
    }
    
    asprintf(&tmp2, "%s\n%s\tWorking dir: %s (user: %d)\n%s\tHostfile: %s\tAdd-Hostfile: %s", tmp, pfx2, src->cwd, (int) src->user_specified_cwd,
             pfx2, (NULL == src->hostfile) ? "NULL" : src->hostfile,
             (NULL == src->add_hostfile) ? "NULL" : src->add_hostfile);
    free(tmp);
    tmp = tmp2;
    
    count = opal_argv_count(src->dash_host);
    for (i=0; i < count; i++) {
        asprintf(&tmp2, "%s\n%s\tDash_host[%lu]: %s", tmp, pfx2, (unsigned long)i, src->dash_host[i]);
        free(tmp);
        tmp = tmp2;
    }
    
    /* set the return */
    *output = tmp;
    
    free(pfx2);
    return ORTE_SUCCESS;
}

/*
 * JOB_MAP
 */
int orte_dt_print_map(char **output, char *prefix, orte_job_map_t *src, opal_data_type_t type)
{
    char *tmp, *tmp2, *tmp3, *pfx, *pfx2;
    int32_t i, j;
    int rc;
    orte_node_t **nodes;
    orte_proc_t **procs;
    
    /* set default result */
    *output = NULL;
    
    /* protect against NULL prefix */
    if (NULL == prefix) {
        asprintf(&pfx2, " ");
    } else {
        asprintf(&pfx2, "%s", prefix);
    }
    
#if !ORTE_DISABLE_FULL_SUPPORT
    if (orte_xml_output) {
        /* need to create the output in XML format */
        asprintf(&tmp, "<map>\n");
        /* loop through nodes */
        nodes = (orte_node_t**)src->nodes->addr;
        for (i=0; i < src->nodes->size; i++) {
            if (NULL == nodes[i]) {
                break;
            }
            orte_dt_print_node(&tmp2, "\t", nodes[i], ORTE_NODE);
            asprintf(&tmp3, "%s%s", tmp, tmp2);
            free(tmp2);
            free(tmp);
            tmp = tmp3;
            /* for each node, loop through procs and print their rank */
            procs = (orte_proc_t**)nodes[i]->procs->addr;
            for (j=0; j < nodes[i]->procs->size; j++) {
                if (NULL == procs[j]) {
                    break;
                }
                orte_dt_print_proc(&tmp2, "\t\t", procs[j], ORTE_PROC);
                asprintf(&tmp3, "%s%s", tmp, tmp2);
                free(tmp2);
                free(tmp);
                tmp = tmp3;
            }
            asprintf(&tmp3, "%s\t</host>\n", tmp);
            free(tmp);
            tmp = tmp3;
        }
        asprintf(&tmp2, "%s</map>\n", tmp);
        free(tmp);
        free(pfx2);
        *output = tmp2;
        return ORTE_SUCCESS;
        
    }
#endif

    asprintf(&pfx, "%s\t", pfx2);
    asprintf(&tmp, "\n%sMap generated by mapping policy: %x\n%s\tPernode: %s\tNpernode: %ld\tOversubscribe allowed: %s\tCPU Lists: %s",
             pfx2, src->policy, pfx2,
             (src->pernode) ? "TRUE" : "FALSE", (long)src->npernode,
             (src->oversubscribe) ? "TRUE" : "FALSE",
             (src->cpu_lists) ? "TRUE" : "FALSE");
    free(pfx2);
    
    if (ORTE_VPID_INVALID == src->daemon_vpid_start) {
        asprintf(&tmp2, "%s\n%sNum new daemons: %ld\tNew daemon starting vpid INVALID\n%sNum nodes: %ld",
                 tmp, pfx, (long)src->num_new_daemons, pfx, (long)src->num_nodes);
    } else {
        asprintf(&tmp2, "%s\n%sNum new daemons: %ld\tNew daemon starting vpid %ld\n%sNum nodes: %ld",
                 tmp, pfx, (long)src->num_new_daemons, (long)src->daemon_vpid_start,
                 pfx, (long)src->num_nodes);
    }
    free(tmp);
    tmp = tmp2;
    
    
    for (i=0; i < src->nodes->size; i++) {
        if (NULL != src->nodes->addr[i]) {
            if (ORTE_SUCCESS != (rc = opal_dss.print(&tmp2, pfx, src->nodes->addr[i], ORTE_NODE))) {
                ORTE_ERROR_LOG(rc);
                free(pfx);
                free(tmp);
                return rc;
            }
            asprintf(&tmp3, "%s\n%s", tmp, tmp2);
            free(tmp);
            free(tmp2);
            tmp = tmp3;
        }
    }
    
    /* set the return */
    *output = tmp;
    
    free(pfx);
    return ORTE_SUCCESS;
}

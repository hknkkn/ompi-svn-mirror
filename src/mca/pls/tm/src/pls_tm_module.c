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
 *
 * These symbols are in a file by themselves to provide nice linker
 * semantics.  Since linkers generally pull in symbols by object
 * files, keeping these symbols as the only symbols in this file
 * prevents utility programs such as "ompi_info" from having to import
 * entire components just to query their version and parameters.
 */

#include "ompi_config.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "include/orte_constants.h"
#include "include/orte_types.h"
#include "util/argv.h"
#include "util/output.h"
#include "mca/pls/pls.h"
#include "pls_tm.h"
#include "tm.h"


const orte_pls_base_module_1_0_0_t orte_pls_tm_module = {
    orte_pls_tm_launch,
    orte_pls_tm_finalize
};


/*
 * Local functions
 */
static int do_tm_resolve(char **hostnames, size_t num_hostnames, 
                         tm_node_id *app_tm_node_ids);
static char* get_tm_hostname(tm_node_id node);


int orte_pls_tm_launch(orte_jobid_t jobid)
{
    int ret, local_errno;
    orte_app_context_t *app;
    size_t i, j, app_size, count, total_count;
    struct tm_roots root;
    tm_task_id tid;
    tm_node_id *tm_node_ids;
    tm_event_t event;
    char *flat;
    int *statuses;
    char old_cwd[OMPI_PATH_MAX];
    char **hostnames = NULL;
    int num_hostnames = 0;

    /* Open up our connection to tm */

    ret = tm_init(NULL, &root);
    if (TM_SUCCESS != ret) {
        /* JMS May change...? */
        return ORTE_ERR_RESOURCE_BUSY;
    }

    /* Get the array of app contexts */

    ret = orte_rmgr_base_get_app_context(jobid, &app, &app_size);
    if (ORTE_SUCCESS != ret) {
        /* JMS May change...? */
        tm_finalize();
        return ret;
    }

    /* Count up how many processes will be starting and create/zero
       out a list of exit statuses (this could be combined with other
       loops below for higher efficiency, but I'm choosing to code for
       simplicity here). */

    for (total_count = i = 0; i < app_size; ++i) {
        total_count += app[i].num_procs;
    }
    statuses = malloc(sizeof(int) * total_count);
    if (NULL == statuses) {
        /* JMS may change...? */
        free(app);
        tm_finalize();
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    for (i = 0; i < total_count; ++i) {
        statuses[i] = ORTE_PROC_STARTING;
    }

    /* Check to ensure that all the cwd's exist */

    ompi_getcwd(old_cwd, OMPI_PATH_MAX);
    for (count = i = 0; i < app_size; ++i) {

        /* Try changing to the cwd and then changing back */

        if (0 != chdir(app[i].cwd) ||
            0 != chdir(old_cwd)) {
            /* JMS May change...? */
            free(statuses);
            free(app);
            tm_finalize();
            return ORTE_ERR_NOT_FOUND;
        }
    }

    /* Get the hostnames from the output of the mapping */

    /* JMS ....? */

    /* Sanity check -- we should have exactly as many hosts as total
       number of processes to be launched */

    if (num_hostnames != total_count) {
        free(statuses);
        free(app);
        tm_finalize();
        return ORTE_ERR_BAD_PARAM;
    }

    /* Convert all the hostnames to TM node IDs */

    tm_node_ids = malloc(sizeof(tm_node_id) * num_hostnames);
    if (NULL == tm_node_ids) {
        /* JMS May change...? */
        free(statuses);
        free(app);
        tm_finalize();
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    ret = do_tm_resolve(hostnames, num_hostnames, tm_node_ids);
    if (ORTE_SUCCESS != ret) {
        /* JMS May change...? */
        free(tm_node_ids);
        free(statuses);
        free(app);
        tm_finalize();
        return ret;
    }

    /* Launch them */

    for (count = i = 0; i < app_size; ++i) {
        flat = ompi_argv_join(app[i].argv, ' ');
        for (j = 0; j < app[i].num_procs; ++j, ++count) {
            ompi_output_verbose(10, 0, "Launching app %d, process %d: %s",
                                i, j, flat);

            /* Do the spawn */

            ret = tm_spawn(app[i].argc, app[i].argv, app[i].env, 
                           tm_node_ids[count], &tid, &event);
            if (TM_SUCCESS != ret) {
                statuses[count] = ORTE_PROC_EXITED;
                ret = ORTE_ERR_RESOURCE_BUSY;
                goto loop_error;
            }
            
            ret = tm_poll(TM_NULL_EVENT, &event, 1, &local_errno);
            if (TM_SUCCESS != ret) {
                statuses[count] = ORTE_PROC_EXITED;
                ret = ORTE_ERR_RESOURCE_BUSY;
                goto loop_error;
            }

            statuses[count] = ORTE_PROC_ALIVE;
            continue;

        loop_error:
            free(flat);
            free(tm_node_ids);
            free(statuses);
            free(app);
            tm_finalize();
            return ret;
        }
        free(flat);
    }

    /* All done */

    free(tm_node_ids);
    free(statuses);
    free(app);
    tm_finalize();
    return ORTE_SUCCESS;
}


/*
 * There's really nothing to do here
 */
int orte_pls_tm_finalize(void)
{
    return ORTE_SUCCESS;
}


/*
 * Take a list of hostnames and return their corresponding TM node
 * ID's.  This is not the most efficient method of doing this, but
 * it's not much of an issue here (this is not a performance-critical
 * section of code)
 */
static int do_tm_resolve(char **hostnames, size_t num_hostnames, 
                         tm_node_id *app_tm_node_ids)
{
    tm_node_id *tm_node_ids;
    size_t i, j, count;
    int ret, num_node_ids, num_tm_hostnames = 0;
    char **tm_hostnames = NULL, *h;

    /* Get the list of nodes allocated in this PBS job */

    ret = tm_nodeinfo(&tm_node_ids, &num_node_ids);
    if (TM_SUCCESS != ret) {
        /* JMS May change...? */
        return ORTE_ERR_NOT_FOUND;
    }

    /* TM "nodes" may actually correspond to PBS "VCPUs", which means
       there may be multiple "TM nodes" that correspond to the same
       physical node.  This doesn't really affect what we're doing
       here (we actually ignore the fact that they're duplicates --
       slightly inefficient, but no big deal); just mentioned for
       completeness... */

    for (i = 0; i < num_node_ids; ++i) {
        h = get_tm_hostname(tm_node_ids[i]);
        ompi_argv_append(&num_tm_hostnames, &tm_hostnames, h);
        free(h);
    }

    /* Now go through the list of hostnames that was given to us and
       return a list of tm node ID's corresponding to them. */

    for (count = i = 0; i < num_hostnames; ++i) {
        for (j = 0; j < num_tm_hostnames; ++j, ++count) {
            if (0 == strcmp(hostnames[i], tm_hostnames[j])) {
                app_tm_node_ids[count] = tm_node_ids[j];
                break;
            }
        }

        if (num_tm_hostnames + 1 == j) {
            /* JMS May change...? */
            ompi_argv_free(tm_hostnames);
            return ORTE_ERR_NOT_FOUND;
        }
    }

    /* Happiness */

    ompi_argv_free(tm_hostnames);
    return ORTE_SUCCESS;
}


/*
 * For a given TM node ID, get the string hostname corresponding to
 * it.
 */
static char* get_tm_hostname(tm_node_id node)
{
    int ret, local_errno;
    char *hostname;
    tm_event_t event;
    char buffer[256];
    char **argv;

    /* Get the info string corresponding to this TM node ID */

    ret = tm_rescinfo(node, buffer, sizeof(buffer) - 1, &event);
    if (TM_SUCCESS != ret) {
        return NULL;
    }

    /* Now wait for that event to happen */

    ret = tm_poll(TM_NULL_EVENT, &event, 1, &local_errno);
    if (TM_SUCCESS != ret) {
        return NULL;
    }

    /* According to the TM man page, we get back a space-separated
       string array.  The hostname is the second item.  Use a cheap
       trick to get it. */

    buffer[sizeof(buffer) - 1] = '\0';
    argv = ompi_argv_split(buffer, ' ');
    if (NULL == argv) {
        return NULL;
    }
    hostname = strdup(argv[1]);
    ompi_argv_free(argv);

    /* All done */

    return hostname;
}

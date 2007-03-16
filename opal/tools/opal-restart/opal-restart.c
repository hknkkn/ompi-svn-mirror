/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
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

/**
 * @file
 * OPAL Restart command
 *
 * This command will restart a single process from 
 * the checkpoint generated by the opal-checkpoint 
 * command.
 */
#include "opal_config.h"

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif  /*  HAVE_STDLIB_H */
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif  /* HAVE_STRING_H */

#include "opal/constants.h"

#include "opal/util/cmd_line.h"
#include "opal/util/argv.h"
#include "opal/util/show_help.h"
#include "opal/util/output.h"
#include "opal/util/opal_environ.h"
#include "opal/util/os_path.h"
#include "opal/mca/base/base.h"
#include "opal/mca/base/mca_base_param.h"

#include "opal/runtime/opal.h"
#include "opal/runtime/opal_cr.h"

#include "opal/mca/crs/crs.h"
#include "opal/mca/crs/base/base.h"

extern char **environ;

/******************
 * Local Functions
 ******************/
static int initialize(int argc, char *argv[]);
static int finalize(void);
static int parse_args(int argc, char *argv[]);
static int check_file(char *given_filename);
static int post_env_vars(int prev_pid);

/*****************************************
 * Global Vars for Command line Arguments
 *****************************************/
static char *expected_crs_comp = NULL;

typedef struct {
    bool help;
    char *filename;
    bool verbose;
    bool forked;
    bool self_case;
    char *snapshot_loc;
    int  output;
} opal_restart_globals_t;

opal_restart_globals_t opal_restart_globals;

opal_cmd_line_init_t cmd_line_opts[] = {
    { NULL, NULL, NULL, 
      'h', NULL, "help", 
      0,
      &opal_restart_globals.help, OPAL_CMD_LINE_TYPE_BOOL,
      "This help message" },

    { NULL, NULL, NULL, 
      'v', NULL, "verbose", 
      0,
      &opal_restart_globals.verbose, OPAL_CMD_LINE_TYPE_BOOL,
      "Be Verbose" },

    { NULL, NULL, NULL, 
      '\0', NULL, "fork", 
      0,
      &opal_restart_globals.forked, OPAL_CMD_LINE_TYPE_BOOL,
      "Fork off a new process which is the restarted process instead of "
      "replacing opal_restart" },

    { "crs", "base", "snapshot_dir",
      'w', NULL, "where",
      1,
      &opal_restart_globals.snapshot_loc, OPAL_CMD_LINE_TYPE_STRING,
      "Where to find the checkpoint files. In most cases this is automatically "
      "detected, however if a custom location was specified to opal-checkpoint "
      "then this argument is meant to match it."},

    /*
     * We do this instead of using the '-mca crs self' convention as to not
     * influence the user into thinking that they need to do this for all of the
     * checkpointers. And to reinforce that the 'self' module is an exception, and
     * all other modules are automaticly detected.
     */
    { NULL, NULL, NULL, 
      's', NULL, "self", 
      0,
      &opal_restart_globals.self_case, OPAL_CMD_LINE_TYPE_BOOL,
      "Is this a restart using the 'self' module. This is a special case as all "
      "other modules are automaticly detected" },

    /* End of list */
    { NULL, NULL, NULL, 
      '\0', NULL, NULL, 
      0,
      NULL, OPAL_CMD_LINE_TYPE_NULL,
      NULL }
};

int
main(int argc, char *argv[])
{
    int ret, exit_status = OPAL_SUCCESS;
    int child_pid;
    int prev_pid = 0;
    opal_crs_base_snapshot_t *snapshot = NULL;

    /***************
     * Initialize
     ***************/
    if (OPAL_SUCCESS != (ret = initialize(argc, argv))) {
        exit_status = ret;
        goto cleanup;
    }
    
    /* 
     * Check for existence of the file, or program in the case of self
     */
    if( OPAL_SUCCESS != (ret = check_file(opal_restart_globals.filename) )) {
        opal_show_help("help-opal-restart.txt", "invalid_filename", true,
                       opal_restart_globals.filename);
        exit_status = ret;
        goto cleanup;
    }

    /* Re-enable the selection of the CRS component, so we can choose the right one */
    opal_setenv(mca_base_param_env_var("crs_base_do_not_select"),
                "0", /* turn on the selection */
                true, &environ);

    /*
     * Make sure we are using the correct checkpointer
     */
    if(NULL == expected_crs_comp) {
        char * base = NULL;

        base = opal_crs_base_get_snapshot_directory(opal_restart_globals.filename);
        expected_crs_comp = strdup(opal_crs_base_extract_expected_component(base, &prev_pid));

        free(base);
    }
    
    opal_output_verbose(10, opal_restart_globals.output,
                        "Restart Expects checkpointer: (%s)",
                        expected_crs_comp);
    
    opal_setenv(mca_base_param_env_var("crs"),
                expected_crs_comp,
                true, &environ);
    
    /* Select this component or don't continue.
     * If the selection of this component fails, then we can't 
     * restart on this node because it doesn't have the proper checkpointer
     * available. 
     */
    if( OPAL_SUCCESS != (ret = opal_crs_base_select()) ) {
        opal_show_help("help-opal-restart.txt", "comp_select_failure", true,
                       expected_crs_comp, ret);
        exit_status = ret;
        goto cleanup;
    }
    
    /*
     * Make sure we have selected the proper component
     */
    if(0 != strncmp(expected_crs_comp, 
                    opal_crs_base_selected_component.crs_version.mca_component_name, 
                    strlen(expected_crs_comp)) ) {
        opal_show_help("help-opal-restart.txt", "comp_select_mismatch", 
                       true,
                       expected_crs_comp, 
                       opal_crs_base_selected_component.crs_version.mca_component_name,
                       ret);
        exit_status = ret;
        goto cleanup;
    }

    /******************************
     * Restart in this process
     ******************************/
    opal_output_verbose(10, opal_restart_globals.output,
                        "Restarting from file (%s)",
                        opal_restart_globals.filename);
    if( opal_restart_globals.forked ) {
        opal_output_verbose(10, opal_restart_globals.output,
                            "\t Forking off a child");
    } else {
        opal_output_verbose(10, opal_restart_globals.output,
                            "\t Exec in self");
    }

    /* We are launching a program that is not going to be a tool :) */
    opal_unsetenv(mca_base_param_env_var("opal_cr_is_tool"),
                  &environ);

    snapshot = OBJ_NEW(opal_crs_base_snapshot_t);
    snapshot->cold_start      = true;
    snapshot->reference_name  = strdup(opal_restart_globals.filename);
    snapshot->local_location  = opal_crs_base_get_snapshot_directory(snapshot->reference_name);
    snapshot->remote_location = strdup(snapshot->local_location);

    /* Since some checkpoint/restart systems don't pass along env vars to the
     * restarted app, we need to take care of that.
     */
    if(OPAL_SUCCESS != (ret = post_env_vars(prev_pid) ) ) {
        exit_status = ret;
        goto cleanup;
    }

    ret = opal_crs.crs_restart(snapshot, 
                               opal_restart_globals.forked,
                               &child_pid);

    if (OPAL_SUCCESS != ret) {
        opal_show_help("help-opal-restart.txt", "restart_cmd_failure", true,
                       opal_restart_globals.filename, ret);
        exit_status = ret;
        goto cleanup;
    }

    /* If we required it to exec in self, then fail if this function returns. */
    if(!opal_restart_globals.forked) {
        opal_show_help("help-opal-restart.txt", "failed-to-exec", true,
                       expected_crs_comp,
                       opal_crs_base_selected_component.crs_version.mca_component_name);
        exit_status = ret;
        goto cleanup;
    }

    opal_output_verbose(10, opal_restart_globals.output,
                        "opal_restart: Restarted Child with PID = %d\n", child_pid);

    /***************
     * Cleanup
     ***************/
 cleanup:
    if (OPAL_SUCCESS != (ret = finalize())) {
        return ret;
    }

    if(NULL != snapshot )
        OBJ_DESTRUCT(snapshot);

    return exit_status;
}

static int initialize(int argc, char *argv[]) {
    int ret, exit_status = OPAL_SUCCESS;

    /*
     * Parse Command line arguments
     */
    if (OPAL_SUCCESS != (ret = parse_args(argc, argv))) {
        goto cleanup;
        exit_status = ret;
    }

    /*
     * Setup OPAL Output handle from the verbose argument
     */
    if( opal_restart_globals.verbose ) {
        opal_restart_globals.output = opal_output_open(NULL);
        opal_output_set_verbosity(opal_restart_globals.output, 10);
    } else {
        opal_restart_globals.output = 0; /* Default=STDOUT */
    }

    /* 
     * Turn off the selection of the CRS component,
     * we need to do that later
     */
    opal_setenv(mca_base_param_env_var("crs_base_do_not_select"),
                "1", /* turn off the selection */
                true, &environ);

    /*
     * Turn on 'cr' style fault tolerance
     */
    opal_setenv(mca_base_param_env_var("ft_enable"),
                "cr", /* Turn on cr fault tolerance  */
                true, &environ);

    /*
     * Initialize the OPAL layer
     */
    if (OPAL_SUCCESS != (ret = opal_init())) {
        exit_status = ret;
        goto cleanup;
    }


 cleanup:
    return exit_status;
}

static int finalize(void) {
    int ret;

    if (OPAL_SUCCESS != (ret = opal_finalize())) {
        return ret;
    }

    return OPAL_SUCCESS;
}

static int parse_args(int argc, char *argv[]) {
    int i, ret, len;
    opal_cmd_line_t cmd_line;
    char **app_env = NULL, **global_env = NULL;
    opal_restart_globals_t tmp = { false, NULL, false, false, false, NULL, 0 };

    opal_restart_globals = tmp;

    /* Parse the command line options */
    opal_cmd_line_create(&cmd_line, cmd_line_opts);
    
    mca_base_open();
    mca_base_cmd_line_setup(&cmd_line);
    ret = opal_cmd_line_parse(&cmd_line, true, argc, argv);
    
    /** 
     * Put all of the MCA arguments in the environment 
     */
    mca_base_cmd_line_process_args(&cmd_line, &app_env, &global_env);
    
    len = opal_argv_count(app_env);
    for(i = 0; i < len; ++i) {
        putenv(app_env[i]);
    }

    len = opal_argv_count(global_env);
    for(i = 0; i < len; ++i) {
        putenv(global_env[i]);
    }
    
    opal_setenv(mca_base_param_env_var("opal_cr_is_tool"),
                "1",
                true, &environ);
    /**
     * Now start parsing our specific arguments
     */
    if (OPAL_SUCCESS != ret || 
        opal_restart_globals.help ||
        1 >= argc) {
        char *args = NULL;
        args = opal_cmd_line_get_usage_msg(&cmd_line);
        opal_show_help("help-opal-restart.txt", "usage", true,
                       args);
        free(args);
        return OPAL_ERROR;
    }

    /* get the remaining bits */
    opal_cmd_line_get_tail(&cmd_line, &argc, &argv);
    if ( 1 > argc ) {
        char *args = NULL;
        args = opal_cmd_line_get_usage_msg(&cmd_line);
        opal_show_help("help-opal-restart.txt", "usage", true,
                       args);
        free(args);
        return OPAL_ERROR;
    }

    opal_restart_globals.filename = strdup(argv[0]);
    if ( NULL == opal_restart_globals.filename || 
         0 >= strlen(opal_restart_globals.filename) ) {
        opal_show_help("help-opal-restart.txt", "invalid_filename", true,
                       opal_restart_globals.filename);
        return OPAL_ERROR;
    }

    /* If we have arguments after the command, then assume they
     * need to be grouped together.
     * Useful in the 'mca crs self' instance.
     */
    if(argc > 1) {
        opal_restart_globals.filename = strdup(opal_argv_join(argv, ' '));
    }
    
    /*
     * Due to the special nature of the 'self' module, we need to know if that is the
     * requested module.
     * If it is then we don't need to do the 'snapshot reference' handling,
     * If it is NOT then we need to extract the requested CRS module from the 
     *   metadata file, and use that.
     * Should note that the 'self' module is the only one that needs to be specified
     * to opal_restart, all others are detected dynamicly.
     */
    if( opal_restart_globals.self_case ) {
        /* They are not required to explicitly use the '-mca crs self' convention,
         * so set the environment var for them */
        expected_crs_comp = strdup("self");
    }

    return OPAL_SUCCESS;
}

static int check_file(char *given_filename) {
    int exit_status = OPAL_SUCCESS;
    int ret;
    char * path_to_check = NULL;
    char **argv = NULL;

    if(NULL == given_filename) {
        exit_status = OPAL_ERROR;
        goto cleanup;
    }
    
    /* If this is the self case then we need to check that the application
     * exists
     */
    if(opal_restart_globals.self_case) {
        if( NULL == (argv = opal_argv_split(given_filename, ' ')) ) {
            exit_status = OPAL_ERROR;
            goto cleanup;
        }

        /* Extract just the application name */
        path_to_check = strdup(argv[0]);
    }
    /* Otherwise we are checking for the existance of the snapshot handle 
     * in the snapshot directory
     */
    else {
        path_to_check = opal_crs_base_get_snapshot_directory(given_filename);
    }

    /* Do the check */
    opal_output_verbose(10, opal_restart_globals.output,
                        "Checking for the existence of (%s)",
                        path_to_check);

    if (0 >  (ret = access(path_to_check, F_OK)) ) {
        exit_status = OPAL_ERROR;
        goto cleanup;
    }

 cleanup:
    if( NULL != path_to_check) 
        free(path_to_check);
    if( NULL != argv)
        opal_argv_free(argv);

    return exit_status;
}

static int post_env_vars(int prev_pid) {
    int ret, exit_status = OPAL_SUCCESS;
    char *command = NULL;
    char *proc_file = NULL;
    
    if( 0 > prev_pid ) {
        opal_output(opal_restart_globals.output,
                    "Invalid PID (%d)\n",
                    prev_pid);
        exit_status = OPAL_ERROR;
        goto cleanup;
    }

    /*
     * JJH: Hardcode /tmp to match opal/runtime/opal_cr.c in the application.
     * This is needed so we can pass the previous environment to the restarted 
     * application process.
     */
    asprintf(&proc_file, "/tmp/%s-%d", OPAL_CR_BASE_ENV_NAME, prev_pid);
    asprintf(&command, "env | grep OMPI_ > %s", proc_file);

    ret = system(command);
    if( 0 > ret) {
        exit_status = ret;
        goto cleanup;
    }

 cleanup:
    if( NULL != command)
        free(command);
    if( NULL != proc_file)
        free(proc_file);
    
    return exit_status;
}

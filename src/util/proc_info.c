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
 */

#include "orte_config.h"

#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "include/orte_constants.h"
#include "mca/base/base.h"
#include "mca/base/mca_base_param.h"

#include "util/proc_info.h"

orte_proc_info_t orte_process_info = {
    /*  .my_name =              */   NULL,
    /*  .vpid_start =           */   0,
    /*  .num_procs =            */   1,
    /*  .pid =                  */   0,
    /*  .seed =                 */   false,
    /*  .daemon =               */   false,
    /*  .singleton =            */   false,
    /*  .ns_replica_uri =       */   NULL,
    /*  .gpr_replica_uri =      */   NULL,
    /*  .ns_replica =           */   NULL,
    /*  .gpr_replica =          */   NULL,
    /*  .tmpdir_base =          */   NULL,
    /*  .top_session_dir =      */   NULL,
    /*  .universe_session_dir = */   NULL,
    /*  .job_session_dir =      */   NULL,
    /*  .proc_session_dir =     */   NULL,
    /*  .sock_stdin =           */   NULL,
    /*  .sock_stdout =          */   NULL,
    /*  .sock_stderr =          */   NULL};


int orte_proc_info(void)
{

    int id;
    
    /* all other params are set elsewhere */
    
    id = mca_base_param_register_int("seed", NULL, NULL, NULL, (int)false);
    mca_base_param_lookup_int(id, &(orte_process_info.seed));

    id = mca_base_param_register_string("gpr", "replica", "uri", NULL, NULL);
    mca_base_param_lookup_string(id, &(orte_process_info.gpr_replica_uri));

    id = mca_base_param_register_string("ns", "replica", "uri", NULL, NULL);
    mca_base_param_lookup_string(id, &(orte_process_info.ns_replica_uri));

    id = mca_base_param_register_string("tmpdir", "base", NULL, NULL, NULL);
    mca_base_param_lookup_string(id, &(orte_process_info.tmpdir_base));

    /* get the process id */
    orte_process_info.pid = getpid();

    return ORTE_SUCCESS;
}

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
#include <sys/bproc.h>

#include "include/orte_constants.h"
#include "include/types.h"
#include "class/ompi_list.h"
#include "util/output.h"
#include "mca/mca.h"
#include "mca/base/mca_base_param.h"
#include "dps/dps.h"
#include "svc_bproc.h"


/*
 * Local function prototypes.
 */

static void orte_svc_bproc_module_spawn(orte_jobid_t jobid);


/*
 * Struct of function pointers and all that to let us be initialized
 */
orte_svc_base_module_t orte_svc_bproc_module = {
    orte_svc_bproc_module_finalize
};

/**
 * Cleanup resources held by module.
 */

int orte_svc_bproc_module_finalize(void)
{
    return ORTE_SUCCESS;
}


/**
 * Receive a request.
 */

void orte_svc_bproc_module_recv(
    int status,
    orte_process_name_t* peer,
    orte_buffer_t* buffer,
    orte_rml_tag_t tag,
    void* cbdata)
{
    int32_t cmd;
    size_t cnt = 1;
    int rc = orte_dps.unpack(buffer, &cmd, &cnt, ORTE_INT32);
    if(rc < 0) {
        ompi_output(0,"orte_svc_bproc_module_recv: unpack command failed with status %d\n", rc);
        goto repost;
    }

    switch(cmd) {
        default:
            ompi_output(0, "orte_svc_bproc_module_recv: invalid command %d\n", cmd);
            break;
    }

repost:
    rc = orte_rml.recv_buffer_nb(
        ORTE_RML_NAME_ANY,
        ORTE_RML_TAG_BPROC_SVC,
        0,
        orte_svc_bproc_module_recv,
        NULL
    );
    if(rc != ORTE_SUCCESS) {
        ompi_output(0,"orte_svc_bproc_module_recv: orte_rml.recv_buffered_nb failed with status %d\n", rc);
    }

}
                                                                                                                                     

/**
 * 
 */

static void orte_svc_bproc_module_spawn(orte_jobid_t jobid)
{
}



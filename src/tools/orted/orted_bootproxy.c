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
/** @file **/

#include "orte_config.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "include/orte_constants.h"
#include "runtime/runtime.h"
#include "util/daemon_init.h"
#include "mca/pls/pls.h"
#include "mca/pls/base/base.h"
#include "orted.h"


static int orte_daemon_context(void)
{
    return ORTE_SUCCESS;
}


int orte_daemon_bootproxy(void)
{
    orte_pls_base_module_t* pls;
    char path[PATH_MAX];
    int rc;
    int fd;

    /* read context from stdin */
    rc = orte_daemon_context();
    if(ORTE_SUCCESS != rc)
        return rc;

    /* lookup launcher */
    pls = orte_pls_base_select("fork");
    if(NULL == pls) {
        return ORTE_ERR_NOT_AVAILABLE;
    }

    /* setup stdin/stdout/stderr */
    fd = open("/dev/null", O_RDONLY);
    if(fd != 0) {
        dup2(fd, 0); 
        close(fd);
    }
    sprintf(path, "%s/orted-%s-%d", 
        orte_universe_info.path, 
        orte_universe_info.host, 
        orte_universe_info.bootproxy);

    fd = open(path, O_RDWR|O_CREAT|O_TRUNC, 0666);
    if(fd >= 0) {
        dup2(fd, 1);
        dup2(fd, 2);
        if(fd > 2) {
           close(fd);
        }
    }

    /* disconnect from controlling terminal */
    rc = orte_daemon_init(orte_universe_info.path);
    if(ORTE_SUCCESS != rc)
        return rc;

    /* launch the requested procs */
    rc = pls->launch(orte_universe_info.bootproxy);
    return rc;
}


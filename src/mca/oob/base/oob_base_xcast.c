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

#include "ompi_config.h"
#include <string.h>

#include "include/constants.h"
#include "util/proc_info.h"
#include "mca/oob/oob.h"
#include "mca/oob/base/base.h"
#include "mca/pcmclient/pcmclient.h"
#include "mca/pcmclient/base/base.h"
#include "mca/ns/ns.h"
#include "runtime/runtime.h"


/**
 *  A "broadcast-like" function over the specified set of peers.
 *  @param  root    The process acting as the root of the broadcast.
 *  @param  peers   The list of processes receiving the broadcast (excluding root).
 *  @param  buffer  The data to broadcast - only significant at root.
 *  @param  cbfunc  Callback function on receipt of data - not significant at root.
 *
 *  Note that the callback function is provided so that the data can be
 *  received and interpreted by the application prior to the broadcast
 *  continuing to forward data along the distribution tree.
 */
                                                                                                  
int mca_oob_xcast(
    orte_process_name_t* root,
    ompi_list_t* peers,
    ompi_buffer_t buffer,
    mca_oob_callback_packed_fn_t cbfunc)
{
    orte_name_services_namelist_t *ptr;
    int rc;
    int tag = MCA_OOB_TAG_XCAST;
    ompi_buffer_t rbuf;
    int cmpval;
        
    /* check to see if I am the root process name */
    if (ORTE_SUCCESS != (rc = orte_name_services.compare(&cmpval, ORTE_NS_CMP_ALL, root, ompi_rte_get_self()))) {
        return rc;
    }
    if(NULL != root && 0 == cmpval) {
        for (ptr = (orte_name_services_namelist_t*)ompi_list_get_first(peers);
	     ptr != (orte_name_services_namelist_t*)ompi_list_get_end(peers);
	     ptr = (orte_name_services_namelist_t*)ompi_list_get_next(ptr)) {
            rc = mca_oob_send_packed(ptr->name, buffer, tag, 0);
            if(rc < 0) {
                return rc;
            }
        }
    } else {
        rc = mca_oob_recv_packed(MCA_OOB_NAME_ANY, &rbuf, &tag);
        if(rc < 0) {
            return rc;
        }
        if(cbfunc != NULL)
            cbfunc(rc, root, rbuf, tag, NULL);
    }
    return OMPI_SUCCESS;
}



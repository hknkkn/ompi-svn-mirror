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
 */
/** @file:
 *
 * The Open MPI General Purpose Registry - Replica component
 *
 */

/*
 * includes
 */
#include "orte_config.h"

#include "util/output.h"
#include "util/proc_info.h"

#include "mca/ns/ns_types.h"
#include "mca/rml/rml.h"

#include "gpr_replica_comm.h"

/* 
 * handle message from proxies
 */

void orte_gpr_replica_recv(int status, orte_process_name_t* sender,
			  orte_buffer_t *buffer, int tag, void* cbdata)
{
    orte_buffer_t *answer;
    size_t buf_size=0;
    int rc;

    if (orte_gpr_replica_debug) {
	   ompi_output(0, "[%d,%d,%d] gpr replica: received message from [%d,%d,%d]",
			    ORTE_NAME_ARGS(*(orte_process_info.my_name)), ORTE_NAME_ARGS(*sender));
    }

    answer = OBJ_NEW(orte_buffer_t);
    if (NULL == answer) {
        return;
    }
    
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_process_command_buffer(buffer, sender,
								 answer))) {

	orte_buffer_size(answer, &buf_size);

	if ((compound_cmd_detected && return_requested) ||
	    (!compound_cmd_detected && 0 < buf_size)) { /* must be some data or status codes to return */
		if (orte_gpr_replica_debug) {
			ompi_output(0, "[%d,%d,%d] gpr replica: sending response of length %d to [%d,%d,%d]",
						ORTE_NAME_ARGS(*(orte_process_info.my_name)), (int)buf_size, ORTE_NAME_ARGS(*sender));
		}
	    if (0 > orte_rml.send_packed(sender, answer, tag, 0)) {
		/* RHC -- not sure what to do if the return send fails */
	    }
	}

	orte_buffer_free(answer);
	if (orte_gpr_replica_debug) {
	    ompi_output(0, "gpr replica: msg processing complete - processing callbacks");
	}

	orte_gpr_replica_process_callbacks();
    }

    /* reissue the non-blocking receive */
    orte_rml.recv_packed_nb(MCA_OOB_NAME_ANY, MCA_OOB_TAG_GPR, 0, orte_gpr_replica_recv, NULL);
}

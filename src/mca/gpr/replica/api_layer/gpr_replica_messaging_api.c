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
/** @file 
 */

#include "orte_config.h"

#include "dps/dps.h"

#include "util/output.h"
#include "util/proc_info.h"

#include "mca/rml/rml_types.h"

#include "gpr_replica_api.h"

int orte_gpr_replica_get_startup_msg(orte_jobid_t jobid,
                                    orte_buffer_t **msg,
                                    size_t *cnt,
                                    orte_process_name_t **procs)
{
    int rc;
    
    if (orte_gpr_replica_debug) {
      ompi_output(0, "[%d,%d,%d] entered get_startup_msg",
            ORTE_NAME_ARGS(*(orte_process_info.my_name)));
    }

    *cnt = 0;
    if (NULL != *procs) {
        return ORTE_ERR_BAD_PARAM;
    }
    *procs = NULL;
    
    if (NULL != *msg) {
        OBJ_RELEASE(*msg);
    }
    
    *msg = OBJ_NEW(orte_buffer_t);
    if (NULL == *msg) {
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    OMPI_THREAD_LOCK(&orte_gpr_replica_mutex);

    rc = orte_gpr_replica_get_startup_msg_fn(jobid, msg, cnt, procs);

    OMPI_THREAD_UNLOCK(&orte_gpr_replica_mutex);

    return rc;
}

void
orte_gpr_replica_decode_startup_msg(int status, orte_process_name_t *peer,
                                    orte_buffer_t* msg,
                                    orte_rml_tag_t tag, void *cbdata)
{
#if 0
    char *segment;
    orte_gpr_notify_message_t *notify_msg;
    orte_gpr_value_t *data_value;
    int32_t num_objects, i;
    size_t buf_size;

    while (0 < ompi_unpack_string(msg, &segment)) {
  if (ompi_rte_debug_flag) {
     ompi_output(0, "[%d,%d,%d] decoding msg for segment %s",
           ORTE_NAME_ARGS(*ompi_rte_get_self()), segment);
    }

 ompi_unpack(msg, &num_objects, 1, OMPI_INT32);  /* unpack #data objects */

    if (ompi_rte_debug_flag) {
     ompi_output(0, "\twith %d objects", num_objects);
  }

 if (0 < num_objects) {
     notify_msg = OBJ_NEW(orte_gpr_notify_message_t);
       notify_msg->segment = strdup(segment);

        for (i=0; i < num_objects; i++) {

     data_value = OBJ_NEW(orte_gpr_value_t);
        ompi_unpack(msg, &data_obj_size, 1, MCA_GPR_OOB_PACK_OBJECT_SIZE);
     data_object = (orte_gpr_object_t)malloc(data_obj_size);
        ompi_unpack(msg, data_object, data_obj_size, OMPI_BYTE);
       data_value->object = data_object;
      data_value->object_size = data_obj_size;

      ompi_list_append(&notify_msg->data, &data_value->item);
        }

     if (ompi_rte_debug_flag) {
     ompi_output(0, "[%d,%d,%d] delivering msg for segment %s with %d data objects",
                ORTE_NAME_ARGS(*ompi_rte_get_self()), segment, (int)num_objects);
      }

     orte_gpr.deliver_notify_msg(OMPI_REGISTRY_NOTIFY_ON_STARTUP, notify_msg);
  }

 free(segment);
    }
#endif
}


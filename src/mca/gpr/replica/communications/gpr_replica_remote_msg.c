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

#include "gpr_replica_comm.h"

int orte_gpr_replica_remote_notify(orte_process_name_t *recipient, int recipient_tag,
                 orte_gpr_notify_message_t *message)
{
#if 0
    ompi_buffer_t msg;
    mca_gpr_cmd_flag_t command;
    int32_t num_items;
    uint i;
    ompi_registry_value_t *regval;
    char **tokptr;
    int recv_tag;

    if (mca_gpr_replica_debug) {
  ompi_output(0, "sending trigger message");
    }

    command = MCA_GPR_NOTIFY_CMD;
    recv_tag = MCA_OOB_TAG_GPR_NOTIFY;

    if (OMPI_SUCCESS != ompi_buffer_init(&msg, 0)) {
   return;
    }

    if (OMPI_SUCCESS != ompi_pack(msg, &command, 1, MCA_GPR_OOB_PACK_CMD)) {
    return;
    }

    if (0 > ompi_pack_string(msg, message->segment)) {
  return;
    }

    i = (int32_t)message->owning_job;
    if (OMPI_SUCCESS != ompi_pack(msg, &i, 1, OMPI_INT32)) {
  return;
    }

    i = (int32_t)recipient_tag;
    if (OMPI_SUCCESS != ompi_pack(msg, &i, 1, OMPI_INT32)) {
    return;
    }

    if (OMPI_SUCCESS != ompi_pack(msg, &message->trig_action, 1, MCA_GPR_OOB_PACK_ACTION)) {
    return;
    }

    if (OMPI_SUCCESS != ompi_pack(msg, &message->trig_synchro, 1, MCA_GPR_OOB_PACK_SYNCHRO_MODE)) {
 return;
    }

    
    num_items = (int32_t)ompi_list_get_size(&message->data);
    if (OMPI_SUCCESS != ompi_pack(msg, &num_items, 1, OMPI_INT32)) {
  return;
    }

    if (0 < num_items) { /* don't send anything else back if the list is empty */
   while (NULL != (regval = (ompi_registry_value_t*)ompi_list_remove_first(&message->data))) {
        if (OMPI_SUCCESS != ompi_pack(msg, &regval->object_size, 1, MCA_GPR_OOB_PACK_OBJECT_SIZE)) {
       return;
        }
      if (OMPI_SUCCESS != ompi_pack(msg, regval->object, regval->object_size, OMPI_BYTE)) {
      return;
        }
      OBJ_RELEASE(regval);
   }
    }
    if (OMPI_SUCCESS != ompi_pack(msg, &message->num_tokens, 1, OMPI_INT32)) {
 return;
    }

    for (i=0, tokptr=message->tokens; i < (uint)message->num_tokens; i++, tokptr++) {
   if (OMPI_SUCCESS != ompi_pack_string(msg, *tokptr)) {
      return;
    }
    }

    if (0 > mca_oob_send_packed(recipient, msg, recv_tag, 0)) {
   return;
    }

    ompi_buffer_free(msg);

    OBJ_RELEASE(message);
#endif
}

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

#include "ompi_config.h"

#include "util/output.h"

#include "runtime/runtime.h"
#include "mca/ns/ns_types.h"
#include "mca/gpr/gpr.h"
#include "mca/rml/rml.h"

/*
 * Local functions
 */
void
ompi_rte_decode_startup_msg(int status, orte_process_name_t *peer,
			    orte_buffer_t* msg, orte_rml_tag_t tag, void *cbdata);


/*
 * Main functions
 */
int ompi_rte_wait_startup_msg(void)
{

    return orte_rml.xcast(NULL, NULL, 0, NULL, ompi_rte_decode_startup_msg);
}


/*
 * Unpack the startup message.
 * When a startup message is received, it contains data objects from
 * several pre-defined registry segments. This includes OOB contact info,
 * PTL contact info, and other things. Each of these subsystems has a
 * callback function that is used to receive updates from the registry
 * This function deconstructs the message and builds a notify
 * message for each segment, and then passes that message to the appropriate
 * callback function as if it came directly from the registry.
 */

void
ompi_rte_decode_startup_msg(int status, orte_process_name_t *peer,
			    orte_buffer_t* msg, orte_rml_tag_t tag, void *cbdata)
{
#if 0
    char *segment;
    orte_gpr_notify_message_t *notify_msg;
    orte_gpr_value_t *data_value;
    orte_gpr_object_t *data_object;
    orte_gpr_object_size_t data_obj_size;
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

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
#include "include/orte_constants.h"
#include "include/constants.h"

#include "util/output.h"

#include "mca/gpr/base/base.h"



/*
 * The following file was created by configure.  It contains extern
 * statements and the definition of an array of pointers to each
 * component's public mca_base_module_t struct.
 */

#include "mca/gpr/base/static-components.h"

/*
 * globals
 */

/* constructor - used to initialize state of keyval instance */
static void orte_gpr_keyval_construct(orte_gpr_keyval_t* keyval)
{
    keyval->key = NULL;
    keyval->type = 0;
    keyval->value.i32 = 0;
}

/* destructor - used to free any resources held by instance */
static void orte_gpr_keyval_destructor(orte_gpr_keyval_t* keyval)
{
    orte_byte_object_t *byteptr;
    
    if (NULL != keyval->key) {
        free(keyval->key);
    }
    if (ORTE_BYTE_OBJECT == keyval->type) {
        byteptr = &(keyval->value.byteobject);
        if (NULL != byteptr->bytes) {
            free(byteptr->bytes);
        }
    }
}

/* define instance of ompi_class_t */
OBJ_CLASS_INSTANCE(
    orte_gpr_keyval_t,              /* type name */
    ompi_object_t,                  /* parent "class" name */
    orte_gpr_keyval_construct,      /* constructor */
    orte_gpr_keyval_destructor);    /* destructor */



/* constructor - used to initialize state of registry value instance */
static void orte_gpr_value_construct(orte_gpr_value_t* reg_val)
{
    reg_val->cnt = 0;
    reg_val->keyvals = NULL;
    reg_val->num_tokens = 0;
    reg_val->tokens = 0;
}

/* destructor - used to free any resources held by instance */
static void orte_gpr_value_destructor(orte_gpr_value_t* reg_val)
{
    char **tokens;
    uint32_t i;
    
    if (0 < reg_val->cnt && NULL != reg_val->keyvals) {
	   free(reg_val->keyvals);
    }
    
    if (0 < reg_val->num_tokens && NULL != reg_val->tokens) {
        tokens = reg_val->tokens;
        for (i=0; i < reg_val->num_tokens; i++) {
            free(*tokens);
            tokens++;
        }
        free(tokens);
    }
}

/* define instance of ompi_class_t */
OBJ_CLASS_INSTANCE(
		   orte_gpr_value_t,  /* type name */
		   ompi_object_t, /* parent "class" name */
		   orte_gpr_value_construct, /* constructor */
		   orte_gpr_value_destructor); /* destructor */


/* constructor - used to initialize state of test results instance */
static void orte_gpr_internal_test_results_construct(orte_gpr_internal_test_results_t* results)
{
    results->test = NULL;
    results->message = NULL;
}

/* destructor - used to free any resources held by instance */
static void orte_gpr_internal_test_results_destructor(orte_gpr_internal_test_results_t* results)
{
    if (NULL != results->test) {
	free(results->test);
    }
    if (NULL != results->message) {
	free(results->message);
    }
}

/* define instance of ompi_class_t */
OBJ_CLASS_INSTANCE(
		   orte_gpr_internal_test_results_t,            /* type name */
		   ompi_list_item_t,                                 /* parent "class" name */
		   orte_gpr_internal_test_results_construct,    /* constructor */
		   orte_gpr_internal_test_results_destructor);  /* destructor */


/* constructor - used to initialize notify message instance */
static void orte_gpr_notify_message_construct(orte_gpr_notify_message_t* msg)
{
    msg->segment = NULL;
    msg->cnt = 0;
    msg->values = NULL;
    msg->cmd = 0;
    msg->flag.cmd_return = 0;
}

/* destructor - used to free any resources held by instance */
static void orte_gpr_notify_message_destructor(orte_gpr_notify_message_t* msg)
{

    if (NULL != msg->segment) {
	   free(msg->segment);
    }

    if (0 < msg->cnt && NULL != msg->values) {
	   free(msg->values);
    }
    
}

/* define instance of ompi_class_t */
OBJ_CLASS_INSTANCE(
		   orte_gpr_notify_message_t,            /* type name */
		   ompi_object_t,                             /* parent "class" name */
		   orte_gpr_notify_message_construct,    /* constructor */
		   orte_gpr_notify_message_destructor);  /* destructor */


/*
 * Global variables
 */
int orte_gpr_base_output = -1;
orte_gpr_base_module_t orte_gpr;
bool orte_gpr_base_selected = false;
ompi_list_t orte_gpr_base_components_available;
mca_gpr_base_component_t orte_gpr_base_selected_component;
ompi_mutex_t orte_gpr_mutex;


/**
 * Function for finding and opening either all MCA components, or the one
 * that was specifically requested via a MCA parameter.
 */
int orte_gpr_base_open(void)
{

  /* Open up all available components */

  if (OMPI_SUCCESS != 
      mca_base_components_open("orte_gpr", 0, mca_gpr_base_static_components, 
                            &orte_gpr_base_components_available)) {
    return ORTE_ERROR;
  }

  /* setup output for debug messages */
  if (!ompi_output_init) {  /* can't open output */
      return ORTE_ERROR;
  }

  orte_gpr_base_output = ompi_output_open(NULL);

  /* All done */

  return ORTE_SUCCESS;
}

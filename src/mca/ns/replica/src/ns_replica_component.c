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
 * The Open MPI Name Server
 *
 * The Open MPI Name Server provides unique name ranges for processes
 * within the universe. Each universe will have one name server
 * running within the seed daemon.  This is done to prevent the
 * inadvertent duplication of names.
 */

/*
 * includes
 */
#include "orte_config.h"

#include "include/orte_constants.h"
#include "include/orte_types.h"

#include "threads/mutex.h"
#include "util/proc_info.h"
#include "util/output.h"

#include "mca/mca.h"
#include "mca/base/mca_base_param.h"

#include "mca/oob/base/base.h"

#include "ns_replica.h"


/*
 * Struct of function pointers that need to be initialized
 */
OMPI_COMP_EXPORT mca_ns_base_component_t mca_ns_replica_component = {
  {
    MCA_NS_BASE_VERSION_1_0_0,

    "replica", /* MCA module name */
    1,  /* MCA module major version */
    0,  /* MCA module minor version */
    0,  /* MCA module release version */
    orte_ns_replica_open,  /* module open */
    orte_ns_replica_close /* module close */
  },
  {
    false /* checkpoint / restart */
  },
  orte_ns_replica_init,    /* module init */
  orte_ns_replica_finalize /* module shutdown */
};

/*
 * setup the function pointers for the module
 */
static mca_ns_base_module_t orte_ns_replica = {
    orte_ns_replica_create_cellid,
    orte_ns_base_assign_cellid_to_process,
    orte_ns_replica_create_jobid,
    orte_ns_base_create_process_name,
    orte_ns_base_copy_process_name,
    orte_ns_base_convert_string_to_process_name,
    orte_ns_replica_reserve_range,
    orte_ns_base_free_name,
    orte_ns_base_get_proc_name_string,
    orte_ns_base_get_vpid_string,
    orte_ns_base_convert_vpid_to_string,
    orte_ns_base_convert_string_to_vpid,
    orte_ns_base_get_jobid_string,
    orte_ns_base_convert_jobid_to_string,
    orte_ns_base_convert_string_to_jobid,
    orte_ns_base_get_cellid_string,
    orte_ns_base_convert_cellid_to_string,
    orte_ns_base_convert_string_to_cellid,
    orte_ns_base_get_vpid,
    orte_ns_base_get_jobid,
    orte_ns_base_get_cellid,
    orte_ns_base_compare,
    orte_ns_base_derive_vpid,
    orte_ns_replica_assign_rml_tag
};

/*
 * Whether or not we allowed this component to be selected
 */
static bool initialized = false;


/* constructor - used to initialize state of name_tracker instance */
static void orte_ns_replica_tracker_construct(orte_ns_replica_name_tracker_t* name_tracker)
{
    name_tracker->job = 0;
    name_tracker->last_used_vpid = 0;
}

/* destructor - used to free any resources held by instance */
static void orte_ns_replica_tracker_destructor(orte_ns_replica_name_tracker_t* name_tracker)
{
}

/* define instance of ompi_class_t */
OBJ_CLASS_INSTANCE(
		   orte_ns_replica_name_tracker_t,  /* type name */
		   ompi_list_item_t, /* parent "class" name */
		   orte_ns_replica_tracker_construct, /* constructor */
		   orte_ns_replica_tracker_destructor); /* destructor */

/* constructor - used to initialize state of taglist instance */
static void orte_ns_replica_tagitem_construct(orte_ns_replica_tagitem_t* tagitem)
{
    tagitem->tag = ORTE_RML_TAG_MAX;
    tagitem->name = NULL;
}

/* destructor - used to free any resources held by instance */
static void orte_ns_replica_tagitem_destructor(orte_ns_replica_tagitem_t* tagitem)
{
    if (NULL != tagitem->name) {
       free(tagitem->name);
    }
}

/* define instance of ompi_class_t */
OBJ_CLASS_INSTANCE(
        orte_ns_replica_tagitem_t,  /* type name */
        ompi_list_item_t, /* parent "class" name */
        orte_ns_replica_tagitem_construct, /* constructor */
        orte_ns_replica_tagitem_destructor); /* destructor */

/*
 * globals needed within replica component
 */
orte_cellid_t orte_ns_replica_next_cellid;
orte_jobid_t orte_ns_replica_next_jobid;
ompi_list_t orte_ns_replica_name_tracker;
orte_rml_tag_t orte_ns_replica_next_rml_tag;
ompi_list_t orte_ns_replica_taglist;
int orte_ns_replica_debug;
ompi_mutex_t orte_ns_replica_mutex;

/*
 * don't really need this function - could just put NULL in the above structure
 * Just holding the place in case we decide there is something we need to do
 */
int orte_ns_replica_open(void)
{
    int id;

    id = mca_base_param_register_int("ns", "replica", "debug", NULL, 0);
    mca_base_param_lookup_int(id, &orte_ns_replica_debug);

    return ORTE_SUCCESS;
}

/*
 * ditto for this one
 */
int orte_ns_replica_close(void)
{
    return ORTE_SUCCESS;
}

mca_ns_base_module_t* orte_ns_replica_init(bool *allow_multi_user_threads, bool *have_hidden_threads, int *priority)
{
    /* If we are to host a replica, then we want to be selected, so do all the
       setup and return the module */

    if (NULL == ompi_process_info.ns_replica) {

      int rc;
      orte_ns_replica_next_cellid = 0;
      orte_ns_replica_next_jobid = 1;  /* jobid 0 reserved for universe */

      /* Return a module (choose an arbitrary, positive priority --
         it's only relevant compared to other ns components).  If
         we're not the seed, then we don't want to be selected, so
         return NULL. */

      *priority = 50;

      /* We allow multi user threads but don't have any hidden threads */

      *allow_multi_user_threads = true;
      *have_hidden_threads = false;

      /* initialize the name tracker */

      OBJ_CONSTRUCT(&orte_ns_replica_name_tracker, ompi_list_t);

      /* initialize the taglist */

      OBJ_CONSTRUCT(&orte_ns_replica_taglist, ompi_list_t);
      orte_ns_replica_next_rml_tag = ORTE_OOB_TAG_START_LIST;

    /* setup the thread lock */
    OBJ_CONSTRUCT(&orte_ns_replica_mutex, ompi_mutex_t);
    
     /* Return the module */

      initialized = true;

      /* issue non-blocking receive for call_back function */
      rc = mca_oob_recv_packed_nb(MCA_OOB_NAME_ANY, MCA_OOB_TAG_NS, 0, orte_ns_replica_recv, NULL);
      if(rc != OMPI_SUCCESS && rc != OMPI_ERR_NOT_IMPLEMENTED) {
          ompi_output(0, "orte_ns_replica_init: unable to post non-blocking recv\n");
          return NULL;
      }

      return &orte_ns_replica;
    } else {
      return NULL;
    }
}

/*
 * finalize routine
 */
int orte_ns_replica_finalize(void)
{
    orte_ns_replica_tagitem_t *tagitem;
    
    if (orte_ns_replica_debug) {
	   ompi_output(0, "finalizing ns replica");
    }

  /* free all tracking storage, but only if this component was initialized */

    if (initialized) {
/*     OBJ_DESTRUCT(&orte_ns_replica_name_tracker); */
        while (NULL != (tagitem = (orte_ns_replica_tagitem_t*)ompi_list_remove_first(&orte_ns_replica_taglist))) {
            OBJ_RELEASE(tagitem);
        }
        OBJ_DESTRUCT(&orte_ns_replica_taglist);
        OBJ_DESTRUCT(&orte_ns_replica_mutex);

        initialized = false;
    }

  /* All done */

    mca_oob_recv_cancel(MCA_OOB_NAME_ANY, MCA_OOB_TAG_NS);
    return ORTE_SUCCESS;
}


/* 
 * handle message from proxies
 */

void orte_ns_replica_recv(int status, orte_process_name_t* sender,
			             ompi_buffer_t buffer, int tag,
			             void* cbdata)
{
    ompi_buffer_t answer, error_answer;
    orte_ns_cmd_flag_t command;
    orte_cellid_t cell;
    orte_jobid_t job;
    orte_vpid_t startvpid, range;
    char *tagname;
    orte_rml_tag_t oob_tag;
    int32_t return_code=ORTE_SUCCESS;

    if (ORTE_SUCCESS != ompi_unpack(buffer, (void*)&command, 1, ORTE_NS_OOB_PACK_CMD)) {
       return_code = ORTE_ERR_BAD_PARAM;
	   goto RETURN_ERROR;
    }

    if (OMPI_SUCCESS != ompi_buffer_init(&answer, 0)) {
        return_code = ORTE_ERR_OUT_OF_RESOURCE;
        goto RETURN_ERROR;
    }

    if (ORTE_NS_CREATE_CELLID_CMD == command) {   /* got a command to create a cellid */
	   if (OMPI_SUCCESS != (return_code = ompi_pack(answer, (void*)&command, 1, ORTE_NS_OOB_PACK_CMD))) {
	       goto RETURN_ERROR;
	   }
     
	   if (ORTE_SUCCESS != (return_code = orte_ns_replica_create_cellid(&cell))) {
            goto RETURN_ERROR;
       }
       
	   if (OMPI_SUCCESS != (return_code = ompi_pack(answer, (void*)&cell, 1, ORTE_NS_OOB_PACK_CELLID))) {
	       goto RETURN_ERROR;
	   }
	   if (0 > mca_oob_send_packed(sender, answer, tag, 0)) {
	    /* RHC -- not sure what to do if the return send fails */
	   }
     
    } else if (ORTE_NS_CREATE_JOBID_CMD == command) {   /* got command to create jobid */
	   if (OMPI_SUCCESS != (return_code = ompi_pack(answer, (void*)&command, 1, ORTE_NS_OOB_PACK_CMD))) {
	       goto RETURN_ERROR;
	   }
     
	   if (ORTE_SUCCESS != (return_code = orte_ns_replica_create_jobid(&job))) {
            goto RETURN_ERROR;
       }
       
	   if (OMPI_SUCCESS != (return_code = ompi_pack(answer, (void*)&job, 1, ORTE_NS_OOB_PACK_JOBID))) {
	       goto RETURN_ERROR;
	   }
     
	   if (0 > mca_oob_send_packed(sender, answer, tag, 0)) {
	       /* RHC -- not sure what to do if the return send fails */
	   }
     
    } else if (ORTE_NS_RESERVE_RANGE_CMD == command) {  /* got command to reserve vpid range */
	   if (OMPI_SUCCESS != (return_code = ompi_unpack(buffer, (void*)&job, 1, ORTE_NS_OOB_PACK_JOBID))) {
	       goto RETURN_ERROR;
	   }

	   if (OMPI_SUCCESS != (return_code = ompi_unpack(buffer, (void*)&range, 1, ORTE_NS_OOB_PACK_VPID))) {
	       goto RETURN_ERROR;
	   }

	   if (ORTE_SUCCESS != (return_code = orte_ns_replica_reserve_range(job, range, &startvpid))) {
            goto RETURN_ERROR;
       }
       
	   if (OMPI_SUCCESS != (return_code = ompi_pack(answer, (void*)&command, 1, ORTE_NS_OOB_PACK_CMD))) {
	       goto RETURN_ERROR;
	   }

	   if (OMPI_SUCCESS != (return_code = ompi_pack(answer, (void*)&startvpid, 1, ORTE_NS_OOB_PACK_VPID))) {
	       goto RETURN_ERROR;
	   }
     
	   if (0 > mca_oob_send_packed(sender, answer, tag, 0)) {
	       /* RHC -- not sure what to do if the return send fails */
	   }

        
    } else if (ORTE_NS_ASSIGN_OOB_TAG_CMD == command) {  /* got command to assign an OOB tag */
       if (0 > ompi_unpack_string(buffer, &tagname)) {
         return_code = ORTE_ERR_UNPACK_FAILURE;
         goto RETURN_ERROR;
      }

       if (0 == strncmp(tagname, "NULL", 4)) {
            if (ORTE_SUCCESS != (return_code = orte_ns_replica_assign_rml_tag(&oob_tag, NULL))) {
                goto RETURN_ERROR;
            }
       } else {
            if (ORTE_SUCCESS != (return_code = orte_ns_replica_assign_rml_tag(&oob_tag, tagname))) {
                goto RETURN_ERROR;
            }
       }
       
      if (OMPI_SUCCESS != (return_code = ompi_pack(answer, (void*)&command, 1, ORTE_NS_OOB_PACK_CMD))) {
         goto RETURN_ERROR;
      }

      if (OMPI_SUCCESS != (return_code = ompi_pack(answer, (void*)&oob_tag, 1, ORTE_NS_OOB_PACK_OOB_TAG))) {
          goto RETURN_ERROR;
      }
     
      if (0 > mca_oob_send_packed(sender, answer, tag, 0)) {
         /* RHC -- not sure what to do if the return send fails */
      }

        
    } else {  /* got an unrecognized command */
    RETURN_ERROR:
        ompi_buffer_free(answer);
	    ompi_buffer_init(&error_answer, 0);
	    ompi_pack(error_answer, (void*)&command, 1, ORTE_NS_OOB_PACK_CMD);
        ompi_pack(error_answer, (void*)&return_code, 1, ORTE_INT32);
	    mca_oob_send_packed(sender, error_answer, tag, 0);
    }

    /* reissue the non-blocking receive */
    mca_oob_recv_packed_nb(MCA_OOB_NAME_ANY, MCA_OOB_TAG_NS, 0, orte_ns_replica_recv, NULL);
}


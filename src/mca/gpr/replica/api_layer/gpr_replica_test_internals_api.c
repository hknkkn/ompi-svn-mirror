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
/** @file:
 *
 * The Open MPI general purpose registry - support functions.
 *
 */

/*
 * includes
 */

#include "orte_config.h"

#include "util/output.h"

#include "mca/ns/ns_types.h"

#include "gpr_replica_api.h"


int orte_gpr_replica_test_internals(int level, ompi_list_t **test_results)
{
    orte_gpr_internal_test_results_t *result=NULL;
    char name[30], name2[30];
    char *name3[30];
    int i, j, num_keys, rc;
    orte_gpr_replica_itag_t segtag, itag, *itags=NULL;
    orte_gpr_replica_segment_t *seg=NULL;
    orte_gpr_replica_dict_t dict_entry;
    bool success=false;

    *test_results = OBJ_NEW(ompi_list_t);

    if (orte_gpr_replica_compound_cmd_mode) {
	   return orte_gpr_base_pack_test_internals(orte_gpr_replica_compound_cmd, level);
    }

    ompi_output(0, "building test segments");
    /* create several test segments */
    success = true;
    result = OBJ_NEW(orte_gpr_internal_test_results_t);
    result->test = strdup("test-define-job-segment");
    for (i=0; i<5 && success; i++) {
	   if (ORTE_SUCCESS != (rc = orte_gpr_replica_define_job_segment((orte_jobid_t)i, (int)1000*i))) {
	       success = false;
	   }
    }
    if (success) {
	    result->message = strdup("success");
    } else {
	    result->message = strdup("failed");
    }
    ompi_list_append(*test_results, &result->item);

    ompi_output(0, "testing get itag for segment ");
    /* check ability to get itag for a segment */
    success = true;
    result = OBJ_NEW(orte_gpr_internal_test_results_t);
    result->test = strdup("test-get-seg-itag");
    for (i=0; i<5 && success; i++) {
	   sprintf(name, "test-def-seg%d", i);
	   if (ORTE_SUCCESS != (rc = orte_gpr_replica_dict_lookup(&dict_entry, NULL, name))) {
            result->exit_code = rc;
	        success = false;
	   }
    }
    if (success) {
	result->message = strdup("success");
    } else {
	result->message = strdup("failed");
    }
    ompi_list_append(*test_results, &result->item);

#if 0
    ompi_output(0, "testing define itag");
    /* check that define itag protects uniqueness */
    success = true;
    result = OBJ_NEW(orte_gpr_internal_test_results_t);
    result->test = strdup("test-define-itag-uniqueness");
    for (i=0; i<5 && success; i++) {
	    sprintf(name, "test-def-seg%d", i);
        sprintf(name2, "dummy_key");
	    if (ORTE_SUCCESS != (rc = orte_gpr_replica_define_itag(&segtag, name, name2))) {
            result->exit_code = rc;
            success = false;
        }
        if (ORTE_SUCCESS == (rc = orte_gpr_replica_define_itag(&itag, name, name2))) {
            result->exit_code = rc;
            success = false;
      }
    }
    if (success) {
	   result->message = strdup("success");
    } else {
	   result->message = strdup("failed");
    }
    ompi_list_append(*test_results, &result->item);

    ompi_output(0, "testing find segment");
    /* check the ability to find a segment */
    i = 2;
    sprintf(name, "test-def-seg%d", i);
    result = OBJ_NEW(orte_gpr_internal_test_results_t);
    result->test = strdup("test-find-seg");
    seg = orte_gpr_replica_find_seg(false, name, test_jobid);
    if (NULL == seg) {
	asprintf(&result->message, "test failed with NULL returned: %s", name);
    } else {  /* locate key and check it */
	segkey = orte_gpr_replica_get_key(NULL, name);
	if (segkey == seg->key) {
	    result->message = strdup("success");
	} else {
	    asprintf(&result->message, "test failed: key %d seg %d", segkey, seg->key);
	}
    }
    ompi_list_append(test_results, &result->item);

    ompi_output(0, "testing get key within segment");
    /* check ability to retrieve key within a segment */
    success = true;
    result = OBJ_NEW(orte_gpr_internal_test_results_t);
    result->test = strdup("test-get-key-segment");
    for (i=0; i<5 && success; i++) {
	sprintf(name, "test-def-seg%d", i);
	seg = orte_gpr_replica_find_seg(false, name, test_jobid);
	for (j=0; j<10 && success; j++) {
 	    sprintf(name2, "test-key%d", j);
	    key = orte_gpr_replica_get_key(seg, name2);
	    if (MCA_GPR_REPLICA_KEY_MAX == key) { /* got an error */
		success = false;
	    }
	}
    }
    if (success) {
	result->message = strdup("success");
    } else {
	result->message = strdup("failed");
    }
    ompi_list_append(test_results, &result->item);


    ompi_output(0, "testing get key list");
    /* check ability to get key list */
    success = true;
    result = OBJ_NEW(orte_gpr_internal_test_results_t);
    result->test = strdup("test-get-keylist");
    for (i=0; i<5 && success; i++) {
	sprintf(name, "test-def-seg%d", i);
	seg = orte_gpr_replica_find_seg(false, name, test_jobid);
	for (j=0; j<10 && success; j++) {
 	    asprintf(&name3[j], "test-key%d", j);
	}
	name3[j] = NULL;
	keys = orte_gpr_replica_get_key_list(seg, name3, &num_keys);
	if (0 >= num_keys) { /* error condition */
	    success = false;
	}
    }
    if (success) {
	result->message = strdup("success");
    } else {
	result->message = strdup("failed");
    }
    ompi_list_append(test_results, &result->item);

    /* check ability to empty segment */

#endif
    return ORTE_SUCCESS;
}

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
 * The Open MPI general purpose registry - unit test
 *
 */

/*
 * includes
 */

#include "orte_config.h"
#include <stdio.h>
#include <string.h>

#include "include/constants.h"

#include "support.h"

#include "class/orte_pointer_array.h"
#include "runtime/runtime.h"
#include "util/proc_info.h"

#include "mca/gpr/base/base.h"
#include "mca/gpr/replica/api_layer/gpr_replica_api.h"
#include "mca/gpr/replica/functional_layer/gpr_replica_fn.h"
#include "mca/gpr/replica/communications/gpr_replica_comm.h"
#include "mca/gpr/replica/transition_layer/gpr_replica_tl.h"

/* output files needed by the test */
static FILE *test_out=NULL;

static char *cmd_str="diff ./test_gpr_replica_out ./test_gpr_replica_out_std";

static void test_cbfunc(orte_gpr_notify_message_t *notify_msg, void *user_tag);


int main(int argc, char **argv)
{
    int rc, i, num_names;
    bool allow_multi_user_threads = false;
    bool have_hidden_threads = false;
    char *tmp=NULL, *tmp2=NULL, *names[15];
    orte_gpr_replica_segment_t *seg=NULL;
    orte_gpr_replica_itag_t itag[10], itag2, *itaglist;
    orte_gpr_replica_container_t *cptr=NULL;
    orte_gpr_keyval_t *kptr=NULL;
    orte_gpr_replica_itagval_t **ivals=NULL, *ivaltst=NULL;
    
    test_init("test_gpr_replica");

   /*  test_out = fopen( "test_gpr_replica_out", "w+" ); */
    test_out = stderr;
    if( test_out == NULL ) {
      test_failure("gpr_test couldn't open test file failed");
      test_finalize();
      exit(1);
    } 

    ompi_init(argc, argv);

    orte_process_info.seed = true;

    /* startup the MCA */
    if (OMPI_SUCCESS == mca_base_open()) {
        fprintf(test_out, "MCA started\n");
    } else {
        fprintf(test_out, "MCA could not start\n");
        exit (1);
    }

    if (ORTE_SUCCESS == orte_gpr_base_open()) {
        fprintf(test_out, "GPR started\n");
    } else {
        fprintf(test_out, "GPR could not start\n");
        exit (1);
    }
    
    if (ORTE_SUCCESS == orte_gpr_base_select(&allow_multi_user_threads, 
                       &have_hidden_threads)) {
        fprintf(test_out, "GPR replica selected\n");
    } else {
        fprintf(test_out, "GPR replica could not be selected\n");
        exit (1);
    }
                       
    fprintf(stderr, "going to find seg\n");
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_find_seg(&seg, true, "test-segment"))) {
        fprintf(test_out, "gpr_test: find_seg failed with error code %d\n", rc);
        test_failure("gpr_test: find_seg failed");
        test_finalize();
        return rc;
    } else {
        fprintf(test_out, "gpr_test: find_seg passed\n");
    }
    
    fprintf(stderr, "creating tags\n");
    for (i=0; i<10; i++) {
        asprintf(&tmp, "test-tag-%d", i);
         if (ORTE_SUCCESS != (rc = orte_gpr_replica_create_itag(&itag[i], seg, tmp))) {
            fprintf(test_out, "gpr_test: create_itag failed with error code %d\n", rc);
            test_failure("gpr_test: create_itag failed");
            test_finalize();
            return rc;
        } else {
            fprintf(test_out, "gpr_test: create_itag passed\n");
        }
        free(tmp);
    }
    
    fprintf(stderr, "lookup tags\n");
    for (i=0; i<10; i++) {
         asprintf(&tmp, "test-tag-%d", i);
         if (ORTE_SUCCESS != (rc = orte_gpr_replica_dict_lookup(&itag2, seg, tmp)) ||
             itag2 != itag[i]) {
            fprintf(test_out, "gpr_test: lookup failed with error code %d\n", rc);
            test_failure("gpr_test: lookup failed");
            test_finalize();
            return rc;
        } else {
            fprintf(test_out, "gpr_test: lookup passed\n");
        }
        free(tmp);
    }
    
    
    fprintf(stderr, "reverse lookup tags\n");
    for (i=0; i<10; i++) {
         asprintf(&tmp2, "test-tag-%d", i);
         if (ORTE_SUCCESS != (rc = orte_gpr_replica_dict_reverse_lookup(&tmp, seg, itag[i])) ||
             0 != strcmp(tmp2, tmp)) {
            fprintf(test_out, "gpr_test: reverse lookup failed with error code %d\n", rc);
            test_failure("gpr_test: reverse lookup failed");
            test_finalize();
            return rc;
        } else {
            fprintf(test_out, "gpr_test: reverse lookup passed\n");
        }
        free(tmp);
    }
    
    
    fprintf(stderr, "delete tags\n");
    for (i=0; i<10; i++) {
         asprintf(&tmp, "test-tag-%d", i);
         if (ORTE_SUCCESS != (rc = orte_gpr_replica_delete_itag(seg, tmp))) {
            fprintf(test_out, "gpr_test: delete tag failed with error code %d\n", rc);
            test_failure("gpr_test: delete tag failed");
            test_finalize();
            return rc;
        } else {
            fprintf(test_out, "gpr_test: delete tag passed\n");
        }
        free(tmp);
    }
    
    fprintf(stderr, "get itag list\n");
    for (i=0; i < 14; i++) {
        asprintf(&names[i], "dummy%d", i);
    }
    names[14] = NULL;
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_get_itag_list(&itaglist, seg,
                                                names, &num_names))) {
        fprintf(test_out, "gpr_test: get itag list failed with error code %d\n", rc);
        test_failure("gpr_test: get itag list failed");
        test_finalize();
        return rc;
    } else {
        fprintf(test_out, "gpr_test: get itag list passed\n");
    }
    
    fprintf(test_out, "number of names found %d\n", num_names);
    for (i=0; i < num_names; i++) {
        fprintf(test_out, "\tname %s itag %d\n", names[i], itaglist[i]);
    }
    
    
    fprintf(stderr, "creating container\n");
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_create_container(&cptr, seg,
                                3, itaglist))) {
        fprintf(test_out, "gpr_test: create_container failed with error code %d\n", rc);
        test_failure("gpr_test: create_container failed");
        test_finalize();
        return rc;
    } else {
        fprintf(test_out, "gpr_test: create_container passed\n");
    }
    
    fprintf(test_out, "itags for container\n");
    for (i=0; i < cptr->num_itags; i++) {
        fprintf(test_out, "\tindex %d itag %d\n", i, cptr->itags[i]);
    }

    fprintf(stderr, "add keyval\n");
    kptr = OBJ_NEW(orte_gpr_keyval_t);
    kptr->key = strdup("stupid-value");
    kptr->type = ORTE_INT16;
    kptr->value.i16 = 21;
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_add_keyval(seg, cptr, &kptr)) ||
        NULL != kptr) {
        fprintf(test_out, "gpr_test: add keyval failed with error code %d\n", rc);
        test_failure("gpr_test: add keyval failed");
        test_finalize();
        return rc;
    } else {
        fprintf(test_out, "gpr_test: add keyval passed\n");
    }
    
    ivals = (orte_gpr_replica_itagval_t**)((cptr->itagvals)->addr);
    if (NULL != ivals[0]) {
        fprintf(stderr, "ival[0] %d %d %d\n", ivals[0]->itag,
                    ivals[0]->type, ivals[0]->value.i16);
    }
    
    fprintf(stderr, "update keyval\n");
    kptr = OBJ_NEW(orte_gpr_keyval_t);
    kptr->key = strdup("second-value");
    kptr->type = ORTE_STRING;
    kptr->value.strptr = strdup("try-string-value");
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_update_keyval(seg, ivals[0], kptr))) {
        fprintf(test_out, "gpr_test: update keyval failed with error code %d\n", rc);
        test_failure("gpr_test: update keyval failed");
        test_finalize();
        return rc;
    } else {
        fprintf(test_out, "gpr_test: update keyval passed\n");
    }
    
    if (NULL != ivals[0]) {
        fprintf(stderr, "ival[0] %d %d %s\n", ivals[0]->itag,
                    ivals[0]->type, ivals[0]->value.strptr);
    }


    fprintf(stderr, "search container\n");
    free(kptr->key);
    kptr->key = strdup("second-value");
    orte_gpr_replica_create_itag(&itag2, seg, kptr->key);
    if (!orte_gpr_replica_search_container(&ivaltst, itag2, cptr)) {
        fprintf(test_out, "gpr_test: search container failed\n");
        test_failure("gpr_test: search container failed");
        test_finalize();
        return -1;
    } else {
        fprintf(test_out, "gpr_test: search container passed\n");
    }
    
    if (NULL != ivaltst) {
        fprintf(stderr, "itag2 %d ivaltst %d %d %s\n", itag2, ivaltst->itag,
                    ivaltst->type, ivaltst->value.strptr);
    } else {
        fprintf(stderr, "ivaltst was NULL\n");
    }
    

    fprintf(stderr, "releasing segment\n");
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_release_segment(&seg)) ||
        NULL != seg) {
        fprintf(test_out, "gpr_test: release segment failed with error code %d\n", rc);
        test_failure("gpr_test: release segment failed");
        test_finalize();
        return rc;
    } else {
        fprintf(test_out, "gpr_test: release segment passed\n");
    }
    
    fclose( test_out );
/*    result = system( cmd_str );
    if( result == 0 ) {
        test_success();
    }
    else {
      test_failure( "test_gpr_replica failed");
    }
*/
    test_finalize();

    return(0);
}

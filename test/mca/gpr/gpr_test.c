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
    int rc;
    bool allow_multi_user_threads = false;
    bool have_hidden_threads = false;
    orte_gpr_replica_segment_t *seg;
    
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

    fprintf(stderr, "open registry\n");
    if (ORTE_SUCCESS == orte_gpr_base_open()) {
        fprintf(test_out, "GPR started\n");
    } else {
        fprintf(test_out, "GPR could not start\n");
        exit (1);
    }
    
    fprintf(stderr, "select registry replica\n");
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
        test_failure("gpr_test: find_seg_failed");
        test_finalize();
        return rc;
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

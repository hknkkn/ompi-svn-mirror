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

#include "include/orte_constants.h"
#include "include/orte_names.h"

#include "support.h"

#include "class/orte_pointer_array.h"
#include "dps/dps.h"
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
    int rc, num_names, num_found;
    int32_t i, j, cnt;
    bool allow_multi_user_threads = false;
    bool have_hidden_threads = false;
    char *tmp=NULL, *tmp2=NULL, *names[15], *keys[5];
    orte_gpr_replica_segment_t *seg=NULL;
    orte_gpr_replica_itag_t itag[10], itag2, *itaglist;
    orte_gpr_replica_container_t *cptr=NULL, **cptrs=NULL;
    orte_gpr_keyval_t *kptr=NULL, *karray[20], **kvals;
    orte_gpr_replica_itagval_t **ivals=NULL;
    orte_gpr_value_t **values;
    orte_gpr_notify_id_t synch, sub;
    
    test_init("test_gpr_replica_trigs");

   /*  test_out = fopen( "test_gpr_replica_out", "w+" ); */
    test_out = stderr;
    if( test_out == NULL ) {
      test_failure("gpr_test couldn't open test file failed");
      test_finalize();
      exit(1);
    } 

    /* ENSURE THE REPLICA IS ISOLATED */
    setenv("OMPI_MCA_gpr_replica_isolate", "1", 1);
    
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
                  
    if (ORTE_SUCCESS == orte_dps_open()) {
        fprintf(test_out, "DPS started\n");
    } else {
        fprintf(test_out, "DPS could not start\n");
        exit (1);
    }
    
    fprintf(stderr, "get itag list\n");
    for (i=0; i < 14; i++) {
        asprintf(&names[i], "dummy%d", i);
    }
    names[14] = NULL;

    fprintf(stderr, "register synchro on segment\n");
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_synchro(ORTE_GPR_XAND,
                                    ORTE_GPR_SYNCHRO_MODE_LEVEL,
                                    "test-segment",
                                    NULL, NULL, 5, &synch,
                                    gpr_test_trig_cb_fn, NULL))) {
        fprintf(test_out, "gpr_test_trigs: synch on seg failed with error %s\n",
                        ORTE_ERROR_NAME(rc));
        test_failure("gpr_test_trigs: synch on seg failed");
        test_finalize();
        return rc;
    } else {
        fprintf(test_out, "gpr_test_trigs: synch on seg registered\n");
    }
    
    fprintf(stderr, "register subscription on segment\n");
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_subscribe(ORTE_GPR_XAND,
                                    ORTE_GPR_NOTIFY_ADD_ENTRY,
                                    "test-segment",
                                    NULL, NULL, 5, &sub,
                                    gpr_test_trig_cb_fn, NULL))) {
        fprintf(test_out, "gpr_test_trigs: subscribe on seg failed with error %s\n",
                        ORTE_ERROR_NAME(rc));
        test_failure("gpr_test_trigs: subscribe on seg failed");
        test_finalize();
        return rc;
    } else {
        fprintf(test_out, "gpr_test_trigs: subscribe on seg registered\n");
    }
    
    orte_gpr_replica_dump(0);

#if 0
    fprintf(stderr, "put\n");
    kptr = OBJ_NEW(orte_gpr_keyval_t);
    kptr->key = strdup("stupid-value-next-one");
    kptr->type = ORTE_INT32;
    kptr->value.i32 = 654321;
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_put(ORTE_GPR_XAND,
                                "test-put-segment",
                                names, 1, &kptr))) {
        fprintf(test_out, "gpr_test: put of 1 keyval failed with error code %d\n", rc);
        test_failure("gpr_test: put of 1 keyval failed");
        test_finalize();
        return rc;
    } else {
        fprintf(test_out, "gpr_test: put of 1 keyval passed\n");
    }

    fprintf(stderr, "put multiple\n");
    for (i=0; i<20; i++) {
        karray[i] = OBJ_NEW(orte_gpr_keyval_t);
        asprintf(&(karray[i]->key), "stupid-test-%d", i);
        karray[i]->type = ORTE_UINT32;
        karray[i]->value.ui32 = (uint32_t)i;
    }
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_put(ORTE_GPR_XAND,
                                "test-put-segment",
                                names, 20, karray))) {
        fprintf(test_out, "gpr_test: put multiple keyval failed with error code %d\n", rc);
        test_failure("gpr_test: put multiple keyval failed");
        test_finalize();
        return rc;
    } else {
        fprintf(test_out, "gpr_test: put multiple keyval passed\n");
    }

    fprintf(stderr, "put multiple - second container\n");
    names[10] = NULL;
    for (i=0; i<10; i++) {
        karray[i] = OBJ_NEW(orte_gpr_keyval_t);
        asprintf(&(karray[i]->key), "stupid-test-%d", i);
        karray[i]->type = ORTE_UINT32;
        karray[i]->value.ui32 = (uint32_t)i;
    }
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_put(ORTE_GPR_XAND,
                                "test-put-segment",
                                names, 10, karray))) {
        fprintf(test_out, "gpr_test: put multiple keyval in second container failed with error code %d\n", rc);
        test_failure("gpr_test: put multiple keyval in second container failed");
        test_finalize();
        return rc;
    } else {
        fprintf(test_out, "gpr_test: put multiple keyval in second container passed\n");
    }

    
    fprintf(stderr, "dump\n");
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_dump(0))) {
        fprintf(test_out, "gpr_test: dump failed with error code %d\n", rc);
        test_failure("gpr_test: dump failed");
        test_finalize();
        return rc;
    } else {
        fprintf(test_out, "gpr_test: dump passed\n");
    }

#endif

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

void gpr_test_trig_cb_fn(orte_gpr_notify_message_t *msg, void *tag)
{
    fprintf(test_out, "TRIGGER FIRED AND RECEIVED\n");
    
    fprintf(test_out, "\tSegment: %s\tNumber of values: %d\n", msg->segment, msg->cnt);
    
    OBJ_RELEASE(msg);
}

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

#include "support.h"

#include "class/orte_pointer_array.h"
#include "dps/dps.h"
#include "runtime/runtime.h"
#include "util/output.h"
#include "util/proc_info.h"
#include "util/sys_info.h"

#include "mca/errmgr/errmgr.h"
#include "mca/errmgr/base/base.h"

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
    int rc, num_names, num_found, num_counters=6;
    int i, j, cnt, ret;
    orte_gpr_value_t *values, value, trig;
    orte_gpr_notify_id_t sub;
    char* keys[] = {
        /* changes to this ordering need to be reflected in code below */
        ORTE_PROC_NUM_AT_STG1,
        ORTE_PROC_NUM_AT_STG2,
        ORTE_PROC_NUM_AT_STG3,
        ORTE_PROC_NUM_FINALIZED,
        ORTE_PROC_NUM_ABORTED,
        ORTE_PROC_NUM_TERMINATED
    };

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
    
    /* Open up the output streams */
    if (!ompi_output_init()) {
        return OMPI_ERROR;
    }
                                                                                                                   
    /* 
     * If threads are supported - assume that we are using threads - and reset otherwise. 
     */
    ompi_set_using_threads(OMPI_HAVE_THREADS);
                                                                                                                   
    /* For malloc debugging */
    ompi_malloc_init();

    /* Ensure the system_info structure is instantiated and initialized */
    if (ORTE_SUCCESS != (ret = orte_sys_info())) {
        return ret;
    }

    /* Ensure the process info structure is instantiated and initialized */
    if (ORTE_SUCCESS != (ret = orte_proc_info())) {
        return ret;
    }
    
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
    
    if (ORTE_SUCCESS == orte_gpr_base_select()) {
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
    
    if (ORTE_SUCCESS == orte_errmgr_base_open()) {
        fprintf(test_out, "error mgr started\n");
    } else {
        fprintf(test_out, "error mgr could not start\n");
        exit (1);
    }
    
    OBJ_CONSTRUCT(&value, orte_gpr_value_t);
    value.addr_mode = ORTE_GPR_TOKENS_OR;
    value.segment = strdup("test-segment");
    value.num_tokens = 0;
    value.tokens = NULL;
    value.cnt = 1;
    value.keyvals = (orte_gpr_keyval_t**)malloc(sizeof(orte_gpr_keyval_t*));
    value.keyvals[0] = OBJ_NEW(orte_gpr_keyval_t);
    value.keyvals[0]->key = strdup("dummy");
    value.keyvals[0]->type = ORTE_INT16;
    value.keyvals[0]->value.i16 = 1;
    
    fprintf(stderr, "register subscription on segment\n");
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_subscribe(ORTE_GPR_NOTIFY_ADD_ENTRY,
                                    &value,
                                    NULL,
                                    &sub,
                                    test_cbfunc, NULL))) {
        fprintf(test_out, "gpr_test_trigs: subscribe on seg failed with error %s\n",
                        ORTE_ERROR_NAME(rc));
        test_failure("gpr_test_trigs: subscribe on seg failed");
        test_finalize();
        return rc;
    } else {
        fprintf(test_out, "gpr_test_trigs: subscribe on seg registered\n");
    }
    OBJ_DESTRUCT(&value);
    
    orte_gpr_replica_dump(0);

    /* setup some test counters */
    OBJ_CONSTRUCT(&value, orte_gpr_value_t);
    value.addr_mode = ORTE_GPR_TOKENS_XAND | ORTE_GPR_KEYS_OR;
    value.segment = strdup("test-segment");
    value.tokens = (char**)malloc(sizeof(char*));
    if (NULL == value.tokens) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        OBJ_DESTRUCT(&value);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    value.tokens[0] = strdup(ORTE_JOB_GLOBALS); /* put counters in the segment's globals container */
    value.num_tokens = 1;
    value.cnt = num_counters;
    value.keyvals = (orte_gpr_keyval_t**)malloc(num_counters * sizeof(orte_gpr_keyval_t*));
    if (NULL == value.keyvals) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        OBJ_DESTRUCT(&value);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    for (i=0; i < num_counters; i++) {
        value.keyvals[i] = OBJ_NEW(orte_gpr_keyval_t);
        if (NULL == value.keyvals[i]) {
            ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
            OBJ_DESTRUCT(&value);
            return ORTE_ERR_OUT_OF_RESOURCE;
        }
        value.keyvals[i]->key = strdup(keys[i]);
        value.keyvals[i]->type = ORTE_INT32;
        value.keyvals[i]->value.i32 = 0;
    }
    /* set value in keys[0] to 3 */
    value.keyvals[0]->value.i32 = 3;
    
    values = &value;
    
    fprintf(test_out, "putting counters on registry\n");
    
    /* put the counters on the registry */
    if (ORTE_SUCCESS != (rc = orte_gpr.put(1, &values))) {
        ORTE_ERROR_LOG(rc);
        OBJ_DESTRUCT(&value);
        return rc;
    }
    
    orte_gpr_replica_dump(0);

    fprintf(test_out, "incrementing all counters\n");
    
    /* increment the counters */
    if (ORTE_SUCCESS != (rc = orte_gpr.increment_value(&value))) {
        ORTE_ERROR_LOG(rc);
        OBJ_DESTRUCT(&value);
        return rc;
    }
    
    orte_gpr_replica_dump(0);
    
    fprintf(test_out, "decrementing all counters\n");
    
    /* decrement the counters */
    if (ORTE_SUCCESS != (rc = orte_gpr.decrement_value(&value))) {
        ORTE_ERROR_LOG(rc);
        OBJ_DESTRUCT(&value);
        return rc;
    }
    
    orte_gpr_replica_dump(0);


    /* for testing the trigger, we want the counter values returned to us
     * setup value so it can be used for that purpose.
     */
    for (i=0; i < num_counters; i++) {
        OBJ_RELEASE(value.keyvals[i]);
    }
    free(value.keyvals);
    value.keyvals = (orte_gpr_keyval_t**)malloc(sizeof(orte_gpr_keyval_t*));
    if (NULL == value.keyvals) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        OBJ_DESTRUCT(&value);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    value.keyvals[0] = OBJ_NEW(orte_gpr_keyval_t);
    if (NULL == value.keyvals[0]) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        OBJ_DESTRUCT(&value);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    value.cnt = 1;
    value.keyvals[0]->key = strdup(keys[0]);
    value.keyvals[0]->type = ORTE_NULL;
    
    /* setup the trigger information - initialize the common elements */
    OBJ_CONSTRUCT(&trig, orte_gpr_value_t);
    trig.addr_mode = ORTE_GPR_TOKENS_XAND;
    trig.segment = strdup("test-segment");
    trig.tokens = (char**)malloc(sizeof(char*));
    if (NULL == trig.tokens) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        OBJ_DESTRUCT(&value);
        OBJ_DESTRUCT(&trig);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    trig.tokens[0] = strdup(ORTE_JOB_GLOBALS);
    trig.num_tokens = 1;
    trig.cnt = 2;
    trig.keyvals = (orte_gpr_keyval_t**)malloc(2*sizeof(orte_gpr_keyval_t*));
    trig.keyvals[0] = OBJ_NEW(orte_gpr_keyval_t);
    trig.keyvals[0]->key = strdup(keys[0]);
    trig.keyvals[0]->type = ORTE_NULL;

    trig.keyvals[1] = OBJ_NEW(orte_gpr_keyval_t);
    trig.keyvals[1]->key = strdup(keys[1]);
    trig.keyvals[1]->type = ORTE_NULL;
    
   fprintf(test_out, "setting trigger\n");
   
   rc = orte_gpr.subscribe(
         ORTE_GPR_TRIG_ALL_CMP,
         &value,
         &trig,
         &rc,
         test_cbfunc,
         NULL);

     if(ORTE_SUCCESS != rc) {
         ORTE_ERROR_LOG(rc);
         OBJ_DESTRUCT(&value);
         OBJ_DESTRUCT(&trig);
         return rc;
     }

    orte_gpr_replica_dump(0);
    
    fprintf(test_out, "incrementing until trigger\n");
    
    /* increment the value in keys[1] until the trig fires */
    free(value.keyvals[0]->key);
    value.keyvals[0]->key = strdup(keys[1]);
    
    for (i=0; i < 10; i++) {
        fprintf(test_out, "\tincrement %s\n", keys[1]);
        if (ORTE_SUCCESS != (rc = orte_gpr.increment_value(&value))) {
            ORTE_ERROR_LOG(rc);
            OBJ_DESTRUCT(&value);
            return rc;
        }
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

void test_cbfunc(orte_gpr_notify_message_t *msg, void *tag)
{
    fprintf(test_out, "TRIGGER FIRED AND RECEIVED\n");
    
    fprintf(test_out, "\tSegment: %s\tNumber of values: %d\n", (msg->values[0])->segment, msg->cnt);
    
    orte_gpr_replica_dump(0);
    
    OBJ_RELEASE(msg);
}

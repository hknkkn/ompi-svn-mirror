/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
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
#include "mca/schema/schema.h"
#include "mca/schema/base/base.h"
#include "mca/ns/base/base.h"
#include "mca/soh/base/base.h"
#include "mca/rmgr/base/base.h"

#include "support.h"

#include "class/orte_pointer_array.h"
#include "dps/dps.h"
#include "runtime/runtime.h"
#include "util/proc_info.h"
#include "util/sys_info.h"
#include "opal/util/malloc.h"
#include "opal/util/output.h"

#include "mca/gpr/base/base.h"
#include "mca/gpr/gpr.h"

/* output files needed by the test */
static FILE *test_out=NULL;

static char *cmd_str="diff ./test_gpr_replica_out ./test_gpr_replica_out_std";

int main(int argc, char **argv)
{
    int rc;
    size_t i, cnt;
    char *names[15], *keys[5];
    orte_gpr_value_t **values, *val;

    test_init("test_gpr_replica");

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
    if (!opal_output_init()) {
        return OMPI_ERROR;
    }

    /* 
     * If threads are supported - assume that we are using threads -
     * and reset otherwise.
     */
    opal_set_using_threads(OMPI_HAVE_THREAD_SUPPORT);

    /* For malloc debugging */
    opal_malloc_init();

    /* Ensure the system_info structure is instantiated and initialized */
    if (ORTE_SUCCESS != (rc = orte_sys_info())) {
        return rc;
    }

    /* Ensure the process info structure is instantiated and initialized */
    if (ORTE_SUCCESS != (rc = orte_proc_info())) {
        return rc;
    }

    orte_process_info.seed = true;
    orte_process_info.my_name = (orte_process_name_t*)malloc(sizeof(orte_process_name_t));
    orte_process_info.my_name->cellid = 0;
    orte_process_info.my_name->jobid = 0;
    orte_process_info.my_name->vpid = 0;

    /* startup the MCA */
    if (OMPI_SUCCESS == mca_base_open()) {
        fprintf(test_out, "MCA started\n");
    } else {
        fprintf(test_out, "MCA could not start\n");
        exit (1);
    }

    /* startup the dps */
    if (OMPI_SUCCESS == orte_dps_open()) {
        fprintf(test_out, "DPS started\n");
    } else {
        fprintf(test_out, "DPS could not start\n");
        exit (1);
    }

    /* startup the name services */
    if (OMPI_SUCCESS == orte_ns_base_open()) {
        fprintf(test_out, "NS started\n");
    } else {
        fprintf(test_out, "NS could not start\n");
        exit (1);
    }

    /* startup the soh */
    if (OMPI_SUCCESS == orte_soh_base_open()) {
        fprintf(test_out, "SOH started\n");
    } else {
        fprintf(test_out, "SOH could not start\n");
        exit (1);
    }

    /* startup the rmgr */
    if (OMPI_SUCCESS == orte_rmgr_base_open()) {
        fprintf(test_out, "RMGR started\n");
    } else {
        fprintf(test_out, "RMGR could not start\n");
        exit (1);
    }

    /* startup the schema */
    if (OMPI_SUCCESS == orte_schema_base_open()) {
        fprintf(test_out, "SCHEMA started\n");
    } else {
        fprintf(test_out, "SCHEMA could not start\n");
        exit (1);
    }

    /* startup the registry */
    if (OMPI_SUCCESS == orte_gpr_base_open()) {
        fprintf(test_out, "GPR started\n");
    } else {
        fprintf(test_out, "GPR could not start\n");
        exit (1);
    }

    /* do a select on the registry components */
    if (OMPI_SUCCESS == orte_gpr_base_select()) {
        fprintf(test_out, "GPR selected\n");
    } else {
        fprintf(test_out, "GPR could not select\n");
        exit (1);
    }
    
    fprintf(stderr, "do a put of a value with multiple keyvals\n");
    val = OBJ_NEW(orte_gpr_value_t);
    val->addr_mode = ORTE_GPR_NO_OVERWRITE | ORTE_GPR_TOKENS_XAND;
    val->cnt = 20;
    val->segment = strdup("test-put-segment");
    val->num_tokens = 14;
    val->tokens = (char**)malloc(val->num_tokens * sizeof(char*));
    for (i=0; i < 14; i++) {
        asprintf(&(val->tokens[i]), "dummy%lu", (unsigned long) i);
    }
    val->keyvals = (orte_gpr_keyval_t**)malloc(20*sizeof(orte_gpr_keyval_t*));
    for (i=0; i<20; i++) {
        val->keyvals[i] = OBJ_NEW(orte_gpr_keyval_t);
        asprintf(&((val->keyvals[i])->key), "stupid-test-%lu", 
                 (unsigned long) i);
        (val->keyvals[i])->type = ORTE_UINT32;
        (val->keyvals[i])->value.ui32 = (uint32_t)i;
    }
    if (ORTE_SUCCESS != (rc = orte_gpr.put(1, &val))) {
        fprintf(test_out, "gpr_test: put 1 value/multiple keyval failed with error code %s\n",
                    ORTE_ERROR_NAME(rc));
        test_failure("gpr_test: put 1 value/multiple keyval failed");
        test_finalize();
        return rc;
    } else {
        fprintf(test_out, "gpr_test: put 1 value/multiple keyval passed\n");
    }
    OBJ_RELEASE(val);

    fprintf(stderr, "do a get\n");
    names[0] = strdup("dummy0");
    names[1] = NULL;
    keys[0] = strdup("stupid-test-1");
    keys[1] = strdup("stupid-test-3");
    keys[2] = strdup("stupid-test-5");
    keys[3] = strdup("stupid-test-8");
    keys[4] = NULL;
    if (ORTE_SUCCESS != (rc = orte_gpr.get(ORTE_GPR_KEYS_OR | ORTE_GPR_TOKENS_OR,
                                "test-put-segment",
                                names, keys,
                                &cnt, &values))) {
        fprintf(test_out, "gpr_test: get failed with error code %s\n",
                    ORTE_ERROR_NAME(rc));
        test_failure("gpr_test: get failed");
        test_finalize();
        return rc;
    } else {
        fprintf(test_out, "gpr_test: get passed\n");
    }

    /* cleanup the results */
    free(names[0]);
    for (i=0; i < 4; i++) free(keys[i]);
    for (i=0; i < cnt; i++) {
        OBJ_RELEASE(values[i]);
    }
    free(values);
    
    fprintf(stderr, "put multiple copies of same entry in single container\n");
    val = OBJ_NEW(orte_gpr_value_t);
    val->addr_mode = ORTE_GPR_NO_OVERWRITE | ORTE_GPR_TOKENS_XAND;
    val->cnt = 1;
    val->segment = strdup("test-put-segment");
    val->num_tokens = 5;
    val->tokens = (char**)malloc(val->num_tokens * sizeof(char*));
    for (i=0; i < 5; i++) {
        asprintf(&(val->tokens[i]), "multi-dum-dum-%lu", (unsigned long) i);
    }
    val->keyvals = (orte_gpr_keyval_t**)malloc(sizeof(orte_gpr_keyval_t*));
    val->keyvals[0] = OBJ_NEW(orte_gpr_keyval_t);
    (val->keyvals[0])->key = strdup("stupid-value-next-one");
    (val->keyvals[0])->type = ORTE_STRING;
    (val->keyvals[0])->value.strptr = strdup("try-string-value");
    for (i = 0; i < 10; i++) {
        fprintf(stderr, "\tputting copy %lu\n", (unsigned long) i);
        if (ORTE_SUCCESS != (rc = orte_gpr.put(1, &val))) {
            fprintf(test_out, "gpr_test: put multiple copies of one keyval in a container failed with error code %s\n",
                        ORTE_ERROR_NAME(rc));
            test_failure("gpr_test: put multiple copies of one keyval in a container failed");
            test_finalize();
            return rc;
        }
    }
    OBJ_RELEASE(val);
    
    fprintf(stderr, "generate a trigger event\n");
    
    fprintf(stderr, "now finalize and see if all memory cleared\n");
    orte_gpr_base_close();
    orte_dps_close();
    orte_sys_info_finalize();
    orte_proc_info_finalize();
    mca_base_close();
    opal_malloc_finalize();
    opal_output_finalize();
    opal_class_finalize();

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

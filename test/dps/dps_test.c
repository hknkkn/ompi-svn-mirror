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
#include "../src/include/orte_constants.h"
#include "../src/include/orte_types.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include "include/constants.h"

#include "support.h"
#include "../src/runtime/runtime.h"
#include "../src/mca/base/base.h"
#include "../src/dps/dps.h"
#include "../src/mca/ns/ns_types.h"

#define NUM_ITERS 1
#define NUM_ELEMS 1

static bool test1(void);        /* verify different buffer inits */
static bool test2(void);        /* verify we can pack ok */
static bool test3(void);        /* verify we can pack expanding buf */
static bool test4(void);        /* verify pack a packed buffer */
static bool test5(void);        /* verify unpack */
static bool test6(void);        /* verify free */
static bool test7(void);        /* verify preallocated buffer init, pack and unpack */
static bool test8(void);        /* verify string pack and unpack */

FILE *test_out;

int main (int argc, char* argv[])
{

    test_init("orte_dps");
    test_out = stderr;
    
    /* open up the mca so we can get parameters */
    ompi_init(argc, argv);

    /* startup the MCA */
    if (OMPI_SUCCESS != mca_base_open()) {
        fprintf(stderr, "can't open mca\n");
        exit (1);
    }
    
    /* setup the dps */
    orte_dps_open();
    
    if (test1()) {
        test_success();
    }
    else {
      test_failure("orte_dps test1 failed");
    }

    if (test2()) {
        test_success();
    }
    else {
      test_failure("orte_dps test2 failed");
    }
    
    if (test3()) {
        test_success();
    }
    else {
      test_failure("orte_dps test3 failed");
    }

    if (test4()) {
        test_success();
    }
    else {
      test_failure("orte_dps test4 failed");
    }

    if (test5()) {
        test_success();
    }
    else {
      test_failure("orte_dps test5 failed");
    }

    if (test6()) {
        test_success();
    }
    else {
      test_failure("orte_dps test6 failed");
    }

    if (test7()) {
        test_success();
    }
    else {
      test_failure("orte_dps test7 failed");
    }

    if (test8()) {
        test_success();
    }
    else {
      test_failure("orte_dps test8 failed");
    }

    test_finalize();
    fclose(test_out);
    return (0);
}

static bool test1(void)        /* verify different buffer inits */
{
    orte_buffer_t *bufA;
    
    bufA = OBJ_NEW(orte_buffer_t);
    if (NULL == bufA) {
        test_comment("orte_buffer failed init in OBJ_NEW");
        fprintf(test_out, "OBJ_NEW failed\n");
        return false;
    }
    
    orte_dps.dump(bufA, 0);
    
    OBJ_RELEASE(bufA);
    if (NULL != bufA) {
        test_comment("OBJ_RELEASE did not NULL the buffer pointer");
        fprintf(test_out, "OBJ_RELEASE did not NULL the buffer pointer");
        return false;
    }
    return (true);
}

/*
 * OMPI_INT16 pack/unpack
 */
static bool test2(void) 
{
    orte_buffer_t *bufA;
    int rc;
    int32_t i;
    int16_t src[NUM_ELEMS];
    int16_t dst[NUM_ELEMS];

    for(i=0; i<NUM_ELEMS; i++)
        src[i] = i;

    bufA = OBJ_NEW(orte_buffer_t);
    if (NULL == bufA) {
        test_comment("orte_buffer failed init in OBJ_NEW");
        fprintf(test_out, "OBJ_NEW failed\n");
        return false;
    }
    
    orte_dps.dump(bufA, 0);
    
    for (i=0;i<NUM_ITERS;i++) {
        rc = orte_dps.pack(bufA, src, NUM_ELEMS, ORTE_INT16);
        if (ORTE_SUCCESS != rc) {
            test_comment ("orte_dps.pack failed");
            fprintf(test_out, "orte_pack_value failed with return code %d\n", rc);
            return(false);
        }
    }
    
    orte_dps.dump(bufA, 0);
    
    for (i=0; i<NUM_ITERS; i++) {
        int j;
        size_t count = NUM_ELEMS;

        for(j=0; j<NUM_ELEMS; j++)
            dst[j] = -1;

        rc = orte_dps.unpack(bufA, dst, &count, ORTE_INT16);
        if (ORTE_SUCCESS != rc || count != NUM_ELEMS) {
            test_comment ("orte_dps.unpack failed");
            fprintf(test_out, "orte_pack_value failed with return code %d\n", rc);
            return(false);
        }

        for(j=0; j<NUM_ELEMS; j++) {
            if(src[j] != dst[j]) {
                test_comment ("test2: invalid results from unpack");
                return(false);
            }
        }
    }
         
    OBJ_RELEASE(bufA);
    if (NULL != bufA) {
        test_comment("OBJ_RELEASE did not NULL the buffer pointer");
        fprintf(test_out, "OBJ_RELEASE did not NULL the buffer pointer");
        return false;
    }

    return (true);
}

/*
 * OMPI_INT32 pack/unpack 
 */
static bool test3(void)
{
    orte_buffer_t *bufA;
    int rc;
    int32_t i;
    int32_t src[NUM_ELEMS];
    int32_t dst[NUM_ELEMS];

    for(i=0; i<NUM_ELEMS; i++)
        src[i] = i;

    bufA = OBJ_NEW(orte_buffer_t);
    if (NULL == bufA) {
        test_comment("orte_buffer failed init in OBJ_NEW");
        fprintf(test_out, "OBJ_NEW failed\n");
        return false;
    }
    
    for (i=0;i<NUM_ITERS;i++) {
        rc = orte_dps.pack(bufA, src, NUM_ELEMS, ORTE_INT32);
        if (ORTE_SUCCESS != rc) {
            test_comment ("orte_dps.pack failed");
            fprintf(test_out, "orte_pack_value failed with return code %d\n", rc);
            return(false);
        }
    }
    
    for (i=0; i<NUM_ITERS; i++) {
        int j;
        size_t count = NUM_ELEMS;

        for(j=0; j<NUM_ELEMS; j++)
            dst[j] = -1;

        rc = orte_dps.unpack(bufA, dst, &count, ORTE_INT32);
        if (ORTE_SUCCESS != rc || count != NUM_ELEMS) {
            test_comment ("orte_dps.unpack failed");
            fprintf(test_out, "orte_pack_value failed with return code %d\n", rc);
            return(false);
        }

        for(j=0; j<NUM_ELEMS; j++) {
            if(src[j] != dst[j]) {
                test_comment ("test2: invalid results from unpack");
                return(false);
            }
        }
    }
         
    OBJ_RELEASE(bufA);
    if (NULL != bufA) {
        test_comment("OBJ_RELEASE did not NULL the buffer pointer");
        fprintf(test_out, "OBJ_RELEASE did not NULL the buffer pointer");
        return false;
    }

    return (true);
}

/*
 * OMPI_STRING pack/unpack
 */

static bool test4(void)
{
    orte_buffer_t *bufA;
    int rc;
    int32_t i;
    char* src[NUM_ELEMS];
    char* dst[NUM_ELEMS];

    for(i=0; i<NUM_ELEMS; i++) {
        asprintf(&src[i], "%d", i);
    }

    bufA = OBJ_NEW(orte_buffer_t);
    if (NULL == bufA) {
        test_comment("orte_buffer failed init in OBJ_NEW");
        fprintf(test_out, "OBJ_NEW failed\n");
        return false;
    }
    
    for (i=0;i<NUM_ITERS;i++) {
        rc = orte_dps.pack(bufA, src, NUM_ELEMS, ORTE_STRING);
        if (ORTE_SUCCESS != rc) {
            test_comment ("orte_dps.pack failed");
            fprintf(test_out, "orte_pack_value failed with return code %d\n", rc);
            return(false);
        }
    }
    
    for (i=0; i<NUM_ITERS; i++) {
        int j;
        size_t count = NUM_ELEMS;

        for(j=0; j<NUM_ELEMS; j++)
            dst[j] = NULL;

        rc = orte_dps.unpack(bufA, dst, &count, ORTE_STRING);
        if (ORTE_SUCCESS != rc || count != NUM_ELEMS) {
            test_comment ("test4: orte_dps.unpack failed");
            fprintf(test_out, "orte_pack_value failed with return code %d\n", rc);
            return(false);
        }

        for(j=0; j<NUM_ELEMS; j++) {
            if(strcmp(src[j],dst[j]) != 0) {
                test_comment ("test4: invalid results from unpack");
                return(false);
            }
        }
    }
         
    OBJ_RELEASE(bufA);
    if (NULL != bufA) {
        test_comment("OBJ_RELEASE did not NULL the buffer pointer");
        fprintf(test_out, "OBJ_RELEASE did not NULL the buffer pointer");
        return false;
    }

    return (true);
}


/**
 *  OMPI_NAME pack/unpack 
 */

static bool test5(void)
{
    orte_buffer_t *bufA;
    int rc;
    int32_t i;
    orte_process_name_t src[NUM_ELEMS];
    orte_process_name_t dst[NUM_ELEMS];

    for(i=0; i<NUM_ELEMS; i++) {
        src[i].cellid = 1000 + i;
        src[i].jobid = 100 + i;
        src[i].vpid = i;
    }

    bufA = OBJ_NEW(orte_buffer_t);
    if (NULL == bufA) {
        test_comment("orte_buffer failed init in OBJ_NEW");
        fprintf(test_out, "OBJ_NEW failed\n");
        return false;
    }
    
    for (i=0;i<NUM_ITERS;i++) {
        rc = orte_dps.pack(bufA, src, NUM_ELEMS, ORTE_NAME);
        if (ORTE_SUCCESS != rc) {
            test_comment ("orte_dps.pack failed");
            fprintf(test_out, "orte_pack_value failed with return code %d\n", rc);
            return(false);
        }
    }
    
    for (i=0; i<NUM_ITERS; i++) {
        int j;
        size_t count = NUM_ELEMS;
        memset(dst,-1,sizeof(dst));

        rc = orte_dps.unpack(bufA, dst, &count, ORTE_NAME);
        if (ORTE_SUCCESS != rc || count != NUM_ELEMS) {
            test_comment ("test5: orte_dps.unpack failed");
            fprintf(test_out, "orte_pack_value failed with return code %d\n", rc);
            return(false);
        }

        for(j=0; j<NUM_ELEMS; j++) {
            if(memcmp(&src[j],&dst[j],sizeof(orte_process_name_t)) != 0) {
                test_comment ("test5: invalid results from unpack");
                return(false);
            }
        }
    }
         
    OBJ_RELEASE(bufA);
    if (NULL != bufA) {
        test_comment("OBJ_RELEASE did not NULL the buffer pointer");
        fprintf(test_out, "OBJ_RELEASE did not NULL the buffer pointer");
        return false;
    }

    return(true);
}

/**
 * OMPI_BOOL pack/unpack
 */

static bool test6(void)
{
    orte_buffer_t *bufA;
    int rc;
    int32_t i;
    bool src[NUM_ELEMS];
    bool dst[NUM_ELEMS];

    for(i=0; i<NUM_ELEMS; i++)
        src[i] = ((i % 2) == 0) ? true : false;

    bufA = OBJ_NEW(orte_buffer_t);
    if (NULL == bufA) {
        test_comment("orte_buffer failed init in OBJ_NEW");
        fprintf(test_out, "OBJ_NEW failed\n");
        return false;
    }
    
    for (i=0;i<NUM_ITERS;i++) {
        rc = orte_dps.pack(bufA, src, NUM_ELEMS, ORTE_BOOL);
        if (ORTE_SUCCESS != rc) {
            test_comment ("orte_dps.pack failed");
            fprintf(test_out, "orte_pack_value failed with return code %d\n", rc);
            return(false);
        }
    }
    
    for (i=0; i<NUM_ITERS; i++) {
        int j;
        size_t count = NUM_ELEMS;
        memset(dst,-1,sizeof(dst));

        rc = orte_dps.unpack(bufA, dst, &count, ORTE_BOOL);
        if (ORTE_SUCCESS != rc || count != NUM_ELEMS) {
            test_comment ("orte_dps.unpack failed");
            fprintf(test_out, "orte_pack_value failed with return code %d\n", rc);
            return(false);
        }

        for(j=0; j<NUM_ELEMS; j++) {
            if(src[j] != dst[j]) {
                test_comment ("test6: invalid results from unpack");
                return(false);
            }
        }
    }
         
    OBJ_RELEASE(bufA);
    if (NULL != bufA) {
        test_comment("OBJ_RELEASE did not NULL the buffer pointer");
        fprintf(test_out, "OBJ_RELEASE did not NULL the buffer pointer");
        return false;
    }

    return (true);
    return (true);
}

/**
 * OMPI_BYTE_OBJECT pack/unpack
 */

static bool test7(void) 
{

    orte_buffer_t *bufA;
    int rc;
    int32_t i;
    orte_byte_object_t src[NUM_ELEMS];
    orte_byte_object_t dst[NUM_ELEMS];

    for(i=0; i<NUM_ELEMS; i++) {
        asprintf((char**)&src[i].bytes, "%d", i);
        src[i].size = strlen((char*)src[i].bytes) + 1;
    }

    bufA = OBJ_NEW(orte_buffer_t);
    if (NULL == bufA) {
        test_comment("orte_buffer failed init in OBJ_NEW");
        fprintf(test_out, "OBJ_NEW failed\n");
        return false;
    }
    
    for (i=0;i<NUM_ITERS;i++) {
        rc = orte_dps.pack(bufA, src, NUM_ELEMS, ORTE_BYTE_OBJECT);
        if (ORTE_SUCCESS != rc) {
            test_comment ("orte_dps.pack failed");
            fprintf(test_out, "orte_pack_value failed with return code %d\n", rc);
            return(false);
        }
    }
    
    for (i=0; i<NUM_ITERS; i++) {
        int j;
        size_t count = NUM_ELEMS;

        memset(dst,0,sizeof(dst));

        rc = orte_dps.unpack(bufA, dst, &count, ORTE_BYTE_OBJECT);
        if (ORTE_SUCCESS != rc || count != NUM_ELEMS) {
            test_comment ("test7: orte_dps.unpack failed");
            fprintf(test_out, "orte_pack_value failed with return code %d\n", rc);
            return(false);
        }

        for(j=0; j<NUM_ELEMS; j++) {
            if(src[j].size != dst[j].size ||
               memcmp(src[j].bytes,dst[j].bytes,src[j].size) != 0) {
                test_comment ("test7: invalid results from unpack");
                return(false);
            }
        }
    }
         
    OBJ_RELEASE(bufA);
    if (NULL != bufA) {
        test_comment("OBJ_RELEASE did not NULL the buffer pointer");
        fprintf(test_out, "OBJ_RELEASE did not NULL the buffer pointer");
        return false;
    }

    return (true);
    return (true);
}

static bool test8(void)
{
    return (true);
}



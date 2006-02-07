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

#include "orte_config.h"
#include "orte/include/orte_constants.h"
#include "orte/include/orte_types.h"


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include "opal/runtime/opal.h"
#include "orte/util/proc_info.h"
#include "orte/mca/errmgr/errmgr.h"

#include "orte/dss/dss.h"

#define NUM_ITERS 100
#define NUM_ELEMS 1024

static bool test1(void);        /* verify different buffer inits */
static bool test2(void);        /* verify int16 */
static bool test3(void);      /* verify int */
static bool test4(void);        /* verify int32 */
static bool test5(void);      /* verify int64 */
static bool test6(void);        /* verify string */
static bool test7(void);        /* verify BOOL */
static bool test8(void);        /* verify OBJECT */
static bool test9(void);        /* verify composite (multiple types and element counts) */
static bool test10(void);        /* verify KEYVAL */
static bool test11(void);        /* verify size_t */
static bool test12(void);        /* verify pid_t */

FILE *test_out;


int main (int argc, char* argv[])
{
    int ret;

    opal_init();

    /* register handler for errnum -> string converstion */
    opal_error_register("ORTE", ORTE_ERR_BASE, ORTE_ERR_MAX, orte_err2str);

    test_out = stderr;

    /* Ensure the process info structure is instantiated and initialized */
    if (ORTE_SUCCESS != (ret = orte_proc_info())) {
        return ret;
    }

    orte_process_info.seed = true;
    orte_process_info.my_name = (orte_process_name_t*)malloc(sizeof(orte_process_name_t));
    orte_process_info.my_name->cellid = 0;
    orte_process_info.my_name->jobid = 0;
    orte_process_info.my_name->vpid = 0;


    /* open the dss */
    if (ORTE_SUCCESS == orte_dss_open()) {
        fprintf(test_out, "DSS started\n");
    } else {
        fprintf(test_out, "DSS could not start\n");
        exit (1);
    }

    /* run the tests */

    fprintf(test_out, "executing test1\n");
    if (test1()) {
        fprintf(test_out, "Test1 succeeded\n");
    }
    else {
      fprintf(test_out, "Test1 failed\n");
    }

    fprintf(test_out, "executing test2\n");
    if (test2()) {
        fprintf(test_out, "Test2 succeeded\n");
    }
    else {
      fprintf(test_out, "Test2 failed\n");
    }

    fprintf(test_out, "executing test3\n");
    if (test3()) {
        fprintf(test_out, "Test3 succeeded\n");
    }
    else {
      fprintf(test_out, "Test3 failed\n");
    }

    fprintf(test_out, "executing test4\n");
    if (test4()) {
        fprintf(test_out, "Test4 succeeded\n");
    }
    else {
      fprintf(test_out, "Test4 failed\n");
    }

    fprintf(test_out, "executing test5\n");
    if (test5()) {
        fprintf(test_out, "Test5 succeeded\n");
    }
    else {
      fprintf(test_out, "Test5 failed\n");
    }

    fprintf(test_out, "executing test6\n");
    if (test6()) {
        fprintf(test_out, "Test6 succeeded\n");
    }
    else {
      fprintf(test_out, "Test6 failed\n");
    }

    fprintf(test_out, "executing test7\n");
    if (test7()) {
        fprintf(test_out, "Test7 succeeded\n");
    }
    else {
      fprintf(test_out, "Test7 failed\n");
    }

    fprintf(test_out, "executing test8\n");
    if (test8()) {
        fprintf(test_out, "Test8 succeeded\n");
    }
    else {
      fprintf(test_out, "Test8 failed\n");
    }

    fprintf(test_out, "executing test9\n");
    if (test9()) {
        fprintf(test_out, "Test9 succeeded\n");
    }
    else {
      fprintf(test_out, "orte_dss test9 failed\n");
    }

    fprintf(test_out, "executing test10\n");
    if (test10()) {
        fprintf(test_out, "Test10 succeeded\n");
    }
    else {
      fprintf(test_out, "orte_dss test10 failed\n");
    }

    fprintf(test_out, "executing test11\n");
    if (test11()) {
        fprintf(test_out, "Test11 succeeded\n");
    }
    else {
      fprintf(test_out, "orte_dss test11 failed\n");
    }

    fprintf(test_out, "executing test12\n");
    if (test12()) {
        fprintf(test_out, "Test12 succeeded\n");
    }
    else {
      fprintf(test_out, "orte_dss test12 failed\n");
    }

    fclose(test_out);

    orte_dss_close();

    opal_finalize();

    return(0);
}

static bool test1(void)        /* verify different buffer inits */
{
    orte_buffer_t *bufA;

    bufA = OBJ_NEW(orte_buffer_t);
    if (NULL == bufA) {
        fprintf(test_out, "orte_buffer failed init in OBJ_NEW\n");
        return false;
    }

    OBJ_RELEASE(bufA);
    if (NULL != bufA) {
        fprintf(test_out, "OBJ_RELEASE did not NULL the buffer pointer\n");
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
        fprintf(test_out, "orte_buffer failed init in OBJ_NEW\n");
        return false;
    }

    for (i=0;i<NUM_ITERS;i++) {
        rc = orte_dss.pack(bufA, src, NUM_ELEMS, ORTE_INT16);
        if (ORTE_SUCCESS != rc) {
            fprintf(test_out, "orte_dss.pack failed with return code %d\n", rc);
            return(false);
        }
    }

    for (i=0; i<NUM_ITERS; i++) {
        int j;
        size_t count;

        for(j=0; j<NUM_ELEMS; j++)
            dst[j] = -1;

        count = NUM_ELEMS;
        rc = orte_dss.unpack(bufA, dst, &count, ORTE_INT16);
        if (ORTE_SUCCESS != rc || count != NUM_ELEMS) {
            fprintf(test_out, "orte_dss.unpack failed with return code %d\n", rc);
            return(false);
        }

        for(j=0; j<NUM_ELEMS; j++) {
            if(src[j] != dst[j]) {
                fprintf(test_out, "test2: invalid results from unpack\n");
                return(false);
            }
        }
    }

    OBJ_RELEASE(bufA);
    if (NULL != bufA) {
        fprintf(test_out, "OBJ_RELEASE did not NULL the buffer pointer\n");
        return false;
    }

    return (true);
}

/*
 * OMPI_INT pack/unpack
 */
static bool test3(void)
{
    orte_buffer_t *bufA;
    int rc;
    int32_t i;
    int src[NUM_ELEMS];
    int dst[NUM_ELEMS];

    for(i=0; i<NUM_ELEMS; i++)
        src[i] = i;

    bufA = OBJ_NEW(orte_buffer_t);
    if (NULL == bufA) {
        fprintf(test_out, "orte_buffer failed init in OBJ_NEW\n");
        return false;
    }

    for (i=0;i<NUM_ITERS;i++) {
        rc = orte_dss.pack(bufA, src, NUM_ELEMS, ORTE_INT);
        if (ORTE_SUCCESS != rc) {
            fprintf(test_out, "orte_dss.pack failed with return code %d\n", rc);
            return(false);
        }
    }

    for (i=0; i<NUM_ITERS; i++) {
        int j;
        size_t count;

        for(j=0; j<NUM_ELEMS; j++)
            dst[j] = -1;

        count = NUM_ELEMS;
        rc = orte_dss.unpack(bufA, dst, &count, ORTE_INT);
        if (ORTE_SUCCESS != rc || count != NUM_ELEMS) {
            fprintf(test_out, "orte_dss.unpack failed with return code %d\n", rc);
            return(false);
        }

        for(j=0; j<NUM_ELEMS; j++) {
            if(src[j] != dst[j]) {
                fprintf(test_out, "test2: invalid results from unpack\n");
                return(false);
            }
        }
    }

    OBJ_RELEASE(bufA);
    if (NULL != bufA) {
        fprintf(test_out, "OBJ_RELEASE did not NULL the buffer pointer\n");
        return false;
    }

    return (true);
}

/*
 * OMPI_INT32 pack/unpack
 */
static bool test4(void)
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
        fprintf(test_out, "orte_buffer failed init in OBJ_NEW\n");
        return false;
    }

    for (i=0;i<NUM_ITERS;i++) {
        rc = orte_dss.pack(bufA, src, NUM_ELEMS, ORTE_INT32);
        if (ORTE_SUCCESS != rc) {
            fprintf(test_out, "orte_dss.pack failed with return code %d\n", rc);
            return(false);
        }
    }

    for (i=0; i<NUM_ITERS; i++) {
        int j;
        size_t count = NUM_ELEMS;

        for(j=0; j<NUM_ELEMS; j++)
            dst[j] = -1;

        rc = orte_dss.unpack(bufA, dst, &count, ORTE_INT32);
        if (ORTE_SUCCESS != rc || count != NUM_ELEMS) {
            fprintf(test_out, "orte_dss.unpack failed with return code %d\n", rc);
            return(false);
        }

        for(j=0; j<NUM_ELEMS; j++) {
            if(src[j] != dst[j]) {
                fprintf(test_out, "test2: invalid results from unpack\n");
                return(false);
            }
        }
    }

    OBJ_RELEASE(bufA);
    if (NULL != bufA) {
        fprintf(test_out, "OBJ_RELEASE did not NULL the buffer pointer\n");
        return false;
    }

    return (true);
}

/*
 * ORTE_INT64 pack/unpack
 */
static bool test5(void)
{
    orte_buffer_t *bufA;
    int rc;
    size_t i;
    int64_t src[NUM_ELEMS];
    int64_t dst[NUM_ELEMS];

    for(i=0; i<NUM_ELEMS; i++)
        src[i] = 1000*i;

    bufA = OBJ_NEW(orte_buffer_t);
    if (NULL == bufA) {
        fprintf(test_out, "orte_buffer failed init in OBJ_NEW\n");
        return false;
    }

    for (i=0;i<NUM_ITERS;i++) {
        rc = orte_dss.pack(bufA, src, NUM_ELEMS, ORTE_INT64);
        if (ORTE_SUCCESS != rc) {
            fprintf(test_out, "orte_dss.pack int64 failed with return code %d\n", rc);
            return(false);
        }
    }

    for (i=0; i<NUM_ITERS; i++) {
        int j;
        size_t count = NUM_ELEMS;

        for(j=0; j<NUM_ELEMS; j++)
            dst[j] = -1;

        rc = orte_dss.unpack(bufA, dst, &count, ORTE_INT64);
        if (ORTE_SUCCESS != rc || count != NUM_ELEMS) {
            fprintf(test_out, "orte_dss.unpack int64 failed with return code %d\n", rc);
            return(false);
        }

        for(j=0; j<NUM_ELEMS; j++) {
            if(src[j] != dst[j]) {
                fprintf(test_out, "test2: invalid results from unpack int64\n");
                return(false);
            }
        }
    }

    OBJ_RELEASE(bufA);
    if (NULL != bufA) {
        fprintf(test_out, "OBJ_RELEASE did not NULL the buffer pointer\n");
        return false;
    }

    return (true);
}

/*
 * OMPI_STRING pack/unpack
 */

static bool test6(void)
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
        fprintf(test_out, "orte_buffer failed init in OBJ_NEW\n");
        return false;
    }

    for (i=0;i<NUM_ITERS;i++) {
        rc = orte_dss.pack(bufA, src, NUM_ELEMS, ORTE_STRING);
        if (ORTE_SUCCESS != rc) {
            fprintf(test_out, "orte_dss.pack failed with return code %d\n", rc);
            return(false);
        }
    }

    for (i=0; i<NUM_ITERS; i++) {
        int j;
        size_t count = NUM_ELEMS;

        for(j=0; j<NUM_ELEMS; j++)
            dst[j] = NULL;

        rc = orte_dss.unpack(bufA, dst, &count, ORTE_STRING);
        if (ORTE_SUCCESS != rc || count != NUM_ELEMS) {
            fprintf(test_out, "orte_dss.unpack failed with return code %d\n", rc);
            return(false);
        }

        for(j=0; j<NUM_ELEMS; j++) {
            if(strcmp(src[j],dst[j]) != 0) {
                fprintf(test_out, "test4: invalid results from unpack\n");
                fprintf(test_out, "item %d src=[%s] len=%d dst=[%s] len=%d\n", j, src[j], (int)strlen(src[j]), dst[j], (int)strlen(dst[j]));
                return(false);
            }
        }
    }

    OBJ_RELEASE(bufA);
    if (NULL != bufA) {
        fprintf(test_out, "OBJ_RELEASE did not NULL the buffer pointer\n");
        return false;
    }

    return (true);
}


/**
 * OMPI_BOOL pack/unpack
 */

static bool test7(void)
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
        fprintf(test_out, "orte_buffer failed init in OBJ_NEW\n");
        return false;
    }

    for (i=0;i<NUM_ITERS;i++) {
        rc = orte_dss.pack(bufA, src, NUM_ELEMS, ORTE_BOOL);
        if (ORTE_SUCCESS != rc) {
            fprintf(test_out, "orte_dss.pack failed with return code %d\n", rc);
            return(false);
        }
    }

    for (i=0; i<NUM_ITERS; i++) {
        int j;
        size_t count = NUM_ELEMS;
        memset(dst,-1,sizeof(dst));

        rc = orte_dss.unpack(bufA, dst, &count, ORTE_BOOL);
        if (ORTE_SUCCESS != rc || count != NUM_ELEMS) {
            fprintf(test_out, "orte_dss.unpack failed with return code %d\n", rc);
            return(false);
        }

        for(j=0; j<NUM_ELEMS; j++) {
            if(src[j] != dst[j]) {
                fprintf(test_out, "test6: invalid results from unpack\n");
                return(false);
            }
        }
    }

    OBJ_RELEASE(bufA);
    if (NULL != bufA) {
        fprintf(test_out, "OBJ_RELEASE did not NULL the buffer pointer\n");
        return false;
    }

    return (true);
}

/**
 * OMPI_BYTE_OBJECT pack/unpack
 */

static bool test8(void)
{

    orte_buffer_t *bufA;
    int rc;
    int32_t i;
    orte_byte_object_t *src[NUM_ELEMS];
    orte_byte_object_t *dst[NUM_ELEMS];

    for(i=0; i<NUM_ELEMS; i++) {
        src[i] = (orte_byte_object_t*)malloc(sizeof(orte_byte_object_t));
        asprintf((char**)&(src[i]->bytes), "%d", i);
        src[i]->size = strlen((char*)(src[i]->bytes)) + 1;
    }

    bufA = OBJ_NEW(orte_buffer_t);
    if (NULL == bufA) {
        fprintf(test_out, "orte_buffer failed init in OBJ_NEW\n");
        return false;
    }

    for (i=0;i<NUM_ITERS;i++) {
        rc = orte_dss.pack(bufA, src, NUM_ELEMS, ORTE_BYTE_OBJECT);
        if (ORTE_SUCCESS != rc) {
            fprintf(test_out, "orte_dss.pack failed with return code %d\n", rc);
            return(false);
        }
    }

    for (i=0; i<NUM_ITERS; i++) {
        int j;
        size_t count = NUM_ELEMS;

        rc = orte_dss.unpack(bufA, dst, &count, ORTE_BYTE_OBJECT);
        if (ORTE_SUCCESS != rc || count != NUM_ELEMS) {
            fprintf(test_out, "orte_dss.unpack failed with return code %d\n", rc);
            return(false);
        }

        for(j=0; j<NUM_ELEMS; j++) {
            if(src[j]->size != dst[j]->size ||
               memcmp(src[j]->bytes,dst[j]->bytes,src[j]->size) != 0) {
                fprintf(test_out, "test7: invalid results from unpack\n");
                fprintf(test_out, "test7: object element %d has incorrect unpacked value\n", j);
                return(false);
            }
        }
    }

    /* cleanup */
    OBJ_RELEASE(bufA);
    if (NULL != bufA) {
        fprintf(test_out, "OBJ_RELEASE did not NULL the buffer pointer\n");
        return false;
    }

    return (true);
}

/**
 * ompi everything composite multipack/unpack
 */

static bool test9(void)
{

    orte_buffer_t *bufA;
    int rc;
    int32_t i;

    /* pack and unpack in this order */
    /* each block now has an offset to make debugging easier.. first block=100, 200,... */
    orte_byte_object_t *srco[NUM_ELEMS];
    orte_byte_object_t *dsto[NUM_ELEMS];
    char* srcs[NUM_ELEMS];
    char* dsts[NUM_ELEMS];
    bool srcb[NUM_ELEMS];
    bool dstb[NUM_ELEMS];
    int32_t src32[NUM_ELEMS];
    int32_t dst32[NUM_ELEMS];
    int16_t src16[NUM_ELEMS];
    int16_t dst16[NUM_ELEMS];

    for(i=0; i<NUM_ELEMS; i++) {
        /* object offset 100 */
        srco[i] = (orte_byte_object_t*)malloc(sizeof(orte_byte_object_t));
        asprintf((char**)&(srco[i]->bytes), "%d", i+100);
        srco[i]->size = strlen((char*)(srco[i]->bytes)) + 1;

        /* strings +200 */
        asprintf(&srcs[i], "%d", i+200);

        /* bool */
        srcb[i] = ((i % 2) == 0) ? true : false;

        /* INT32 +300 */
        src32[i] = i+300;

        /* INT16 +400 */
        src16[i] = i+400;
    }

    bufA = OBJ_NEW(orte_buffer_t);
    if (NULL == bufA) {
        fprintf(test_out, "orte_buffer failed init in OBJ_NEW\n");
        return false;
    }

    for (i=0;i<NUM_ITERS;i++) {
        /* object first */
        rc = orte_dss.pack(bufA, srco, NUM_ELEMS, ORTE_BYTE_OBJECT);
        if (ORTE_SUCCESS != rc) {
            fprintf(test_out, "orte_dss.pack on object failed with return code %d\n", rc);
            return(false);
        }
        /* STRING */
        rc = orte_dss.pack(bufA, srcs, NUM_ELEMS, ORTE_STRING);
        if (ORTE_SUCCESS != rc) {
            fprintf(test_out, "orte_dss.pack on string failed with return code %d\n", rc);
            return(false);
        }
        /* BOOL */
        rc = orte_dss.pack(bufA, srcb, NUM_ELEMS, ORTE_BOOL);
        if (ORTE_SUCCESS != rc) {
            fprintf(test_out, "orte_dss.pack on bool failed with return code %d\n", rc);
            return(false);
        }
        /* INT32 */
        rc = orte_dss.pack(bufA, src32, NUM_ELEMS, ORTE_INT32);
        if (ORTE_SUCCESS != rc) {
            fprintf(test_out, "orte_dss.pack on INT32 failed with return code %d\n", rc);
            return(false);
        }
        /* INT16 */
        rc = orte_dss.pack(bufA, src16, NUM_ELEMS, ORTE_INT16);
        if (ORTE_SUCCESS != rc) {
            fprintf(test_out, "orte_dss.pack on INT16 failed with return code %d\n", rc);
            return(false);
        }
    }

/*  fprintf(test_out,"test8:packed buffer info for STRING with %d iterations %d elements each\n", NUM_ITERS, NUM_ELEMS); */

    for (i=0; i<NUM_ITERS; i++) {
        int j;
        size_t count;

        /* string */
        for(j=0; j<NUM_ELEMS; j++) dsts[j] = NULL;
        /* bool */
        memset(dstb,-1,sizeof(dstb));
        /* int32 */
        for(j=0; j<NUM_ELEMS; j++) dst32[j] = -1;
        /* int16 */
        for(j=0; j<NUM_ELEMS; j++) dst16[j] = -1;


        /* object */
        count=NUM_ELEMS;
        rc = orte_dss.unpack(bufA, dsto, &count, ORTE_BYTE_OBJECT);
        if (ORTE_SUCCESS != rc || count != NUM_ELEMS) {
            fprintf(test_out, "orte_dss.unpack on object failed with return code %d\n", rc);
            return(false);
        }

        for(j=0; j<NUM_ELEMS; j++) {
            if(srco[j]->size != dsto[j]->size ||
               memcmp(srco[j]->bytes,dsto[j]->bytes,srco[j]->size) != 0) {
                fprintf(test_out, "test8: object element %d has incorrect unpacked value\n", j);
                return(false);
            }
        }

        /* string */
        count = NUM_ELEMS;
        rc = orte_dss.unpack(bufA, dsts, &count, ORTE_STRING);
        if (ORTE_SUCCESS != rc || count != NUM_ELEMS) {
            fprintf(test_out, "orte_dss.unpack on string failed with return code %d\n", rc);
            return(false);
        }

        for(j=0; j<NUM_ELEMS; j++) {
            if(strcmp(srcs[j],dsts[j]) != 0) {
                fprintf(test_out, "test8: invalid results from unpack\n");
                fprintf(test_out, "item %d src=[%s] len=%d dst=[%s] len=%d\n", j, srcs[j], (int)strlen(srcs[j]), dsts[j], (int)strlen(dsts[j]));
                return(false);
            }
        }

        /* bool */
        count = NUM_ELEMS;
        rc = orte_dss.unpack(bufA, dstb, &count, ORTE_BOOL);
        if (ORTE_SUCCESS != rc || count != NUM_ELEMS) {
            fprintf(test_out, "orte_dss.unpack on bool failed with return code %d\n", rc);
            return(false);
        }

        for(j=0; j<NUM_ELEMS; j++) {
            if(srcb[j] != dstb[j]) {
                fprintf(test_out, "test8: invalid results from unpack\n");
                return(false);
            }
        }

        /* int32 */
        count = NUM_ELEMS;
        rc = orte_dss.unpack(bufA, dst32, &count, ORTE_INT32);
        if (ORTE_SUCCESS != rc || count != NUM_ELEMS) {
            fprintf(test_out, "orte_dss.unpack on int32 failed with return code %d\n", rc);
            return(false);
        }

        for(j=0; j<NUM_ELEMS; j++) {
            if(src32[j] != dst32[j]) {
                fprintf(test_out, "test8: invalid results from unpack\n");
                return(false);
            }
        }

        /* int16 */
        count = NUM_ELEMS;
        rc = orte_dss.unpack(bufA, dst16, &count, ORTE_INT16);
        if (ORTE_SUCCESS != rc || count != NUM_ELEMS) {
            fprintf(test_out, "orte_dss.unpack on int16 failed with return code %d\n", rc);
            return(false);
        }

        for(j=0; j<NUM_ELEMS; j++) {
            if(src16[j] != dst16[j]) {
                fprintf(test_out, "test8: invalid results from unpack\n");
                return(false);
            }
        }


    } /* per iteration */

    OBJ_RELEASE(bufA);
    if (NULL != bufA) {
        fprintf(test_out, "OBJ_RELEASE did not NULL the buffer pointer\n");
        return false;
    }

    return (true);
}

/* ORTE_DATA_VALUE */
static bool test10(void)
{
    orte_buffer_t *bufA;
    int rc;
    int i;
    int16_t i16[NUM_ELEMS];
    orte_data_value_t *src[NUM_ELEMS];
    orte_data_value_t *dst[NUM_ELEMS];

    /* setup source array of data values */
    for(i=0; i<NUM_ELEMS; i++) {
        i16[i] = (int16_t)i;
        src[i] = OBJ_NEW(orte_data_value_t);
        src[i]->type = ((i % 2) == 0) ? ORTE_INT16 : ORTE_STRING;
        if (ORTE_INT16 == src[i]->type)
            src[i]->data = &i16[i];
        else
            src[i]->data = strdup("truly-a-dumb-test");
    }

    bufA = OBJ_NEW(orte_buffer_t);
    if (NULL == bufA) {
        fprintf(test_out, "orte_buffer failed init in OBJ_NEW\n");
        return false;
    }

    for (i=0;i<NUM_ITERS;i++) {
        rc = orte_dss.pack(bufA, src, NUM_ELEMS, ORTE_DATA_VALUE);
        if (ORTE_SUCCESS != rc) {
            fprintf(test_out, "orte_dss.pack failed with error code %d\n", rc);
            return(false);
        }
    }

    for (i=0; i<NUM_ITERS; i++) {
        int j;
        size_t count = NUM_ELEMS;
        memset(dst,-1,sizeof(dst));

        rc = orte_dss.unpack(bufA, dst, &count, ORTE_DATA_VALUE);
        if (ORTE_SUCCESS != rc || count != NUM_ELEMS) {
            fprintf(test_out,
            "orte_dss.unpack (DATA_VALUE) failed on iteration %d with error code %d\n",
                                i, rc);
            return(false);
        }

        for(j=0; j<NUM_ELEMS; j++) {
            if (src[j]->type != dst[j]->type) {
                fprintf(test_out, "orte_dss.unpack (DATA_VALUE) invalid results type mismatch from unpack\n");
                return(false);
            }
            if (0 != orte_dss.compare(src[j], dst[j], src[j]->type)) {
                fprintf(test_out, "orte_dss.unpack (DATA_VALUE) invalid results value mismatch from unpack");
                return(false);
            }
        }
    }

    OBJ_RELEASE(bufA);
    if (NULL != bufA) {
        fprintf(test_out, "OBJ_RELEASE did not NULL the buffer pointer\n");
        return false;
    }

    return (true);
}


/* size_t */
static bool test11(void)
{
    orte_buffer_t *bufA;
    int rc;
    size_t i;
    size_t src[NUM_ELEMS];
    size_t dst[NUM_ELEMS];

    for(i=0; i<NUM_ELEMS; i++)
        src[i] = 1000*i;

    bufA = OBJ_NEW(orte_buffer_t);
    if (NULL == bufA) {
        fprintf(test_out, "orte_buffer failed init in OBJ_NEW");
        fprintf(test_out, "OBJ_NEW failed\n");
        return false;
    }

    for (i=0;i<NUM_ITERS;i++) {
        rc = orte_dss.pack(bufA, src, NUM_ELEMS, ORTE_SIZE);
        if (ORTE_SUCCESS != rc) {
            fprintf(test_out, "orte_dss.pack size_t failed");
            fprintf(test_out, "orte_pack_size_t failed with return code %d\n", rc);
            return(false);
        }
    }

    for (i=0; i<NUM_ITERS; i++) {
        size_t j;
        size_t count;

        count = NUM_ELEMS;
        rc = orte_dss.unpack(bufA, dst, &count, ORTE_SIZE);
        if (ORTE_SUCCESS != rc || count != NUM_ELEMS) {
            fprintf(test_out, "orte_dss.unpack size_t failed");
            fprintf(test_out, "orte_unpack_size_t failed with return code %d\n", rc);
            return(false);
        }

        for(j=0; j<NUM_ELEMS; j++) {
            if(src[j] != dst[j]) {
                fprintf(test_out, "test2: invalid results from unpack size_t");
                return(false);
            }
        }
    }

    OBJ_RELEASE(bufA);
    if (NULL != bufA) {
        fprintf(test_out, "OBJ_RELEASE did not NULL the buffer pointer");
        fprintf(test_out, "OBJ_RELEASE did not NULL the buffer pointer");
        return false;
    }

    return (true);
}


/*
 * pid_t pack/unpack
 */
static bool test12(void)
{
    orte_buffer_t *bufA;
    int rc;
    size_t i;
    pid_t src[NUM_ELEMS];
    pid_t dst[NUM_ELEMS];

    for(i=0; i<NUM_ELEMS; i++)
        src[i] = (pid_t)i;

    bufA = OBJ_NEW(orte_buffer_t);
    if (NULL == bufA) {
        fprintf(test_out, "orte_buffer failed init in OBJ_NEW");
        fprintf(test_out, "OBJ_NEW failed\n");
        return false;
    }

    for (i=0;i<NUM_ITERS;i++) {
        rc = orte_dss.pack(bufA, src, NUM_ELEMS, ORTE_PID);
        if (ORTE_SUCCESS != rc) {
            fprintf(test_out, "orte_dss.pack failed");
            fprintf(test_out, "orte_pack pid_t failed with return code %d\n", rc);
            return(false);
        }
    }

    for (i=0; i<NUM_ITERS; i++) {
        size_t j;
        size_t count;

        count = NUM_ELEMS;
        rc = orte_dss.unpack(bufA, dst, &count, ORTE_PID);
        if (ORTE_SUCCESS != rc || count != NUM_ELEMS) {
            fprintf(test_out, "orte_dss.unpack failed");
            fprintf(test_out, "orte_pack pid_t failed with return code %d\n", rc);
            return(false);
        }

        for(j=0; j<NUM_ELEMS; j++) {
            if(src[j] != dst[j]) {
                fprintf(test_out, "test2: invalid results from unpack");
                return(false);
            }
        }
    }

    OBJ_RELEASE(bufA);
    if (NULL != bufA) {
        fprintf(test_out, "OBJ_RELEASE did not NULL the buffer pointer\n");
        return false;
    }

    return (true);
}

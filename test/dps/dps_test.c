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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif


#include "support.h"
#include "../src/mca/base/base.h"
#include "../src/dps/dps.h"

static void dump_buf (orte_buffer_t *buf);

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
//    test_out = fopen("orte_dps_test.txt", "w");
    test_out = stderr;
    
    /* setup the dps */
    mca_base_open();
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
    
    OBJ_RELEASE(bufA);
    if (NULL != bufA) {
        test_comment("OBJ_RELEASE did not NULL the buffer pointer");
        fprintf(test_out, "OBJ_RELEASE did not NULL the buffer pointer");
        return false;
    }

   return (true);

    
}

static bool test2(void)        /* verify we can pack ok */
{
    orte_buffer_t *bufA;
    int rc;
    int32_t i;

    bufA = OBJ_NEW(orte_buffer_t);
    if (NULL == bufA) {
        test_comment("orte_buffer failed init in OBJ_NEW");
        fprintf(test_out, "OBJ_NEW failed\n");
        return false;
    }
    
    for (i=0;i<100;i++) {
        rc = orte_dps.pack(bufA, &i, 1, ORTE_INT32);
        if (ORTE_SUCCESS != rc) {
            test_comment ("orte_pack_value failed");
            fprintf(test_out, "orte_pack_value failed with return code %d\n", rc);
            return(false);
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


static bool test3(void)          /* verify we can pack expanding buf */
{
    orte_buffer_t *bufA;
    int rc;
    int32_t i;
    char *desc;

    bufA = OBJ_NEW(orte_buffer_t);
    if (NULL == bufA) {
        test_comment("orte_buffer failed init in OBJ_NEW");
        fprintf(test_out, "OBJ_NEW failed\n");
        return false;
    }

    for (i=0;i<2000;i++) {
        rc = orte_dps.pack(bufA, &i, 1, ORTE_INT32);
        if (ORTE_SUCCESS != rc) {
            test_comment ("orte_pack_value expanding buf failed");
            fprintf(test_out, "orte_pack_value expanding buf failed with return code %d\n", rc);
            return(false);
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

static bool test4(void)        /* verify pack a byte object */
{
//    int rc;
//    orte_byte_object_t obj;
//
//    obj.size = 8192;
//    obj.bytes = (uint8_t*)malloc(8192);
//    if (NULL == obj.bytes) {
//        return false;
//    }
//    
//    rc = orte_dps.pack_value(bufA, &obj, NULL, ORTE_BYTE_OBJECT);
//    if (ORTE_SUCCESS != rc) {
//        test_comment ("orte_pack_value for byte_object failed");
//        fprintf(test_out, "orte_pack_value for byte_object failed with return code %d\n", rc);
//        return(false);
//    }
//    
//    rc = orte_dps.pack_value(bufB, &obj, "byte object", ORTE_BYTE_OBJECT);
//    if (ORTE_SUCCESS != rc) {
//        test_comment ("orte_pack_value for byte_object with desc failed");
//        fprintf(test_out, "orte_pack_value for byte_object with desc failed with return code %d\n", rc);
//        return(false);
//    }
//
    return (true);
}

static bool test5(void)        /* verify unpack INT32 */
{
//    int rc;
//    int i, j;
//    int *out;
//    char *desc;
//    orte_pack_type_t type;
//
//    for (i=0;i<2000;i++) {
//        j = i; /* for bufA */
//        rc = orte_dps.unpack_value(bufA, out, desc, &type);
//        if (ORTE_SUCCESS != rc) {
//            test_comment ("orte_unpack_value failed");
//            fprintf(test_out, "orte_unpack_value failed with return code %d\n", rc);
//            return(false);
//        }
//
//        if (ORTE_INT32 != type) {
//            test_comment ("orte_unpack_value failed - wrong type");
//            fprintf(test_out, "orte_unpack_value failed with wrong type %d\n", type);
//            return(false);
//        }
//            
//        if (*out != j) {
//            test_comment ("orte_unpack_value failed - value mismatch");
//            fprintf(test_out, "orte_unpack_value failed with value mismatch - got %d instead of %d\n", *out, j);
//            return(false);
//        }
//    }
//
//    for (i=0;i<2000;i++) {
//        j = i; /* for bufA */
//        rc = orte_dps.unpack_value(bufB, out, desc, &type);
//        if (ORTE_SUCCESS != rc) {
//            test_comment ("orte_unpack_value with desc failed");
//            fprintf(test_out, "orte_unpack_value with desc failed with return code %d\n", rc);
//            return(false);
//        }
//
//        if (ORTE_INT32 != type) {
//            test_comment ("orte_unpack_value with desc failed - wrong type");
//            fprintf(test_out, "orte_unpack_value with desc failed with wrong type %d\n", type);
//            return(false);
//        }
//            
//        if (*out != j) {
//            test_comment ("orte_unpack_value failed - value mismatch");
//            fprintf(test_out, "orte_unpack_value failed with value mismatch - got %d instead of %d\n", *out, j);
//            fprintf(test_out, "\tdescription: %s\n", desc);
//            return(false);
//        }
//    }
//
    return (true);
}


static bool test6(void)        /* verify string pack and unpack */
{
//
//    int rc;
//    char *str1;
//    char *str2;
//
//
//    rc = orte_dps.pack(bufA, "HELLO ", 1, ORTE_STRING);
//    if (OMPI_ERROR==rc) { test_comment ("ompi_pack_string failed"); return(false);}

    return (true);
}

static bool test7(void)        /* verify preallocated buffer init, pack and unpack */
{

    return (true);
}

static bool test8(void)        /* verify free */
{
    return (true);
}

static void dump_buf (orte_buffer_t *buf)
{
   int rc, i, out;

    fprintf(test_out, "Buffer Dump\n");
    fprintf(test_out, "pages: %d\tsize: %d\n", buf->pages, buf->size);
    fprintf(test_out, "len: %d\tspace: %d\n", buf->len, buf->space);
    fprintf(test_out, "toend: %d\n", buf->toend);
    
//    fprintf(test_out, "Packed data\n");
//    while (ORTE_SUCCESS == orte_dps.peek(buf, &type, &num_values)) {
//
//        orte_dps.unpack(buf, dest, &num_values, 
//   rc = 0;
//   i = 0;
//   while (1) {
//       rc = ompi_unpack (buf, &out, 1, OMPI_INT32);
//       if (rc==0) printf("%d[%d] ", i, out);
//       else {
//           printf("\n");
//           break;
//       }
//       i++;
//   }

   return;
}


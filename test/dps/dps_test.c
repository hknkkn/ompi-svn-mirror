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
#include "../src/dps/dps.h"

/* used for debugging */
/* int dump_buf (ompi_buffer_t buf); */

orte_buffer_t *bufA;
orte_buffer_t *bufB;


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
    int rc;

    rc = orte_dps.buffer_init (&bufA, "Test Buffer Init Label");
    if (ORTE_ERROR==rc) { test_comment ("orte_buffer_init with label failed"); return(false);}

    rc = orte_dps.buffer_init (&bufB, NULL);
    if (ORTE_ERROR==rc) { test_comment ("orte_buffer_init with no label failed"); return(false);}

    return (true);

    
}

static bool test2(void)        /* verify we can pack ok */
{
    int rc;
    int i;
    char *desc;

    for (i=0;i<100;i++) {
        rc = orte_dps.pack_value(bufA, &i, NULL, ORTE_INT32);
        if (ORTE_SUCCESS != rc) {
            test_comment ("orte_pack_value failed");
            fprintf(test_out, "orte_pack_value failed with return code %d\n", rc);
            return(false);
        }
    }

    for (i=0; i<100; i++) {
        asprintf(&desc, "value %d", i);
        rc = orte_dps.pack_value(bufB, &i, desc, ORTE_INT32);
        free(desc);
        if (ORTE_SUCCESS != rc) {
            test_comment ("orte_pack_value with descriptions failed");
            fprintf(test_out, "orte_pack_value with descriptions failed with return code %d\n", rc);
            return(false);
        }
    }
    
    return (true);
}


static bool test3(void)          /* verify we can pack expanding buf */
{
    int rc;
    int i;
    char *desc;

    for (i=100;i<2000;i++) {
        rc = orte_dps.pack_value(bufA, &i, NULL, ORTE_INT32);
        if (ORTE_SUCCESS != rc) {
            test_comment ("orte_pack_value expanding buf failed");
            fprintf(test_out, "orte_pack_value expanding buf failed with return code %d\n", rc);
            return(false);
        }
    }

    for (i=100;i<2000;i++) {
        asprintf(&desc, "value %d", i);
        rc = orte_dps.pack_value(bufB, &i, desc, ORTE_INT32);
        free(desc);
        if (ORTE_SUCCESS != rc) {
            test_comment ("orte_pack_value expanding buf with desc failed");
            fprintf(test_out, "orte_pack_value expanding buf with desc failed with return code %d\n", rc);
            return(false);
        }
    }

    return (true);
}

static bool test4(void)        /* verify pack a byte object */
{
    int rc;
    orte_byte_object_t obj;

    obj.size = 8192;
    obj.bytes = (uint8_t*)malloc(8192);
    if (NULL == obj.bytes) {
        return false;
    }
    
    rc = orte_dps.pack_value(bufA, &obj, NULL, ORTE_BYTE_OBJECT);
    if (ORTE_SUCCESS != rc) {
        test_comment ("orte_pack_value for byte_object failed");
        fprintf(test_out, "orte_pack_value for byte_object failed with return code %d\n", rc);
        return(false);
    }
    
    rc = orte_dps.pack_value(bufB, &obj, "byte object", ORTE_BYTE_OBJECT);
    if (ORTE_SUCCESS != rc) {
        test_comment ("orte_pack_value for byte_object with desc failed");
        fprintf(test_out, "orte_pack_value for byte_object with desc failed with return code %d\n", rc);
        return(false);
    }

    return (true);
}

static bool test5(void)        /* verify unpack INT32 */
{
    int rc;
    int i, j;
    int *out;
    char *desc;
    orte_pack_type_t type;

    for (i=0;i<2000;i++) {
        j = i; /* for bufA */
        rc = orte_dps.unpack_value(bufA, out, desc, &type);
        if (ORTE_SUCCESS != rc) {
            test_comment ("orte_unpack_value failed");
            fprintf(test_out, "orte_unpack_value failed with return code %d\n", rc);
            return(false);
        }

        if (ORTE_INT32 != type) {
            test_comment ("orte_unpack_value failed - wrong type");
            fprintf(test_out, "orte_unpack_value failed with wrong type %d\n", type);
            return(false);
        }
            
        if (*out != j) {
            test_comment ("orte_unpack_value failed - value mismatch");
            fprintf(test_out, "orte_unpack_value failed with value mismatch - got %d instead of %d\n", *out, j);
            return(false);
        }
    }

    for (i=0;i<2000;i++) {
        j = i; /* for bufA */
        rc = orte_dps.unpack_value(bufB, out, desc, &type);
        if (ORTE_SUCCESS != rc) {
            test_comment ("orte_unpack_value with desc failed");
            fprintf(test_out, "orte_unpack_value with desc failed with return code %d\n", rc);
            return(false);
        }

        if (ORTE_INT32 != type) {
            test_comment ("orte_unpack_value with desc failed - wrong type");
            fprintf(test_out, "orte_unpack_value with desc failed with wrong type %d\n", type);
            return(false);
        }
            
        if (*out != j) {
            test_comment ("orte_unpack_value failed - value mismatch");
            fprintf(test_out, "orte_unpack_value failed with value mismatch - got %d instead of %d\n", *out, j);
            fprintf(test_out, "\tdescription: %s\n", desc);
            return(false);
        }
    }

    return (true);
}


static bool test6(void)        /* verify string pack and unpack */
{

    int rc;
    char *str1;
    char *str2;


    rc = orte_dps.pack_value(bufA, "HELLO ", NULL, ORTE_STRING);
    if (OMPI_ERROR==rc) { test_comment ("ompi_pack_string failed"); return(false);}

    rc = ompi_pack_string (bufB, "WORLD!");
    if (OMPI_ERROR==rc) { test_comment ("ompi_pack_string failed"); return(false);}

    rc = ompi_pack (bufC, bufA, 1, OMPI_PACKED);
    if (OMPI_ERROR==rc) { test_comment ("ompi_pack failed"); return(false);}
    rc = ompi_pack (bufC, bufB, 1, OMPI_PACKED);
    if (OMPI_ERROR==rc) { test_comment ("ompi_pack failed"); return(false);}

 /* we now have a buffer with two strings in it */
    rc = ompi_unpack_string (bufC, &str1);
    if (OMPI_ERROR==rc) { test_comment ("ompi_unpack_string failed"); return(false);}

    rc = strcmp ("HELLO ", str1);
  if (rc) { test_comment ("strcmp returns no zero value."); return (false); }

    rc = ompi_unpack_string (bufC, &str2);
    if (OMPI_ERROR==rc) { test_comment ("ompi_unpack_string failed"); return(false);}

 rc = strcmp ("WORLD!", str2);
  if (rc) { test_comment ("strcmp returns no zero value."); return (false); }


    rc = ompi_buffer_free (bufA);
    if (OMPI_ERROR==rc) { test_comment ("ompi_buffer_free failed"); return(false);}

    rc = ompi_buffer_free (bufB);
    if (OMPI_ERROR==rc) { test_comment ("ompi_buffer_free failed"); return(false);}

    rc = ompi_buffer_free (bufC);
    if (OMPI_ERROR==rc) { test_comment ("ompi_buffer_free failed"); return(false);}

 if (str1) { free (str1); }
 if (str2) { free (str2); }

    return (true);
}

static bool test7(void)        /* verify preallocated buffer init, pack and unpack */
{

    int rc;
    int i, j, out;
    char *mybuf;
    int *p;


    return (true);
}

static bool test8(void)        /* verify free */
{
    int rc;

    rc = orte_dps.buffer_free (&bufA);
    if (ORTE_SUCCESS != rc || NULL != bufA) {
        test_comment ("orte_dps.buffer_free failed");
        fprintf(test_out, "orte_dps.buffer_free failed with return code %d", rc);
        if (NULL == bufA) {
            fprintf(test_out, "\tbuffer pointer NULL'd\n");
        } else {
            fprintf(test_out, "\tbuffer pointer was NOT NULL'd\n");
        }
        return(false);
    }

    rc = orte_dps.buffer_free (&bufB);
    if (ORTE_SUCCESS != rc || NULL != bufB) {
        test_comment ("orte_dps.buffer_free with described data and label failed");
        fprintf(test_out, "orte_dps.buffer_free with described data and label failed with return code %d", rc);
        if (NULL == bufA) {
            fprintf(test_out, "\tbuffer pointer NULL'd\n");
        } else {
            fprintf(test_out, "\tbuffer pointer was NOT NULL'd\n");
        }
        return(false);
    }

    return (true);
}

/* int dump_buf (ompi_buffer_t buf) */
/* { */
/* int rc, i, out; */
/*  */
/* rc = 0; */
/* i = 0; */
/* while (1) { */
/*     rc = ompi_unpack (buf, &out, 1, OMPI_INT32); */
/*     if (rc==0) printf("%d[%d] ", i, out); */
/*     else { */
/*         printf("\n"); */
/*         break; */
/*     } */
/*     i++; */
/* } */
/*  */
/* return (i); */
/* } */


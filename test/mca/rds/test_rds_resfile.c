/*
 * unit test for RDS resource file parser

 --------------------------------------------------------------------------

 Authors:    Ralph H. Castain <rhc@lanl.gov>

 --------------------------------------------------------------------------

*/

#include "orte_config.h"
#include <stdio.h>
#include <string.h>

#include "support.h"

#include "include/orte_constants.h"

/* output files needed by the test */
static FILE *test_out=NULL;

static char *cmd_str="diff ./test_ns_replica_out ./test_ns_replica_out_std";


int
main(int argc, char **argv)
{
    test_init("test_ns_replica");

    test_out = fopen( "test_ns_replica_out", "w+" );
    if( test_out == NULL ) {
      test_failure("test_ns_replica couldn't open test file failed");
      test_finalize();
      exit(1);
    } 

    /* startup the MCA */
    if (OMPI_SUCCESS == mca_base_open()) {
        fprintf(test_out, "MCA started\n");
    } else {
        fprintf(test_out, "MCA could not start - please report error to bugs@open-mpi.org\n");
        exit (1);
    }

}
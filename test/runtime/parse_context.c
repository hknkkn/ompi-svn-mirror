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
 * parse_context - unit test
 *
 */

/*
 * includes
 */

#include "orte_config.h"

#include "support.h"

#include "util/cmd_line.h"
#include "mca/base/base.h"
#include "mca/base/mca_base_param.h"

#include "runtime/runtime.h"

/* output files needed by the test */
static FILE *test_out=NULL;

static char *cmd_str="diff ./test_parse_context_out ./test_parse_context_out_std";


int main(int argc, char **argv)
{
    int ret;
    ompi_cmd_line_t *cmd_line;
    
    test_init("test_gpr_replica");

   /*  test_out = fopen( "test_parse_context_out", "w+" ); */
    test_out = stderr;
    if( test_out == NULL ) {
      test_failure("parse_context_test couldn't open test file failed");
      test_finalize();
      exit(1);
    } 

    cmd_line = OBJ_NEW(ompi_cmd_line_t);

    ompi_init(argc, argv);

    /* startup the MCA */
    if (OMPI_SUCCESS == mca_base_open()) {
        fprintf(test_out, "MCA started\n");
    } else {
        fprintf(test_out, "MCA could not start\n");
        exit (1);
    }

    /*
     * setup  mca command line arguments
     */
    if (OMPI_SUCCESS != (ret = mca_base_cmd_line_setup(cmd_line))) {
        fprintf(test_out, "mca_cmd_line_setup: failed\n");
        exit (ret);
    }

    if (OMPI_SUCCESS != mca_base_cmd_line_process_args(cmd_line)) {
        fprintf(test_out, "mca_cmd_line_process_args: failed\n");
        exit (ret);
    }

    /* parse the cmd line */
    if (OMPI_SUCCESS != ompi_cmd_line_parse(cmd_line, true, argc, argv)) {
        fprintf(test_out, "cmd_line_parse: failed\n");
        exit(1);
    }

    /* set some MCA parameters in environment */
    if (ORTE_SUCCESS != orte_parse_proc_context(cmd_line, argc, argv)) {
       fprintf(test_out, "parse_proc_context: failed\n");
       exit(1);
    }
    
    /* parse them */
    
    test_finalize();

    return(0);
}

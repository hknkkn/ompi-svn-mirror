/*
 * $HEADER
 */

#include "ompi_config.h"
#include "util/common_cmd_line.h"


/*
 * Private variables
 */
static bool setup = false;

/*
 * The global variable that everyone accesses
 */
ompi_cmd_line_t *ompi_common_cmd_line = NULL;


/*
 * Setup the ompi_common_cmd_line variable
 */
int ompi_common_cmd_line_init(int argc, char **argv)
{
  if (!setup) {
    ompi_common_cmd_line = ompi_cmd_line_create();

    /* Setup the options that are allowable */

    ompi_cmd_line_make_opt(ompi_common_cmd_line, 'v', "version", 0,
			   "Show version of Open MPI and this program");

    ompi_cmd_line_make_opt(ompi_common_cmd_line, 'h', "help", 0,
			   "Show help for this function");


    /* Parse the command line */

    if (OMPI_SUCCESS != ompi_cmd_line_parse(ompi_common_cmd_line, true, argc, argv)) {
	return OMPI_ERROR;
    }

    /* Done */

    setup = true;
  }

  return OMPI_SUCCESS;
}


/*
 * Free resources associated with the ompi_common_cmd_line variable
 */
int ompi_common_cmd_line_finalize(void)
{
  if (setup) {
    ompi_cmd_line_free(ompi_common_cmd_line);
    setup = false;
  }

  return OMPI_SUCCESS;
}

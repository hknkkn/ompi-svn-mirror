/*
 * $HEADER$
 */

/**
 * @file
 *
 * Setup command line options for the Open MPI Run Time Environment
 */


#include "ompi_config.h"

#include <string.h>
#include <sys/time.h>

#include "include/constants.h"
#include "util/output.h"
#include "util/sys_info.h"
#include "util/proc_info.h"
#include "util/os_path.h"
#include "util/pack.h"
#include "util/session_dir.h"
#include "util/universe_setup_file_io.h"

#include "mca/oob/base/base.h"
#include "mca/ns/base/base.h"

#include "runtime/runtime.h"


static struct timeval ompi_rte_ping_wait = {30, 0};


int ompi_rte_local_universe_exists()
{
    char *contact_file;
    int ret;
    ompi_process_name_t seed={0,0,0};

    /* does universe already exist on local host? Check session directory to see */
    if (ompi_rte_debug_flag) {
	ompi_output(0, "checking local universe existence: universe %s", ompi_universe_info.name);
    }

    if (0 != strncmp(ompi_universe_info.host, ompi_system_info.nodename, strlen(ompi_system_info.nodename))) { /* remote host specified */
	ompi_output(0, "remote hosts not supported");
	return OMPI_ERR_NOT_IMPLEMENTED;
    }

    /* check to see if local universe already exists */
    if (OMPI_SUCCESS == ompi_session_dir(false,
					 ompi_process_info.tmpdir_base,
					 ompi_system_info.user,
					 ompi_system_info.nodename,
					 NULL,
					 ompi_universe_info.name,
					 NULL,
					 NULL)) { /* found */

	/* check for "contact-info" file. if present, read it in. */
	contact_file = ompi_os_path(false, ompi_process_info.universe_session_dir,
				    "universe-setup.txt", NULL);

	if (OMPI_SUCCESS != (ret = ompi_read_universe_setup_file(contact_file))) {
	    ompi_output(0, "could not read contact file %s", contact_file);
	    return ret;
	}

	if (!ompi_universe_info.persistence ||   /* not persistent... */
	    (0 == strncmp(ompi_universe_info.scope, "exclusive", strlen("exclusive")))) {  /* ...or no connection allowed */
	    /* also need to check "local" and that we did not specify the exact
	     * matching universe name
	     */
	    ompi_output(0, "connection not allowed");
	    return OMPI_ERR_NO_CONNECTION_ALLOWED;
	}

	/* if persistent, set contact info... */
	if (OMPI_SUCCESS != mca_oob_set_contact_info(ompi_universe_info.oob_contact_info)) { /* set contact info */
	    ompi_output(0, "error setting oob contact info - please report error to bugs@open-mpi.org\n");
	    return OMPI_ERR_FATAL;
	}

	mca_oob_parse_contact_info(ompi_universe_info.oob_contact_info, &seed, NULL);
	/* ...and ping to verify it's alive */
	if (OMPI_SUCCESS != mca_oob_ping(&seed, &ompi_rte_ping_wait)) {
	    ompi_output(0, "ping failed");
	    return OMPI_ERR_CONNECTION_FAILED;
	}

	/* set the my_universe field */
	ompi_process_info.my_universe = strdup(ompi_universe_info.name);

	return OMPI_SUCCESS;
    }

    return OMPI_ERR_NOT_FOUND;
}

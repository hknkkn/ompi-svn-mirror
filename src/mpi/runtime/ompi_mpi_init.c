/*
 * $HEADER$
 */

#include "ompi_config.h"

#include "include/constants.h"
#include "mpi/runtime/mpiruntime.h"
#include "mpi/runtime/params.h"
#include "runtime/runtime.h"
#include "util/sys_info.h"
#include "util/proc_info.h"
#include "mpi.h"
#include "communicator/communicator.h"
#include "group/group.h"
#include "info/info.h"
#include "util/show_help.h"
#include "errhandler/errhandler.h"
#include "errhandler/errcode.h"
#include "errhandler/errclass.h"
#include "errhandler/errcode-internal.h"
#include "op/op.h"
#include "file/file.h"

#include "mca/base/base.h"
#include "mca/base/base.h"
#include "mca/allocator/base/base.h"
#include "mca/allocator/allocator.h"
#include "mca/mpool/base/base.h"
#include "mca/mpool/mpool.h"
#include "mca/ptl/ptl.h"
#include "mca/ptl/base/base.h"
#include "mca/pml/pml.h"
#include "mca/pml/base/base.h"
#include "mca/coll/coll.h"
#include "mca/coll/base/base.h"
#include "mca/topo/topo.h"
#include "mca/topo/base/base.h"
#include "mca/io/io.h"
#include "mca/io/base/base.h"
#include "mca/oob/base/base.h"
#include "mca/ns/base/base.h"

#include "runtime/runtime.h"


/*
 * Global variables and symbols for the MPI layer
 */

bool ompi_mpi_initialized = false;
bool ompi_mpi_finalized = false;

bool ompi_mpi_thread_multiple = false;
int ompi_mpi_thread_requested = MPI_THREAD_SINGLE;
int ompi_mpi_thread_provided = MPI_THREAD_SINGLE;


int ompi_mpi_init(int argc, char **argv, int requested, int *provided)
{
    int ret, param;
    bool allow_multi_user_threads;
    bool have_hidden_threads;
    ompi_proc_t** procs;
    size_t nprocs;
    ompi_cmd_line_t *cmd_line=NULL;
    char *contact=NULL, **tmp, *nsreplica=NULL, *gprreplica=NULL;
    char *error;

    /* Become a OMPI process */

    if (OMPI_SUCCESS != (ret = ompi_init(argc, argv))) {
        error = "ompi_init() failed";
        goto error;
    }

    /* Open up the MCA */

    if (OMPI_SUCCESS != (ret = mca_base_open())) {
        error = "mca_base_open() failed";
        goto error;
    }

    /* Join the run-time environment */
    allow_multi_user_threads = true;
    have_hidden_threads = false;
    if (OMPI_SUCCESS != (ret = ompi_rte_init_stage1(&allow_multi_user_threads,
						    &have_hidden_threads))) {
	return ret;
    }

    /* setup rte command line arguments */
    cmd_line = OBJ_NEW(ompi_cmd_line_t);
    ompi_rte_cmd_line_setup(cmd_line);

    /* parse the rte command line arguments */
    if (OMPI_SUCCESS != ompi_cmd_line_parse(cmd_line, true, argc, argv)) {
	exit(ret);
    }

    if (ompi_cmd_line_is_taken(cmd_line, "initcontact")) {
	if (NULL == (contact = ompi_cmd_line_get_param(cmd_line, "initcontact", 0, 0))) {
	    return OMPI_ERROR;
	}
	mca_oob_set_contact_info(contact);
    }

    if (ompi_cmd_line_is_taken(cmd_line, "nsreplica")) {
	if (NULL == (nsreplica = ompi_cmd_line_get_param(cmd_line, "nsreplica", 0, 0))) {
	    return OMPI_ERROR;
	}
	mca_oob_set_contact_info(nsreplica);
	ompi_process_info.ns_replica = ns_base_create_process_name(0,0,0);  /* allocate a space */
        mca_oob_parse_contact_info(nsreplica, ompi_process_info.ns_replica, NULL);
    } else {
	ompi_process_info.ns_replica = NULL;
    }

    if (ompi_cmd_line_is_taken(cmd_line, "gprreplica")) {
	if (NULL == (gprreplica = ompi_cmd_line_get_param(cmd_line, "gprreplica", 0, 0))) {
	    return OMPI_ERROR;
	}
	mca_oob_set_contact_info(gprreplica);
	ompi_process_info.gpr_replica = ns_base_create_process_name(0,0,0);  /* allocate a space */
	mca_oob_parse_contact_info(gprreplica, ompi_process_info.gpr_replica, NULL);
    } else {
	ompi_process_info.gpr_replica = NULL;
    }

    /* start the rest of the rte */
      if (OMPI_SUCCESS != (ret = ompi_rte_init_stage2(&allow_multi_user_threads,
						    &have_hidden_threads))) {
        error = "mca_rte_init() failed";
        goto error;
    }

    /* Once we've joined the RTE, see if any MCA parameters were
       passed to the MPI level */

    if (OMPI_SUCCESS != (ret = ompi_mpi_register_params())) {
        error = "mca_mpi_register_params() failed";
        goto error;
    }

    /* initialize ompi procs */
    if (OMPI_SUCCESS != (ret = ompi_proc_init())) {
        error = "mca_proc_init() failed";
        goto error;
    }

    /* Open up relevant MCA modules. */

    if (OMPI_SUCCESS != (ret = mca_allocator_base_open())) {
        error = "mca_allocator_base_open() failed";
        goto error;
    }
    if (OMPI_SUCCESS != (ret = mca_mpool_base_open())) {
        error = "mca_mpool_base_open() failed";
        goto error;
    }
    if (OMPI_SUCCESS != (ret = mca_pml_base_open())) {
        error = "mca_pml_base_open() failed";
        goto error;
    }
    if (OMPI_SUCCESS != (ret = mca_ptl_base_open())) {
        error = "mca_ptl_base_open() failed";
        goto error;
    }
    if (OMPI_SUCCESS != (ret = mca_coll_base_open())) {
        error = "mca_coll_base_open() failed";
        goto error;
    }
    if (OMPI_SUCCESS != (ret = mca_topo_base_open())) {
        error = "mca_topo_base_open() failed";
        goto error;
    }
    if (OMPI_SUCCESS != (ret = mca_io_base_open())) {
        error = "mca_io_base_open() failed";
        goto error;
    }

    /* Select which pml, ptl, and coll modules to use, and determine the
         final thread level */

    if (OMPI_SUCCESS != 
            (ret = mca_base_init_select_components(requested, 
                                                   allow_multi_user_threads,
                                                   have_hidden_threads, 
                                                   provided))) {
        error = "mca_base_init_select_components() failed";
        goto error;
    }

    /* initialize info */
    if (OMPI_SUCCESS != (ret = ompi_info_init())) {
        error = "ompi_info_init() failed";
        goto error;
    }
    /* initialize error handlers */
    if (OMPI_SUCCESS != (ret = ompi_errhandler_init())) {
        error = "ompi_errhandler_init() failed";
        goto error;
    }

    /* initialize error codes */
    if (OMPI_SUCCESS != (ret = ompi_mpi_errcode_init())) {
        error = "ompi_mpi_errcode_init() failed";
        goto error;
    }

    /* initialize error classes */
    if (OMPI_SUCCESS != (ret = ompi_errclass_init())) {
        error = "ompi_errclass_init() failed";
        goto error;
    }
    
    /* initialize internal error codes */
    if (OMPI_SUCCESS != (ret = ompi_errcode_intern_init())) {
        error = "ompi_errcode_intern_init() failed";
        goto error;
    }
     
    /* initialize groups  */
    if (OMPI_SUCCESS != (ret = ompi_group_init())) {
        error = "ompi_group_init() failed";
        goto error;
    }

    /* initialize attribute meta-data structure for comm/win/dtype */
    if (OMPI_SUCCESS != (ret = ompi_attr_init())) {
        error = "ompi_attr_init() failed";
        goto error;
    }

    /* initialize communicators */
    if (OMPI_SUCCESS != (ret = ompi_comm_init())) {
        error = "ompi_comm_init() failed";
        goto error;
    }

    /* initialize datatypes */
    if (OMPI_SUCCESS != (ret = ompi_ddt_init())) {
        error = "ompi_ddt_init() failed";
        goto error;
    }

    /* initialize ops */
    if (OMPI_SUCCESS != (ret = ompi_op_init())) {
        error = "ompi_op_init() failed";
        goto error;
    }

    /* initialize file handles */
    if (OMPI_SUCCESS != (ret = ompi_file_init())) {
        error = "ompi_file_init() failed";
        goto error;
    }

    /* do module exchange */
    if (OMPI_SUCCESS != (ret = mca_base_modex_exchange())) {
        error = "ompi_base_modex_exchange() failed";
        goto error;
    }

    /* add all ompi_proc_t's to PML */
    if (NULL == (procs = ompi_proc_world(&nprocs))) {
        error = "ompi_proc_world() failed";
        goto error;
    }
    if (OMPI_SUCCESS != (ret = mca_pml.pml_add_procs(procs, nprocs))) {
        free(procs);
        error = "PML add procs failed";
        goto error;
    }
    free(procs);

    /* start PTL's */
    param = 1;
    if (OMPI_SUCCESS != 
        (ret = mca_pml.pml_control(MCA_PTL_ENABLE, &param, sizeof(param)))) {
        error = "PML control failed";
        goto error;
    }

    /* save the resulting thread levels */

    ompi_mpi_thread_requested = requested;
    *provided = ompi_mpi_thread_provided;
    ompi_mpi_thread_multiple = (ompi_mpi_thread_provided == 
                                MPI_THREAD_MULTIPLE);

    /* Init coll for the comms */

    if (OMPI_SUCCESS != 
        (ret = mca_coll_base_comm_select(MPI_COMM_SELF, NULL))) {
        error = "mca_coll_base_comm_select(MPI_COMM_SELF) failed";
        goto error;
    }

    if (OMPI_SUCCESS !=
        (ret = mca_coll_base_comm_select(MPI_COMM_WORLD, NULL))) {
        error = "mca_coll_base_comm_select(MPI_COMM_WORLD) failed";
        goto error;
    }

    /* Wait for everyone to initialize */

    if (MPI_SUCCESS != (ret = 
                        MPI_COMM_WORLD->c_coll.coll_barrier(MPI_COMM_WORLD))) {
        error = "Barrier over MPI_COMM_WORLD failed";
        goto error;
    }

 error:
    if (ret != OMPI_SUCCESS) {
        ompi_show_help("help-mpi-runtime",
                       "mpi_init:startup:internal-failure", true,
                       "MPI_INIT", "MPI_INIT", error, ret);
        return ret;
    }

    /* All done */

    ompi_mpi_initialized = true;
    ompi_mpi_finalized = false;
    return MPI_SUCCESS;
}

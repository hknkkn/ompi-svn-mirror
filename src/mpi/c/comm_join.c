/*
 * $HEADERS$
 */
#include "ompi_config.h"
#include <stdio.h>

#include "mpi.h"
#include "mpi/c/bindings.h"
#include "runtime/runtime.h"
#include "communicator/communicator.h"

#if OMPI_HAVE_WEAK_SYMBOLS && OMPI_PROFILING_DEFINES
#pragma weak MPI_Comm_join = PMPI_Comm_join
#endif

#if OMPI_PROFILING_DEFINES
#include "mpi/c/profile/defines.h"
#endif

int MPI_Comm_join(int fd, MPI_Comm *intercomm) 
{
    int rc;
    ompi_proc_t rproc;
    uint32_t lleader=0; /* OOB contact information of our root */
    ompi_communicator_t *comp, *newcomp;

    comp = (ompi_communicator_t *)MPI_COMM_SELF;

    if ( MPI_PARAM_CHECK ) {
        if ( ompi_mpi_finalized )
            return OMPI_ERRHANDLER_INVOKE(MPI_COMM_WORLD, MPI_ERR_INTERN, 
                                          "MPI_Comm_join");
        if ( NULL == intercomm ) 
            return OMPI_ERRHANDLER_INVOKE(MPI_COMM_WORLD, MPI_ERR_ARG, 
                                          "MPI_Comm_join");  
    }
    
    /* sendrecv OOB-name (port-name) through the socket connection.
       Need to determine somehow how to avoid a potential deadlock
       here. *o/
    /* if proc unknown, set up the proc-structure */

    /* setup the intercomm-structure using ompi_comm_set (); */
    newcomp = ompi_comm_set ( comp,                                   /* old comm */
                              comp->c_local_group->grp_proc_count,    /* local_size */
                              comp->c_local_group->grp_proc_pointers, /* local_procs*/
                              1,                                      /* remote_size */
                              &rproc,                                 /* remote_procs */
                              NULL,                                   /* attrs */
                              comp->error_handler,                    /* error handler */
                              NULL,                                   /* coll module */
                              NULL                                    /* topo module */
                              );

    /* setup comm_cid */
    rc = ompi_comm_nextcid ( newcomp,                  /* new comm */ 
                             comp,                     /* old comm */
                             NULL,                     /* bridge comm */
                             &lleader,                /* local leader */
                             &rproc,                   /* remote_leader */
                             OMPI_COMM_CID_INTRA_OOB); /* mode */
    if ( OMPI_SUCCESS != rc ) {
        return OMPI_ERRHANDLER_INVOKE(MPI_COMM_SELF, rc, "MPI_Comm_join");
    }


    /* PROBLEM: do we have to re-start some low level stuff
       to enable the usage of fast communication devices
       between the two worlds ? */
    
    *intercomm = newcomp;
    return MPI_SUCCESS;
}

/* -*- C -*-
 *
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
/**
 * @file
 *
 * The Open RTE Process Launch Subsystem
 * 
 * The process launch subsystem (PLS) is responsible for actually
 * launching a specified application's processes across the indicated
 * resource. The PLS is invoked by the controlling program (mpirun or
 * whatever) after the resource discovery, allocation, and mapping
 * subsystems have performed their work. Thus, the PLS can assume that
 * certain data structures have been created, and that some data MAY
 * be present - the PLS must also be capable of appropriately dealing
 * with situations where earlier subsystems may not have access to
 * complete information. For example, while the discovery subsystem
 * (RDS) will provide information on the launcher used by a particular
 * resource, that information may NOT have been provided and hence may
 * not be available when the PLS is invoked. Thus, the PLS components
 * must include the ability to sense their environment where
 * necessary.
 * 
 * The PLS obtains its input information from several sources:
 * 
 * - the ORTE_JOB_SEGMENT of the registry. Information on this segment
 * includes: the application to be executed; the number of processes
 * of each application to be run; the context (argv and enviro arrays)
 * for each process.
 * 
 * - the ORTE_RESOURCE_SEGMENT of the registry. This includes:
 * identification of the launcher to be used on the indicated
 * resource; location of temporary directory and other filesystem
 * directory locations;
 * 
 * - MCA parameters. This includes any directive from the user as to
 * the launcher to be used and/or its configuration.
 * 
 * The PLS uses this information to launch the processes upon the
 * indicated resource(s).  PLS components are free to ignore
 * information that is not pertinent to their operation. For example,
 * although the user may have specified a particular mapping of
 * process to nodename, a PLS launching the application on a resource
 * that does not permit such specifications would ignore the
 * corresponding information that the mapper placed on the registry -
 * it is irrelevant to that launcher's operation (although a warning
 * to the user, in this case, might be appropriate).
 * 
 * The PLS is tightly coupled to the PLSNDS - the PLS name discovery
 * service - that each process uses to "discover" its official
 * name. Each PLS MUST:
 * 
 * - set the environmental parameter OMPI_MCA_PLS_LAUNCHER to indicate
 * the launcher used to spawn the process.
 *
 * <JMS>
 * Should this really be the "pls_launcher" MCA parameter?
 *
 * What should it contain, the component name (i.e., a string
 * exactly maching the component.mca_component_name field?)?  If so,
 * why doesn't the pls framework (or the rmgr) do this?  Seems kinda
 * odd to make every component do exactly the same thing when the RMGR
 * assumedly has this information...?
 * </JMS>
 * 
 * - have a corresponding entry in the orte_plsnds table (defined in
 * src/plsnds/plsnds_open_close.c) that identifies the launcher and
 * its associated function for obtaining the process name
 *
 * <JMS>
 * you mean hard-code this in plsnds_open_close.c?  Kinda defeats
 * the point of a component system.  Can we have a registration
 * function, instead?  Or can you just key off the component name
 * automatically?
 * </JMS>
 * 
 * - where necessary, provide a function in the orte_plsnds directory
 * that can define the process name from whatever info that
 * corresponding launcher provided
 * 
 * More information on the requirements for the PLSNDS can be found in
 * the header file src/plsnds/plsnds.h.
 * 
 * Unless otherwise directed by the user and/or the system
 * configuration, the PLS will utilize a daemon-based launch to
 * maximize the availability of ORTE services. To accomplish this, the
 * resource manager (RMGR) subsystem must support both the detection
 * of daemon existence and the ability to execute a two-step launch
 * sequence (with the first step being daemon launch, followed by the
 * secondary application launch). In turn, the PLS must provide a
 * component with the ability to launch via an existing daemon.
 * 
 * NOTE: The RMGR may override local launcher specification to utilize
 * the daemon-based launch component - it is expected that the daemons
 * in the local environment will know how to launch in that
 * environment. It is vital, therefore, that the PLS components NOT be
 * directly called by any ORTE function - instead, all PLS
 * functionality is to be accessed via the RMGR.
 * 
 * As part of the launch procedure, PLS components must provide the
 * following capabilities:
 * 
 * - set the OMPI_MCA_PLS_LAUNCHER environmental parameter indicating
 * which launcher was used. This information is subsequently used by
 * the name discovery service to determine a process' official name,
 * as described above.
 *
 * <JMS>
 * See above.
 * </JMS>
 * 
 * - setup I/O forwarding for all processes (where possible). Some
 * environments will, of course, not support this capability or will
 * provide it natively. Those respective PLS components should behave
 * accordingly. In other cases, however, the PLS component should
 * establish the I/O forwarding interconnects and enable that
 * subsystem.
 *
 * <JMS>
 * Setup I/O forwarding to where?  This process (i.e., the one
 * invoking module.launch())?  What about when daemons do the
 * launching?  Is it required to use the iof framework, or can we use
 * something else?  Is it required to separate the stdin/out/err for
 * each process in the job, or can it be amalgomated?
 * </JMS>
 * 
 * - pass context info to each process. The argv and enviro arrays are
 * stored on the registry by the resource allocation subsystem (RAS) -
 * this includes any process- specific deviations from the
 * application's general overall context. The PLS should obtain this
 * information from the registry and pass the context along to each
 * process.
 *
 * <JMS>
 * What code is invoked on the launched-process's side to do this
 * stuff?  Is there a "client side" to the PLS (analogous to the old
 * pcmclient)?  Without that, I'm not sure how to pass the context
 * info to each process in all cases.
 * </JMS>
 * 
 * - utilize scalable launch methods (where possible). In environments
 * that allow it, PLS components should utilize methods that support
 * scalable launch of applications involving large numbers of
 * processes.
 * 
 * - detect that required libraries are present on involved compute
 * nodes. This is a secondary feature for future implementations.
 * 
 * - preposition files and libraries where required and possible. This
 * is a secondary feature for future implementations.
 * 
 * When launching an application, the PLS shall update the registry
 * with information on batch jobid, assigned jobname, etc. that may
 * have been provided by the local resource's launcher. This
 * information is stored on the registry's ORTE_JOB_SEGMENT in the
 * "global" container. In addition, any information relevant to
 * state-of-health monitoring (e.g., sockets opened to an application
 * process by a spawning daemon to detect completion of process
 * startup) should be stored on the ORTE_JOB_SEGMENT in the respective
 * process' container.
 * 
 * Once a process is launched, two options exist for subsequent
 * operations:
 * 
 * - if it is an ORTE process (i.e., one that calls orte_init), the
 * process will register itself on the ORTE_JOB_SEGMENT of the
 * registry. This includes providing information on the nodename where
 * the process is located, contact information for the runtime message
 * library (RML) and other subsystems, local pid, etc.
 * 
 * - if it is NOT an ORTE process, then registration will not take
 * place. In this case, the ability to subsequently monitor the
 * progress/state-of-health of the process and/or provide other
 * services will be limited. The PLS has no further responsibilities
 * for such processes.
 *
 * <JMS>
 * Why not?  Some PLS's may still have information about non-ORTE
 * jobs (e.g., daemon), and therefore may still have positive control
 * over a) actively killing the proceses/job, or b) knowing when the
 * processes/job dies.  I think there's really 2 distinctions and 3
 * possibilities here:
 *
 * 1. ORTE job, which means we always have positive control over the job
 * after it launches.
 *
 * 2. Non-ORTE job, but in some cases we do have positive control
 * after launch.
 *
 * 3. Non-ORTE job, and we have no control over it after launch.
 * </JMS>
 * 
 * Once the PLS has completed launch of the application, it notifies
 * the state-of-health (SOH) monitor that a jobid has been launched
 * and is now available for monitoring.  It is the SOH's
 * responsibility to determine the level of monitoring that can be
 * provided, and to notify the rest of the ORTE system of process
 * failures/problems.
 */

#ifndef ORTE_MCA_PLS_H
#define ORTE_MCA_PLS_H

#include "orte_config.h"

#include "mca/mca.h"
#include "mca/ns/ns_types.h"
#include "class/ompi_list.h"

/*
 * PLS module functions
 */

/**
 * Launch the indicated jobid 
 */
typedef int (*orte_pls_base_module_launch_fn_t)(orte_jobid_t);

/**
 * Cleanup all resources held by the module
 */
typedef int (*orte_pls_base_module_finalize_fn_t)(void);

/**
 * PLS module version 1.0.0
 */
struct orte_pls_base_module_1_0_0_t {
   orte_pls_base_module_launch_fn_t launch;
   orte_pls_base_module_finalize_fn_t finalize;
};

/** shorten orte_pls_base_module_1_0_0_t declaration */
typedef struct orte_pls_base_module_1_0_0_t orte_pls_base_module_1_0_0_t;
/** shorten orte_pls_base_module_t declaration */
typedef struct orte_pls_base_module_1_0_0_t orte_pls_base_module_t;

/**
 * PLS initialization function
 *
 * Called by the MCA framework to initialize the component.  Invoked
 * exactly once per process.
 *
 * @param priority (OUT) Relative priority or ranking use by MCA to
 *                       select a module.
 */
typedef struct orte_pls_base_module_1_0_0_t* 
(*orte_pls_base_component_init_fn_t)(int *priority);

/** 
 * PLS component v1.0.0
 */
struct orte_pls_base_component_1_0_0_t {
    /** component version */
    mca_base_component_t pls_version;
    /** component data */
    mca_base_component_data_1_0_0_t pls_data;
    /** Function called when component is initialized */
    orte_pls_base_component_init_fn_t pls_init;
};
/** shorten orte_pls_base_component_1_0_0_t declaration */
typedef struct orte_pls_base_component_1_0_0_t orte_pls_base_component_1_0_0_t;
/** shorten orte_pls_base_component_t declaration */
typedef orte_pls_base_component_1_0_0_t orte_pls_base_component_t;


/**
 * Macro for use in modules that are of type pml v1.0.0
 */
#define ORTE_PLS_BASE_VERSION_1_0_0 \
  /* pls v1.0 is chained to MCA v1.0 */ \
  MCA_BASE_VERSION_1_0_0, \
  /* pls v1.0 */ \
  "pls", 1, 0, 0

#endif /* MCA_PLS_H */

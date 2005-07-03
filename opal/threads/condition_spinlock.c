/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University.
 *                         All rights reserved.
 * Copyright (c) 2004-2005 The Trustees of the University of Tennessee.
 *                         All rights reserved.
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

#include "ompi_config.h"

#include "threads/mutex.h"
#include "threads/condition.h"

#if (OMPI_HAVE_POSIX_THREADS == 0) || (OMPI_ENABLE_PROGRESS_THREADS == 0)

static void ompi_condition_construct(ompi_condition_t *c)
{
    c->c_waiting = 0;
    c->c_signaled = 0;
}


static void ompi_condition_destruct(ompi_condition_t *c)
{
}


OBJ_CLASS_INSTANCE(ompi_condition_t,
                   opal_object_t,
                   ompi_condition_construct,
                   ompi_condition_destruct);

#endif

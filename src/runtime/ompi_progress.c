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

#include "ompi_config.h"

#include <sched.h>

#include "event/event.h"
#include "mca/pml/pml.h"
#include "mca/io/io.h"
#include "runtime/ompi_progress.h"
#include "include/constants.h"

static int ompi_progress_event_flag = OMPI_EVLOOP_ONCE;
int ompi_progress_pending_io_reqs = false;
static ompi_progress_callback_t *callbacks = NULL;
static size_t callbacks_len = 0;
static size_t callbacks_size = 0;

void ompi_progress_events(int flag)
{
    ompi_progress_event_flag = flag;
}


void ompi_progress(void)
{
    /* progress any outstanding communications */
    int ret, events = 0;
    size_t i;

    if (ompi_using_threads() == false && ompi_progress_event_flag != 0) {
        ret = ompi_event_loop(ompi_progress_event_flag);
        if (ret > 0) {
            events += ret;
        }
    }

    for (i = 0 ; i < callbacks_len ; ++i) {
        if (NULL != callbacks[i]) {
            ret = (callbacks[i])();
            if (ret > 0) {
                events += ret;
            }
        }
    }

#if 0
    ret = mca_pml.pml_progress();
    if (ret > 0) {
        events += ret;
    }

    /* Progress IO requests, if there are any */
    if (ompi_progress_pending_io_reqs > 0) {
        ret = mca_io_base_progress();
        if (ret > 0) {
            events += ret;
            ompi_progress_pending_io_reqs -= ret;
        }
    }
#endif

#if 0
    /* TSW - disable this until can validate that it doesn't impact SMP
     * performance
    */
    /*
     * if there is nothing to do - yield the processor - otherwise
     * we could consume the processor for the entire time slice. If
     * the processor is oversubscribed - this will result in a best-case
     * latency equivalent to the time-slice.
    */
    if(events == 0) {
        sched_yield();
    }
#endif
}


/*
 * NOTE: This function is not in any way thread-safe.  Do not allow
 * multiple calls to ompi_progress_register and/or ompi_progress
 * concurrently.  This will be fixed in the near future.
 */
int
ompi_progress_register(ompi_progress_callback_t cb)
{
    /* see if we need to allocate more space */
    if (callbacks_len + 1 > callbacks_size) {
        ompi_progress_callback_t *tmp;
        tmp = realloc(callbacks, callbacks_size + 4);
        if (tmp == NULL) return OMPI_ERR_TEMP_OUT_OF_RESOURCE;

        callbacks = tmp;
        callbacks_size += 4;
    }

    callbacks[callbacks_len++] = cb;

    return OMPI_SUCCESS;
}

int
ompi_progress_unregister(ompi_progress_callback_t cb)
{
    size_t i;

    for (i = 0 ; i < callbacks_len ; ++i) {
        if (cb == callbacks[i]) {
            callbacks[i] == NULL;
            return OMPI_SUCCESS;
        }
    }

    return OMPI_ERR_NOT_FOUND;
}

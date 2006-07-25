/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
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
#include "ompi/constants.h"
#include "ompi/request/request.h"


int ompi_request_test_any(
    size_t count,
    ompi_request_t ** requests,
    int *index,
    int *completed,
    ompi_status_public_t * status)
{
    size_t i;
    size_t num_requests_null_inactive = 0;
    ompi_request_t **rptr;
    ompi_request_t *request;
    int rc;

    opal_atomic_mb();
    rptr = requests;
    for (i = 0; i < count; i++, rptr++) {
        request = *rptr;
        if( request->req_state == OMPI_REQUEST_INACTIVE ) {
            num_requests_null_inactive++;
            continue;
        }
        if( request->req_complete ) {
            *index = i;
            *completed = true;
            if (MPI_STATUS_IGNORE != status) {
                /* See MPI-1.2, sec 3.2.5, p.22 */
                int old_error = status->MPI_ERROR;
                *status = request->req_status;
                status->MPI_ERROR = old_error;
            }
            if( request->req_persistent ) {
                request->req_state = OMPI_REQUEST_INACTIVE;
                return OMPI_SUCCESS;
            }
            rc = request->req_status.MPI_ERROR;
            if (OMPI_SUCCESS != ompi_request_free(rptr)) {
                return OMPI_ERROR;
            }
            return rc;
        }
    }

    /* Only fall through here if we found nothing */
    *index = MPI_UNDEFINED;
    if(num_requests_null_inactive != count) {
        *completed = false;
#if OMPI_ENABLE_PROGRESS_THREADS == 0
        opal_progress();
#endif
    } else {
        *completed = true;
        if (MPI_STATUS_IGNORE != status) {
            *status = ompi_status_empty;
        }
    }
    return OMPI_SUCCESS;
}


int ompi_request_test_all(
    size_t count,
    ompi_request_t ** requests,
    int *completed,
    ompi_status_public_t * statuses)
{
    size_t i;
    ompi_request_t **rptr;
    size_t num_completed = 0;
    ompi_request_t *request;

    opal_atomic_mb();
    rptr = requests;
    for (i = 0; i < count; i++, rptr++) {
        request = *rptr;
        if( request->req_state == OMPI_REQUEST_INACTIVE ||
            request->req_complete) {
            num_completed++;
        }
    }

    if (num_completed != count) {
        *completed = false;
#if OMPI_ENABLE_PROGRESS_THREADS == 0
        opal_progress();
#endif
        return OMPI_SUCCESS;
    }

    rptr = requests;
    *completed = true;
    if (MPI_STATUSES_IGNORE != statuses) {
        /* fill out completion status and free request if required */
        for( i = 0; i < count; i++, rptr++ ) {
            int rc;
            request  = *rptr;
            if( request->req_state == OMPI_REQUEST_INACTIVE ) {
                statuses[i] = ompi_status_empty;
                continue;
            }
            statuses[i] = request->req_status;
            if( request->req_persistent ) {
                request->req_state = OMPI_REQUEST_INACTIVE;
                continue;
            }
            rc = ompi_request_free(rptr);
            if(rc != OMPI_SUCCESS)
                 return rc;
        }
    } else {
        /* free request if required */
        for( i = 0; i < count; i++, rptr++ ) {
            int rc;
            request = *rptr;
            if( request->req_state == OMPI_REQUEST_INACTIVE) {
                continue;
            }
            if( request->req_persistent ) {
                request->req_state = OMPI_REQUEST_INACTIVE;
                continue;
            }
            rc = ompi_request_free(rptr);
            if(rc != OMPI_SUCCESS)
                return rc;
        }
    }
    return OMPI_SUCCESS;
}


int ompi_request_test_some(
    size_t count,
    ompi_request_t ** requests,
    int * outcount,
    int * indices,
    ompi_status_public_t * statuses)
{
    size_t i, num_requests_null_inactive=0, num_requests_done = 0;
    int rc = OMPI_SUCCESS;
    ompi_request_t **rptr;
    ompi_request_t *request;

    opal_atomic_mb();
    rptr = requests;
    for (i = 0; i < count; i++, rptr++) {
        request = *rptr;
        if (request->req_state == OMPI_REQUEST_INACTIVE) {
            num_requests_null_inactive++;
            continue;
        }
        if (true == request->req_complete) {
            indices[num_requests_done++] = i;
        }
    }

    /*
     * If there are no active requests, no need to progress
     */
    if (num_requests_null_inactive == count) {
        *outcount = MPI_UNDEFINED;
        return OMPI_SUCCESS;
    }

    *outcount = num_requests_done;

    if (num_requests_done == 0) {
#if OMPI_ENABLE_PROGRESS_THREADS == 0
        opal_progress();
#endif
        return OMPI_SUCCESS;
    }

    /* fill out completion status and free request if required */
    for( i = 0; i < num_requests_done; i++) {
        request = requests[indices[i]];

        if (MPI_STATUSES_IGNORE != statuses) {
            statuses[i] = request->req_status;
        }

        rc += request->req_status.MPI_ERROR;

        if( request->req_persistent ) {
            request->req_state = OMPI_REQUEST_INACTIVE;
        } else {
            int tmp;
            /* return request to pool */
            tmp = ompi_request_free(&(requests[indices[i]]));
            /*
              * If it fails, we are screwed. We cannot put the
              * request_free return code into the status, possibly
              * overwriting some other important error; therefore just quit.
              */
            if (OMPI_SUCCESS != tmp) {
                return tmp;
            }
        }
    }

    if (OMPI_SUCCESS != rc) {
        rc = MPI_ERR_IN_STATUS;
    }

    return rc;
}

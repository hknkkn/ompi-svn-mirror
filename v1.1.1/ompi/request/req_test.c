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
                *status = request->req_status;
            }
            if( request->req_persistent ) {
                request->req_state = OMPI_REQUEST_INACTIVE;
                return OMPI_SUCCESS;
            }
            return ompi_request_free(rptr);
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


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

/**
 * @file
 *
 * Generic helper routines for environment manipulation.
 */

#ifndef OMPI_ENVIRON_H
#define OMPI_ENVIRON_H

#include "ompi_config.h"

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif
    /**
     * Merge two environ-like arrays into a single, new array, ensuring
     * that there are no duplicate entries.
     *
     * @param minor Set 1 of the environ's to merge
     * @param major Set 2 of the environ's to merge
     * @retval New array of environ
     *
     * Merge two environ-like arrays into a single, new array,
     * ensuring that there are no duplicate entires.  If there are
     * duplicates, entries in the \em major array are favored over
     * those in the \em minor array.
     *
     * Both \em major and \em minor are expected to be argv-style
     * arrays (i.e., terminated with a NULL pointer).
     *
     * The array that is returned is an unencumbered array that should
     * later be freed with a call to ompi_argv_free().
     *
     * Either (or both) of \em major and \em minor can be NULL.  If
     * one of the two is NULL, the other list is simply copied to the
     * output.  If both are NULL, NULL is returned.
     */
    OMPI_DECLSPEC char **ompi_environ_merge(char **minor, char **major);

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif

#endif /* OMPI_ARGV_H */

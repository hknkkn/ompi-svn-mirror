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

#include <stdlib.h>
#include <string.h>

#include "include/constants.h"
#include "util/argv.h"
#include "util/environ.h"


/*
 * Merge two environ-like char arrays, ensuring that there are no
 * duplicate entires
 */
char **ompi_environ_merge(char **minor, char **major)
{
    int argc = 0;
    char **argv = NULL;
    char *a, *b;
    int i, j;

    /* Check for bozo cases */

    if (NULL == major) {
        if (NULL == minor) {
            return NULL;
        } else {
            return ompi_argv_copy(minor);
        }
    }

    /* First, copy major */

    argv = ompi_argv_copy(major);
    argc = ompi_argv_count(argv);

    /* Do we have something in minor? */

    if (NULL == minor) {
        return argv;
    }

    /* Now go through minor and see if there's any non-duplicated env
       variables in there that we can append to the new argv.  This is
       not terribly efficient O(N*M), but this will never be in a
       performance-critical path. */

    for (i = 0; NULL != minor[i]; ++i) {
        a = strchr(minor[i], '=');
        if (NULL != a) {
            *a = '\0';
        }

        for (j = 0; NULL != major[j]; ++j) {
            b = strchr(major[j], '=');
            if (NULL != b) {
                *b = '\0';
            }

            if (0 == strcmp(a, b)) {
                if (NULL != b) {
                    *b = '=';
                }
                break;
            }
            if (NULL != b) {
                *b = '=';
            }
        }
        if (NULL != a) {
            *a = '=';
        }

        /* Did we find a match in major?  If not, add this entry from
           minor into the output argv */

        if (NULL == major[j]) {
            ompi_argv_append(&argc, &argv, minor[i]);
        }
    }

    /* All done */

    return argv;
}

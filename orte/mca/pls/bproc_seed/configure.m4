# -*- shell-script -*-
#
# Copyright (c) 2004-2005 The Trustees of Indiana University.
#                         All rights reserved.
# Copyright (c) 2004-2005 The Trustees of the University of Tennessee.
#                         All rights reserved.
# Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
#                         University of Stuttgart.  All rights reserved.
# Copyright (c) 2004-2005 The Regents of the University of California.
#                         All rights reserved.
# $COPYRIGHT$
# 
# Additional copyrights may follow
# 
# $HEADER$
#

# MCA_pls_bproc_seed_CONFIG([action-if-found], [action-if-not-found])
# -----------------------------------------------------------
AC_DEFUN([MCA_pls_bproc_seed_CONFIG],[
    # only accept newer non-Scyld bproc
    OMPI_CHECK_BPROC([pls_bproc_seed], [pls_bproc_seed_good=1], 
                     [pls_bproc_seed_good=0], [pls_bproc_seed_good=0])

    # if check worked, set wrapper flags if so.  
    # Evaluate succeed / fail
    AS_IF([test "$pls_bproc_seed_good" = "1"],
          [pls_bproc_seed_WRAPPER_EXTRA_LDFLAGS="$pls_bproc_seed_LDFLAGS"
           pls_bproc_seed_WRAPPER_EXTRA_LIBS="$pls_bproc_seed_LIBS"
           $1],
          [$2])
    AS_IF([test "$pls_bproc_good" = "0" && test ! -z "$with_bproc" -a "$with_bproc" != "no"],
          [AC_MSG_ERROR([Scyld bproc is not supported by the launching system yet])])

    # set build flags to use in makefile
    AC_SUBST([pls_bproc_seed_OBJCFLAGS])
    AC_SUBST([pls_bproc_seed_LDFLAGS])
    AC_SUBST([pls_bproc_seed_LIBS])
])dnl

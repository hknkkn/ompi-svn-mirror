# -*- shell-script -*-
#
# Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
#                         University Research and Technology
#                         Corporation.  All rights reserved.
# Copyright (c) 2004-2005 The University of Tennessee and The University
#                         of Tennessee Research Foundation.  All rights
#                         reserved.
# Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
#                         University of Stuttgart.  All rights reserved.
# Copyright (c) 2004-2005 The Regents of the University of California.
#                         All rights reserved.
# Copyright (c) 2009      Cisco Systems, Inc.  All rights reserved.
# $COPYRIGHT$
# 
# Additional copyrights may follow
# 
# $HEADER$
#

# ORTE_CHECK_ALPS(prefix, [action-if-found], [action-if-not-found])
# --------------------------------------------------------
AC_DEFUN([ORTE_CHECK_ALPS],[
    if test -z "$orte_check_alps_happy"; then
        # require that we check for pmi support request first so
        # we can get the static library ordering correct
        AC_REQUIRE([ORTE_CHECK_PMI])

        AC_ARG_WITH([alps],
                    [AC_HELP_STRING([--with-alps(=DIR|yes|no)],
                    [Build with ALPS scheduler component, optionally adding DIR/include, DIR/lib, and DIR/lib64 to the search path for headers and libraries (default: no)])])
        OMPI_CHECK_WITHDIR([alps], [$with_alps], [.])

        AC_ARG_WITH([alps-libdir],
                    [AC_HELP_STRING([--with-alps-libdir=DIR],
                    [Location of alps libraries (alpslli, alpsutil) (default: /usr/lib/alps)])])

        # save the CPPFLAGS so we can check for alps/apInfo.h without adding $with_alps/include to the global path
        orte_check_alps_$1_save_CPPFLAGS="$CPPFLAGS"

        #
        # check to see where alps is installed, it wandered to a new location starting with CLE 5.0
        #
 
        if test -f "/usr/lib/alps/libalps.a" ; then
            using_cle5_install="no"
        else
            using_cle5_install="yes"
        fi

        if test "$with_alps" = "no" -o -z "$with_alps" ; then
            orte_check_alps_happy="no"
        else
           # Only need to do these tests once (this macro is invoked
           # from multiple different components' configure.m4 scripts

           orte_check_alps_happy="yes"
           orte_check_alps_libdir="$with_alps_libdir"
           
           if test -z "$orte_check_alps_libdir" ; then
               if test "$with_alps" != "yes" ; then
                   AS_IF([test -d "$with_alps/lib64"],
                       [orte_check_alps_libdir="$with_alps/lib64"],
                       [orte_check_alps_libdir="$with_alps/lib"])
               else
                   if test "$using_cle5_install" = "yes"; then
                       orte_check_alps_libdir="/opt/cray/alps/default/lib64"
                   else
                       orte_check_alps_libdir="/usr/lib/alps"
                   fi
               fi
           fi

           if test "$using_cle5_install" = "yes" ; then
                  AS_IF([test "$with_alps" = "yes"],
                        [orte_check_alps_dir="/usr"],
                        [orte_check_alps_dir="$with_alps"])
           else
                  AS_IF([test "$with_alps" = "yes"],
                        [orte_check_alps_dir="/opt/cray/alps/default"],
                        [orte_check_alps_dir="$with_alps"])
           fi

           $1_CPPFLAGS="-I$orte_check_alps_dir/include"
           $1_LDFLAGS="-L$orte_check_alps_libdir"

           if test -z "$orte_check_alps_pmi_happy"; then
               # if pmi support is requested, then ORTE_CHECK_PMI
               # will have added the -lpmi flag to LIBS. We then need
               # to add a couple of alps libs to support static
               # builds
               orte_check_alps_pmi_happy=no

               if test "$orte_enable_pmi" = 1 ; then
                   AC_MSG_CHECKING([for alps libraries in "$orte_check_alps_libdir"])

                   # libalpslli and libalpsutil are needed by libpmi to compile statically
                   AS_IF([test -f "$orte_check_alps_libdir/libalpslli.a" -a -f "$orte_check_alps_libdir/libalpsutil.a"],
                         [AC_MSG_RESULT([found])
                          orte_check_alps_pmi_happy=yes],
                         [AC_MSG_RESULT([not found])])

                   AS_IF([test "$orte_check_alps_pmi_happy" = "yes" -a "$orte_without_full_support" = 0],
                         [WRAPPER_EXTRA_LDFLAGS="$WRAPPER_EXTRA_LDFLAGS -L$orte_check_alps_libdir"
                          WRAPPER_EXTRA_LIBS="$WRAPPER_EXTRA_LIBS -lalpslli -lalpsutil"],
                         [AC_MSG_WARN([PMI support for Alps requested but not found])
                          AC_MSG_ERROR([Cannot continue])])
               fi
           fi
        fi
    fi

    AS_IF([test "$orte_check_alps_happy" = "yes"], 
          [$2], 
          [$3])
])

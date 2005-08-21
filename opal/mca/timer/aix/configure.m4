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

AC_DEFUN([MCA_timer_aix_COMPILE_MODE], [
    AC_MSG_CHECKING([for MCA component $2:$3 compile mode])
    $4="static"
    AC_MSG_RESULT([$$4])
])


# MCA_timer_aix_CONFIG(action-if-can-compile, 
#                        [action-if-cant-compile])
# ------------------------------------------------
AC_DEFUN([MCA_timer_aix_CONFIG],[
    AC_ARG_WITH([timer],
        [AC_HELP_STRING([--with-timer=TYPE],
                        [high resolution timer component])])

    AS_IF([test "$with_timer" = "aix"],
          [timer_aix_happy="yes"
           timer_aix_should_use=1],
          [timer_aix_should_use=0
           AS_IF([test "$with_timer" = ""],
                 [timer_aix_happy="yes"],
                 [timer_aix_happy="no"])])

    AS_IF([test "$timer_aix_happy" = "yes"],
          [AC_CHECK_FUNC([time_base_to_time], 
                         [timer_aix_happy="yes"],
                         [timer_aix_happy="no"])])

    AS_IF([test "$timer_aix_happy" = "yes"],
          [AC_CHECK_FUNC([pm_cycles], 
                         [timer_aix_happy="yes"],
                         [timer_aix_happy="no"])])

   AS_IF([test "$timer_aix_happy" = "no" -a \
               "$timer_aix_should_use" = "1"],
         [AC_MSG_ERROR([AIX timer requested but not available.  Aborting.])])

    AS_IF([test "$timer_aix_happy" = "yes"], 
          [timer_base_include="aix/timer_aix.h"
           $1], 
          [$2])
])

dnl -*- shell-script -*-
dnl
dnl Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
dnl                         University Research and Technology
dnl                         Corporation.  All rights reserved.
dnl Copyright (c) 2004-2005 The University of Tennessee and The University
dnl                         of Tennessee Research Foundation.  All rights
dnl                         reserved.
dnl Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
dnl                         University of Stuttgart.  All rights reserved.
dnl Copyright (c) 2004-2005 The Regents of the University of California.
dnl                         All rights reserved.
dnl Copyright (c) 2006-2010 Oracle and/or its affiliates.  All rights reserved.
dnl Copyright (c) 2009-2012 Cisco Systems, Inc.  All rights reserved.
dnl $COPYRIGHT$
dnl 
dnl Additional copyrights may follow
dnl 
dnl $HEADER$
dnl


# OMPI_SETUP_WRAPPER_INIT()
# -------------------------
# Setup wrapper compiler configuration information - should be called
# before the bulk of the tests that can affect the wrapper compilers
#
# Note that we keep the user-specified flags seperately because we
# don't want them to go through OPAL_UNIQ because that has resulted in
# unexpected behavior for the user in the past.
AC_DEFUN([OMPI_SETUP_WRAPPER_INIT],[
    WRAPPER_EXTRA_FCFLAGS=
    WRAPPER_EXTRA_FCFLAGS_PREFIX=
    USER_WRAPPER_EXTRA_FCFLAGS=
    USER_WRAPPER_EXTRA_FCLAGS_PREFIX=

    AC_ARG_WITH([wrapper-fcflags], 
        [AC_HELP_STRING([--with-wrapper-fcflags],
             [Extra flags to add to FCFLAGS when using mpifort])])
    if test "$with_wrapper_fcflags" = "yes" -o "$with_wrapper_fcflags" = "no"; then
        AC_MSG_ERROR([--with-wrapper-fcflags must have an argument.  Aborting])
    elif test ! -z "$with_wrapper_fcflags" ; then
        USER_WRAPPER_EXTRA_FCFLAGS="$with_wrapper_fcflags"
    fi

    AC_ARG_WITH([wrapper-fcflags-prefix], 
        [AC_HELP_STRING([--with-wrapper-fcflags-prefix],
             [Extra flags (before user flags) to add to FCFLAGS when using mpifort])])
    if test "$with_wrapper_fcflags_prefix" = "yes" -o "$with_wrapper_fcflags_prefix" = "no"; then
        AC_MSG_ERROR([--with-wrapper-fcflags-prefix must have an argument.  Aborting])
    elif test ! -z "$with_wrapper_fcflags_prefix" ; then
        USER_WRAPPER_EXTRA_FCFLAGS_PREFIX="$with_wrapper_fcflags_prefix"
    fi

])

AC_DEFUN([_OMPI_SETUP_ORTE_WRAPPERS],[
    AC_MSG_CHECKING([for OMPI CPPFLAGS])
    OMPI_WRAPPER_EXTRA_CPPFLAGS="$ORTE_WRAPPER_EXTRA_CPPFLAGS"
    AC_SUBST([OMPI_WRAPPER_EXTRA_CPPFLAGS])
    AC_MSG_RESULT([$OMPI_WRAPPER_EXTRA_CPPFLAGS])

    AC_MSG_CHECKING([for OMPI CFLAGS])
    OMPI_WRAPPER_EXTRA_CFLAGS="$ORTE_WRAPPER_EXTRA_CFLAGS"
    AC_SUBST([OMPI_WRAPPER_EXTRA_CFLAGS])
    AC_MSG_RESULT([$OMPI_WRAPPER_EXTRA_CFLAGS])

    AC_MSG_CHECKING([for OMPI CFLAGS_PREFIX])
    OMPI_WRAPPER_EXTRA_CFLAGS_PREFIX="$ORTE_WRAPPER_EXTRA_CFLAGS_PREFIX"
    AC_SUBST([OMPI_WRAPPER_EXTRA_CFLAGS_PREFIX])
    AC_MSG_RESULT([$OMPI_WRAPPER_EXTRA_CFLAGS_PREFIX])

    AC_MSG_CHECKING([for OMPI CXXFLAGS])
    OMPI_WRAPPER_EXTRA_CXXFLAGS="$ORTE_WRAPPER_EXTRA_CXXFLAGS"
    AC_SUBST([OMPI_WRAPPER_EXTRA_CXXFLAGS])
    AC_MSG_RESULT([$OMPI_WRAPPER_EXTRA_CXXFLAGS])

    AC_MSG_CHECKING([for OMPI CXXFLAGS_PREFIX])
    OMPI_WRAPPER_EXTRA_CXXFLAGS_PREFIX="$ORTE_WRAPPER_EXTRA_CXXFLAGS_PREFIX"
    AC_SUBST([OMPI_WRAPPER_EXTRA_CXXFLAGS_PREFIX])
    AC_MSG_RESULT([$OMPI_WRAPPER_EXTRA_CXXFLAGS_PREFIX])

    AC_MSG_CHECKING([for OMPI FCFLAGS])
    OMPI_WRAPPER_EXTRA_FCFLAGS="$WRAPPER_EXTRA_FCFLAGS $USER_WRAPPER_EXTRA_FCFLAGS"
    AC_SUBST([OMPI_WRAPPER_EXTRA_FCFLAGS])
    AC_MSG_RESULT([$OMPI_WRAPPER_EXTRA_FCFLAGS])

    AC_MSG_CHECKING([for OMPI FCFLAGS_PREFIX])
    OMPI_WRAPPER_EXTRA_FCFLAGS_PREFIX="$WRAPPER_EXTRA_FCFLAGS_PREFIX $USER_WRAPPER_EXTRA_FCFLAGS_PREFIX"
    AC_SUBST([OMPI_WRAPPER_EXTRA_FCFLAGS_PREFIX])
    AC_MSG_RESULT([$OMPI_WRAPPER_EXTRA_FCFLAGS_PREFIX])

    AC_MSG_CHECKING([for OMPI LDFLAGS])
    OMPI_WRAPPER_EXTRA_LDFLAGS="$ompi_WRAPPER_EXTRA_LDFLAGS $ORTE_WRAPPER_EXTRA_LDFLAGS"
    AC_SUBST([OMPI_WRAPPER_EXTRA_LDFLAGS])
    AC_MSG_RESULT([$OMPI_WRAPPER_EXTRA_LDFLAGS])

    AC_MSG_CHECKING([for OMPI LIBS])
    OMPI_WRAPPER_EXTRA_LIBS="$ompi_WRAPPER_EXTRA_LIBS $ORTE_WRAPPER_EXTRA_LIBS"
    AC_SUBST([OMPI_WRAPPER_EXTRA_LIBS])
    AC_MSG_RESULT([$OMPI_WRAPPER_EXTRA_LIBS])

    AC_MSG_CHECKING([for OMPI extra include dirs])
    OMPI_WRAPPER_EXTRA_INCLUDES="$ORTE_WRAPPER_EXTRA_INCLUDES"
    AC_SUBST([OMPI_WRAPPER_EXTRA_INCLUDES])
    AC_MSG_RESULT([$OMPI_WRAPPER_EXTRA_INCLUDES])
])

AC_DEFUN([_OMPI_SETUP_OPAL_WRAPPERS],[
    AC_MSG_CHECKING([for OMPI CPPFLAGS])
    OMPI_WRAPPER_EXTRA_CPPFLAGS="$OPAL_WRAPPER_EXTRA_CPPFLAGS"
    AC_SUBST([OMPI_WRAPPER_EXTRA_CPPFLAGS])
    AC_MSG_RESULT([$OMPI_WRAPPER_EXTRA_CPPFLAGS])

    AC_MSG_CHECKING([for OMPI CFLAGS])
    OMPI_WRAPPER_EXTRA_CFLAGS="$OPAL_WRAPPER_EXTRA_CFLAGS"
    AC_SUBST([OMPI_WRAPPER_EXTRA_CFLAGS])
    AC_MSG_RESULT([$OMPI_WRAPPER_EXTRA_CFLAGS])

    AC_MSG_CHECKING([for OMPI CFLAGS_PREFIX])
    OMPI_WRAPPER_EXTRA_CFLAGS_PREFIX="$OPAL_WRAPPER_EXTRA_CFLAGS_PREFIX"
    AC_SUBST([OMPI_WRAPPER_EXTRA_CFLAGS_PREFIX])
    AC_MSG_RESULT([$OMPI_WRAPPER_EXTRA_CFLAGS_PREFIX])

    AC_MSG_CHECKING([for OMPI CXXFLAGS])
    OMPI_WRAPPER_EXTRA_CXXFLAGS="$OPAL_WRAPPER_EXTRA_CXXFLAGS"
    AC_SUBST([OMPI_WRAPPER_EXTRA_CXXFLAGS])
    AC_MSG_RESULT([$OMPI_WRAPPER_EXTRA_CXXFLAGS])

    AC_MSG_CHECKING([for OMPI CXXFLAGS_PREFIX])
    OMPI_WRAPPER_EXTRA_CXXFLAGS_PREFIX="$OPAL_WRAPPER_EXTRA_CXXFLAGS_PREFIX"
    AC_SUBST([OMPI_WRAPPER_EXTRA_CXXFLAGS_PREFIX])
    AC_MSG_RESULT([$OMPI_WRAPPER_EXTRA_CXXFLAGS])

    AC_MSG_CHECKING([for OMPI FCFLAGS])
    OMPI_WRAPPER_EXTRA_FCFLAGS="$WRAPPER_EXTRA_FCFLAGS $USER_WRAPPER_EXTRA_FCFLAGS"
    AC_SUBST([OMPI_WRAPPER_EXTRA_FCFLAGS])
    AC_MSG_RESULT([$OMPI_WRAPPER_EXTRA_FCFLAGS])

    AC_MSG_CHECKING([for OMPI FCFLAGS_PREFIX])
    OMPI_WRAPPER_EXTRA_FCFLAGS_PREFIX="$WRAPPER_EXTRA_FCFLAGS_PREFIX $USER_WRAPPER_EXTRA_FCFLAGS_PREFIX"
    AC_SUBST([OMPI_WRAPPER_EXTRA_FCFLAGS_PREFIX])
    AC_MSG_RESULT([$OMPI_WRAPPER_EXTRA_FCFLAGS_PREFIX])

    AC_MSG_CHECKING([for OMPI LDFLAGS])
    OMPI_WRAPPER_EXTRA_LDFLAGS="$ompi_WRAPPER_EXTRA_LDFLAGS $OPAL_WRAPPER_EXTRA_LDFLAGS"
    AC_SUBST([OMPI_WRAPPER_EXTRA_LDFLAGS])
    AC_MSG_RESULT([$OMPI_WRAPPER_EXTRA_LDFLAGS])

    AC_MSG_CHECKING([for OMPI LIBS])
    OMPI_WRAPPER_EXTRA_LIBS="$ompi_WRAPPER_EXTRA_LIBS $OPAL_WRAPPER_EXTRA_LIBS"
    AC_SUBST([OMPI_WRAPPER_EXTRA_LIBS])
    AC_MSG_RESULT([$OMPI_WRAPPER_EXTRA_LIBS])

    AC_MSG_CHECKING([for OMPI extra include dirs])
    OMPI_WRAPPER_EXTRA_INCLUDES="$OPAL_WRAPPER_EXTRA_INCLUDES"
    AC_SUBST([OMPI_WRAPPER_EXTRA_INCLUDES])
    AC_MSG_RESULT([$OMPI_WRAPPER_EXTRA_INCLUDES])
])

AC_DEFUN([OMPI_SETUP_WRAPPER_FINAL],[
    OPAL_UNIQ([WRAPPER_EXTRA_FCFLAGS])
    OPAL_UNIQ([WRAPPER_EXTRA_FCFLAGS_PREFIX])

    OPAL_UNIQ([ompi_WRAPPER_EXTRA_LDFLAGS])
    OPAL_UNIQ([ompi_WRAPPER_EXTRA_LIBS])

    m4_ifdef([project_orte], [_OMPI_SETUP_ORTE_WRAPPERS], [_OMPI_SETUP_OPAL_WRAPPERS])

    # language binding support.  C++ is a bit different, as the
    # compiler should work even if there is no MPI C++ bindings
    # support.  However, we do want it to fail if there is no C++
    # compiler.
    if test "$WANT_MPI_CXX_SUPPORT" = "1" ; then
        OMPI_WRAPPER_CXX_LIB="-lmpi_cxx"
        OMPI_WRAPPER_CXX_REQUIRED_FILE=""
    elif test "$CXX" = "none"; then
        OMPI_WRAPPER_CXX_LIB=""
        OMPI_WRAPPER_CXX_REQUIRED_FILE="not supported"
    else
        OMPI_WRAPPER_CXX_LIB=""
        OMPI_WRAPPER_CXX_REQUIRED_FILE=""
    fi
    AC_SUBST([OMPI_WRAPPER_CXX_LIB])
    AC_SUBST([OMPI_WRAPPER_CXX_REQUIRED_FILE])

    if test "$OMPI_WANT_FORTRAN_BINDINGS" = "1" ; then
        OMPI_WRAPPER_FORTRAN_REQUIRED_FILE=""
    else
        OMPI_WRAPPER_FORTRAN_REQUIRED_FILE="not supported"
    fi
    AC_SUBST([OMPI_WRAPPER_FORTRAN_REQUIRED_FILE])

    # For script-based wrappers that don't do relocatable binaries.
    # Don't use if you don't have to.
    exec_prefix_save="${exec_prefix}"
    test "x$exec_prefix" = xNONE && exec_prefix="${prefix}"
    eval "OMPI_WRAPPER_INCLUDEDIR=\"${includedir}\""
    eval "OMPI_WRAPPER_LIBDIR=\"${libdir}\""
    exec_prefix="${exec_prefix_save}"
    AC_SUBST([OMPI_WRAPPER_INCLUDEDIR])
    AC_SUBST([OMPI_WRAPPER_LIBDIR])

    # compatibility defines that will eventually go away
    WRAPPER_EXTRA_FCFLAGS="$OMPI_WRAPPER_EXTRA_FCFLAGS"
    WRAPPER_EXTRA_FCFLAGS_PREFIX="$OMPI_WRAPPER_EXTRA_FCFLAGS_PREFIX"
    AC_SUBST([WRAPPER_EXTRA_FCFLAGS])
    AC_SUBST([WRAPPER_EXTRA_FCFLAGS_PREFIX])

    AC_DEFINE_UNQUOTED(WRAPPER_EXTRA_FCFLAGS, "$WRAPPER_EXTRA_FCFLAGS",
        [Additional FCFLAGS to pass through the wrapper compilers])
    AC_DEFINE_UNQUOTED(WRAPPER_EXTRA_FCFLAGS_PREFIX, "$WRAPPER_EXTRA_FCFLAGS_PREFIX",
        [Additional FCFLAGS to pass through the wrapper compilers])


    # if wrapper compilers were requested, set the ompi one up
    if test "$WANT_SCRIPT_WRAPPER_COMPILERS" = "1" ; then
        AC_CONFIG_FILES([ompi/tools/wrappers/ompi_wrapper_script],
                        [chmod +x ompi/tools/wrappers/ompi_wrapper_script])
    fi

])

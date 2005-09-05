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


# OMPI_CHECK_OPENIB(prefix, [action-if-found], [action-if-not-found])
# --------------------------------------------------------
# check if OPENIB support can be found.  sets prefix_{CPPFLAGS, 
# LDFLAGS, LIBS} as needed and runs action-if-found if there is
# support, otherwise executes action-if-not-found
AC_DEFUN([OMPI_CHECK_OPENIB],[
    AC_ARG_WITH([openib],
                [AC_HELP_STRING([--with-openib=OPENIB_DIR],
                                [Additional directory to search for OPENIB installation])])
    AC_ARG_WITH([openib-libdir],
       [AC_HELP_STRING([--with-openib-libdir=IBLIBDIR],
                       [directory where the IB library can be found, if it is not in OPENIB_DIR/lib or OPENIB_DIR/lib64])])

    AS_IF([test ! -z "$with_btl_openib" -a "$with_btl_openib" != "yes"],
          [ompi_check_openib_dir="$with_btl_openib"])
    AS_IF([test ! -z "$with_btl_openib_libdir" -a "$with_btl_openib_libdir" != "yes"],
          [ompi_check_openib_libdir="$with_btl_openib_libdir"])

    # check for pthreads and emit a warning that things might go south...
    AS_IF([test "$HAVE_POSIX_THREADS" != "1"],
          [AC_MSG_WARN([POSIX threads not enabled.  May not be able to link with openib])])

    ompi_check_openib_$1_save_LDFLAGS="$LDFLAGS"
    ompi_check_openib_$1_save_LIBS="$LIBS"
    
    AC_CHECK_LIB(sysfs, 
	         sysfs_open_class, 
		 [ompi_check_openib_sysfs=yes
                  LIBS="$LIBS -lsysfs"
                  $1_LIBS="-lsysfs"], 
		 [ompi_check_openib_sysfs=no])

    AS_IF([test "$ompi_check_openib_sysfs" = "yes"],
          [$2],
          [AS_IF([test ! -z "$with_btl_openib" -a "$with_btl_openib" != "no"],
                 [AC_MSG_ERROR([OPENIB support requested but required sysfs not found.  Aborting])])
           $3])
    
    AS_IF([test "$ompi_check_openib_libdir" = ""], 
	  [ompi_check_openib_my_libdir=$ompi_check_openib_dir], 
	  [ompi_check_openib_my_libdir=$ompi_check_openib_libdir]) 

    AS_IF([test -d "$ompi_check_openib_my_libdir/lib64/infiniband"], 
	[ompi_check_openib_libflag=" -L $ompi_check_openib_my_libdir/lib64/infiniband" 
	      LDFLAGS="$LDFLAGS -L $ompi_check_openib_my_libdir/lib64/infiniband"], 
	[AS_IF([test -d "$ompi_check_openib_my_libdir/lib/infiniband"], 
		[ompi_check_openib_libflag=" -L$ompi_check_openib_my_libdir/lib/infiniband"
		    LDFLAGS="$LDFLAGS -L $ompi_check_openib_my_libdir/lib/infiniband"])])

    OMPI_CHECK_PACKAGE([$1],
                       [infiniband/verbs.h],
                       [ibverbs],
                       [ibv_open_device],
                       [-libcm],
                       [$ompi_check_openib_dir],
                       [$ompi_check_openib_libdir],
                       [ompi_check_openib_happy="yes"],
                       [ompi_check_openib_happy="no"])

    LDFLAGS="$ompi_check_openib_$1_save_LDFLAGS"
    LIBS="$ompi_check_openib_$1_save_LIBS"

    AS_IF([test "$ompi_check_openib_happy" = "yes"],
          [$2],
          [AS_IF([test ! -z "$with_btl_openib" -a "$with_btl_openib" != "no"],
                 [AC_MSG_ERROR([OPENIB support requested but not found.  Aborting])])
           $3])
    $1_LDFLAGS="$$1_LDFLAGS $ompi_check_openib_libflag"
])


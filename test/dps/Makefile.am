#
# Copyright (c) 2004-2005 The Trustees of Indiana University.
#                         All rights reserved.
# Copyright (c) 2004-2005 The Trustees of the University of Tennessee.
#                         All rights reserved.
# Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
#                         University of Stuttgart.  All rights reserved.
# $COPYRIGHT$
# 
# Additional copyrights may follow
# 
# $HEADER$
#

include $(top_srcdir)/config/Makefile.options
AM_CPPFLAGS = -I$(top_srcdir)/test/support -DOMPI_ENABLE_DEBUG_OVERRIDE=1 -g

noinst_PROGRAMS = \
	dps_test 

dps_test_SOURCES = dps_test.c
dps_test_LDADD = \
        $(top_builddir)/src/dps/libdps.la \
        $(top_builddir)/src/mca/base/libmca_base.la \
        $(top_builddir)/src/class/liblfc.la \
        $(top_builddir)/src/util/libutil.la \
        $(top_builddir)/src/threads/libthreads.la \
        $(top_builddir)/test/support/libsupport.la 
dps_test_DEPENDENCIES = $(dps_test_LDADD)

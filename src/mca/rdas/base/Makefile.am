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

noinst_LTLIBRARIES = libmca_rdas_base.la

# For VPATH builds, have to specify where static-modules.h will be found

AM_CPPFLAGS = -I$(top_builddir)/src
AM_LFLAGS = -Pmca_rdas_base_yy
LEX_OUTPUT_ROOT = lex.mca_rdas_base_yy

# Source code files

headers = \
	base.h \
	rdas_base_parse_hostfile_lex.h

libmca_rdas_base_la_SOURCES = \
    $(headers) \
	rdas_base_close.c \
	rdas_base_open.c \
	rdas_base_parse_hostfile.c \
	rdas_base_parse_hostfile_lex.l \
	rdas_base_select.c

# Conditionally install the header files

if WANT_INSTALL_HEADERS
ompidir = $(includedir)/openmpi/mca/rdas/base
ompi_HEADERS = $(headers)
else
ompidir = $(includedir)
endif


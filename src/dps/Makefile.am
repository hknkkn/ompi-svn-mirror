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

noinst_LTLIBRARIES = libdps.la

# Source code files

headers = \
    dps.h \
    dps_types.h \
    dps_internal.h

libdps_la_SOURCES = \
    $(headers) \
    orte_dps_open_close.c \
    dps_init_free.c \
    dps_pack_unpack_object.c \
    dps_pack_value.c \
    dps_unpack_value.c \
    dps_internal_functions.c

# Conditionally install the header files

if WANT_INSTALL_HEADERS
ompidir = $(includedir)/openmpi/dps
ompi_HEADERS = $(headers)
else
ompidir = $(includedir)
endif

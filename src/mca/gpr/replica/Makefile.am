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

# Use the top-level Makefile.options

include $(top_ompi_srcdir)/config/Makefile.options

SUBDIRS = api_layer functional_layer transition_layer communications

# Make the output library in this directory, and name it either
# mca_<type>_<name>.la (for DSO builds) or libmca_<type>_<name>.la
# (for static builds).

if OMPI_BUILD_gpr_replica_DSO
component_noinst = 
component_install = mca_gpr_replica.la
else
component_noinst = libmca_gpr_replica.la
component_install =
endif

mcacomponentdir = $(libdir)/openmpi
mcacomponent_LTLIBRARIES = $(component_install)
mca_gpr_replica_la_SOURCES =
mca_gpr_replica_la_LIBADD = \
	api_layer/libmca_gpr_replica.la \
	transition_layer/libmca_gpr_replica.la \
	functional_layer/libmca_gpr_replica.la \
	communications/libmca_gpr_replica.la
mca_gpr_replica_la_LDFLAGS = -module -avoid-version

noinst_LTLIBRARIES = $(component_noinst)
libmca_gpr_replica_la_SOURCES =
libmca_gpr_replica_la_LIBADD = \
	api_layer/libmca_gpr_replica.la \
	transition_layer/libmca_gpr_replica.la \
	functional_layer/libmca_gpr_replica.la \
	communications/libmca_gpr_replica.la
libmca_gpr_replica_la_LDFLAGS = -module -avoid-version

#
# Copyright (c) 2010      Cisco Systems, Inc.  All rights reserved. 
# Copyright (c) 2012      Los Alamos National Security, Inc. All rights reserved.
# $COPYRIGHT$
# 
# Additional copyrights may follow
# 
# $HEADER$
#
EXTRA_DIST = .windows

sources = \
        db_hash.h \
        db_hash_component.c \
        db_hash.c

# Make the output library in this directory, and name it either
# mca_<type>_<name>.la (for DSO builds) or libmca_<type>_<name>.la
# (for static builds).

if MCA_BUILD_orte_db_hash_DSO
component_noinst =
component_install = mca_db_hash.la
else
component_noinst = libmca_db_hash.la
component_install =
endif

mcacomponentdir = $(pkglibdir)
mcacomponent_LTLIBRARIES = $(component_install)
mca_db_hash_la_SOURCES = $(sources)
mca_db_hash_la_LDFLAGS = -module -avoid-version
mca_db_hash_la_LIBADD = $(db_hash_LIBS)

noinst_LTLIBRARIES = $(component_noinst)
libmca_db_hash_la_SOURCES =$(sources)
libmca_db_hash_la_LDFLAGS = -module -avoid-version

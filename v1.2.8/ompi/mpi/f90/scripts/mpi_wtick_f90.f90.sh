#! /bin/sh

#
# Copyright (c) 2004-2006 The Trustees of Indiana University and Indiana
#                         University Research and Technology
#                         Corporation.  All rights reserved.
# Copyright (c) 2004-2005 The Regents of the University of California.
#                         All rights reserved.
# Copyright (c) 2006      Cisco Systems, Inc.  All rights reserved.
# $COPYRIGHT$
# 
# Additional copyrights may follow
# 
# $HEADER$
#

# This flie was not generated by XML scripts; it was written by hand.
# It may be generated someday, but at this point, it was simpler to
# just write it by hand.

. "$1/fortran_kinds.sh"

# This entire file is only generated in small or larger modules.  So
# if we're not at least small, bail now.

check_size small
if test "$output" = "0"; then
    exit 0
fi

# Ok, we should continue.

cat <<EOF

function MPI_Wtick()
  implicit none
  double precision :: MPI_Wtick, foo
  call MPI_Wtick_f90(foo)
  MPI_Wtick = foo
end function MPI_Wtick

EOF

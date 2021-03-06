! -*- f90 -*-
!
! Copyright (c) 2010-2012 Cisco Systems, Inc.  All rights reserved.
! Copyright (c) 2009-2012 Los Alamos National Security, LLC.
!               All Rights reserved.
! $COPYRIGHT$

subroutine MPI_Win_test_f08(win,flag,ierror)
   use :: mpi_f08_types, only : MPI_Win
   use :: mpi_f08, only : ompi_win_test_f
   implicit none
   LOGICAL, INTENT(OUT) :: flag
   TYPE(MPI_Win), INTENT(IN) :: win
   INTEGER, OPTIONAL, INTENT(OUT) :: ierror
   integer :: c_ierror

   call ompi_win_test_f(win%MPI_VAL,flag,c_ierror)
   if (present(ierror)) ierror = c_ierror

end subroutine MPI_Win_test_f08

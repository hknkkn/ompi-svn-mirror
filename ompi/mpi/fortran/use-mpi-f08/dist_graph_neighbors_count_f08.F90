! -*- f90 -*-
!
! Copyright (c) 2010-2012 Cisco Systems, Inc.  All rights reserved.
! Copyright (c) 2009-2012 Los Alamos National Security, LLC.
!                         All rights reserved.
! $COPYRIGHT$

subroutine MPI_Dist_graph_neighbors_count_f08(comm,indegree,outdegree,weighted,ierror)
   use :: mpi_f08_types, only : MPI_Comm
   use :: mpi_f08, only : ompi_dist_graph_neighbors_count_f
   implicit none
   TYPE(MPI_Comm), INTENT(IN) :: comm
   INTEGER, INTENT(OUT) :: indegree, outdegree
   LOGICAL, INTENT(OUT) :: weighted
   INTEGER, OPTIONAL, INTENT(OUT) :: ierror
   integer :: c_ierror

   call ompi_dist_graph_neighbors_count_f(comm%MPI_VAL,indegree,outdegree,weighted,c_ierror)
   if (present(ierror)) ierror = c_ierror

end subroutine MPI_Dist_graph_neighbors_count_f08

#ifndef LAM_F77_PROTOTYPES_PMPI_H
#define LAM_F77_PROTOTYPES_PMPI_H

/*
 * $HEADER$
 */

/*
 * This file prototypes all MPI fortran functions in all four fortran
 * symbol conventions as well as all the internal "real" LAM wrapper
 * functions (different from any of the four fortran symbol
 * conventions for clarity, at the cost of more typing for me...).
 * This file is included in the lower-level build ONLY. The prototyping
 * is done ONLY for PMPI_* bindings
 */

/*
 * Zeroth, the LAM wrapper functions, with a "_f" suffix.
 */
/* This needs to be included ONLY if the top level "prototypes_mpi.h"
 * has not yet been included
 */
void pmpi_alloc_mem_f(MPI_Fint *size, MPI_Fint *info, char *baseptr, MPI_Fint *ierr);
void pmpi_comm_get_name_f(MPI_Fint *comm, char *name, MPI_Fint *l, MPI_Fint *ierror, 
                         MPI_Fint charlen);
void pmpi_comm_set_name_f(MPI_Fint *comm, char *name, MPI_Fint *ierror, MPI_Fint charlen);
void pmpi_init_f(MPI_Fint *ierror);
void pmpi_finalize_f(MPI_Fint *ierror);
void pmpi_free_mem_f(char *baseptr, MPI_Fint *ierr);
void pmpi_group_compare_f(MPI_Fint *group1, MPI_Fint *group2,
                          MPI_Fint *result, MPI_Fint *ierror);

/*
 * First, all caps.
 */
void PMPI_ALLOC_MEM(MPI_Fint *size, MPI_Fint *info, char *baseptr, MPI_Fint *ierr);
void PMPI_COMM_GET_NAME(MPI_Fint *comm, char *name, MPI_Fint *l, MPI_Fint *ierror, 
                       MPI_Fint charlen);
void PMPI_COMM_SET_NAME(MPI_Fint *comm, char *name, MPI_Fint *ierror, MPI_Fint charlen);
void PMPI_INIT(MPI_Fint *ierror);
void PMPI_FINALIZE(MPI_Fint *ierror);
void PMPI_FREE_MEM(char *baseptr, MPI_Fint *ierr);
void PMPI_GROUP_COMPARE(MPI_Fint *group1, MPI_Fint *group2,
                        MPI_Fint *result, MPI_Fint *ierror);

/*
 * Second, all lower case.
 */
void pmpi_alloc_mem(MPI_Fint *size, MPI_Fint *info, char *baseptr, MPI_Fint *ierr);
void pmpi_comm_get_name(MPI_Fint *comm, char *name, MPI_Fint *l, MPI_Fint *ierror, 
                       MPI_Fint charlen);
void pmpi_comm_set_name(MPI_Fint *comm, char *name, MPI_Fint *ierror, MPI_Fint charlen);
void pmpi_init(MPI_Fint *ierror);
void pmpi_finalize(MPI_Fint *ierror);
void pmpi_free_mem(char *baseptr, MPI_Fint *ierr);
void pmpi_group_compare(MPI_Fint *group1, MPI_Fint *group2,
                        MPI_Fint *result, MPI_Fint *ierror);

/*
 * Third, one trailing underscore.
 */
void pmpi_alloc_mem_(MPI_Fint *size, MPI_Fint *info, char *baseptr, MPI_Fint *ierr);
void pmpi_comm_get_name_(MPI_Fint *comm, char *name, MPI_Fint *l, MPI_Fint *ierror, 
                        MPI_Fint charlen);
void pmpi_comm_set_name_(MPI_Fint *comm, char *name, MPI_Fint *ierror, MPI_Fint charlen);
void pmpi_init_(MPI_Fint *ierror);
void pmpi_finalize_(MPI_Fint *ierror);
void pmpi_free_mem_(char *baseptr, MPI_Fint *ierr);
void pmpi_group_compare_(MPI_Fint *group1, MPI_Fint *group2,
                         MPI_Fint *result, MPI_Fint *ierror);

/*
 * Fourth, two trailing underscores.
 */
void pmpi_alloc_mem__(MPI_Fint *size, MPI_Fint *info, char *baseptr, MPI_Fint *ierr);
void pmpi_comm_get_name__(MPI_Fint *comm, char *name, MPI_Fint *l, MPI_Fint *ierror, 
                         MPI_Fint charlen);
void pmpi_comm_set_name__(MPI_Fint *comm, char *name, MPI_Fint *ierror, MPI_Fint charlen);
void pmpi_init__(MPI_Fint *ierror);
void pmpi_finalize__(MPI_Fint *ierror);
void pmpi_free_mem__(char *baseptr, MPI_Fint *ierr);
void pmpi_group_compare__(MPI_Fint *group1, MPI_Fint *group2,
                          MPI_Fint *result, MPI_Fint *ierror);

#endif /* PROTOTYPE_H */

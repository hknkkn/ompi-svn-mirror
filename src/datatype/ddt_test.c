/* -*- Mode: C; c-basic-offset:4 ; -*- */

#include "datatype.h"
#include "datatype_internal.h"
#include <time.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#define TIMER_DATA_TYPE struct timeval
#define GET_TIME(TV)   gettimeofday( &(TV), NULL )
#define ELAPSED_TIME(TSTART, TEND)  (((TEND).tv_sec - (TSTART).tv_sec) * 1000000 + ((TEND).tv_usec - (TSTART).tv_usec))

int mpich_typeub( void )
{
   int errs = 0;
   long extent, lb, extent1, extent2, extent3;
   long displ[2];
   int blens[2];
   dt_desc_t *type1, *type2, *type3, *types[2];

   lam_ddt_create_vector( 2, 1, 4, &(basicDatatypes[DT_INT]), &type1 );
   lam_ddt_commit( &type1 );
   lam_ddt_get_extent( type1, &lb, &extent );
   extent1 = 5 * sizeof(int);
   if (extent != extent1) {
      printf("EXTENT 1 %ld != %ld\n",extent,extent1);
      errs++;
      printf("extent(type1)=%ld\n",(long)extent);
   }

   blens[0]=1;
   blens[1]=1;
   displ[0]=0;
   displ[1]=sizeof(int)*4;
   types[0]=type1;
   types[1]=&(basicDatatypes[DT_UB]);
   extent2 = displ[1];

   /*    using MPI_UB and Type_struct, monkey with the extent, making it 16
    */
   lam_ddt_create_struct( 2, blens, displ, types, &type2 );
   lam_ddt_commit( &type2 );
   lam_ddt_get_extent( type2, &lb, &extent );
   if (extent != extent2) {
      printf("EXTENT 2 %ld != %ld\n",extent,extent2);
      errs++;
      printf("extent(type2)=%ld\n",(long)extent);
   }

   /*    monkey with the extent again, making it 4
    *     ===> MPICH gives 4
    *     ===> MPIF gives 16, the old extent
    */
   displ[1]=sizeof(int);
   types[0]=type2;
   types[1]=&(basicDatatypes[DT_UB]);
   extent3 = extent2;

   lam_ddt_create_struct( 2, blens, displ, types, &type3 );
   lam_ddt_commit( &type3 );

   lam_ddt_get_extent( type3, &lb, &extent );
   if (extent != extent3) {
      printf("EXTENT 3 %ld != %ld\n",extent,extent3);
      errs++;
      printf("extent(type3)=%ld\n",(long)extent);
   }

   OBJ_RELEASE( type1 );
   assert( type1 == NULL );
   OBJ_RELEASE( type2 );
   assert( type2 == NULL );
   OBJ_RELEASE( type3 );
   assert( type3 == NULL );
   return errs;
}

int mpich_typeub2( void )
{
   int blocklen[3], err = 0, sz1, sz2, sz3;
   long disp[3], lb, ub, ex1, ex2, ex3;
   dt_desc_t *types[3], *dt1, *dt2, *dt3;

   blocklen[0] = 1;
   blocklen[1] = 1;
   blocklen[2] = 1;
   disp[0] = -3;
   disp[1] = 0;
   disp[2] = 6;
   types[0] = &(basicDatatypes[DT_LB]);
   types[1] = &(basicDatatypes[DT_INT]);
   types[2] = &(basicDatatypes[DT_UB]);

   lam_ddt_create_struct(3,blocklen,disp, types,&dt1);
   lam_ddt_commit(&dt1);

   lam_ddt_type_lb(dt1, &lb);          lam_ddt_type_ub(dt1, &ub);
   lam_ddt_type_extent(dt1,&ex1);      lam_ddt_type_size(dt1,&sz1);

   /* Values should be lb = -3, ub = 6 extent 9; size depends on implementation */
   if (lb != -3 || ub != 6 || ex1 != 9) {
      printf("Example 3.26 type1 lb %d ub %d extent %d size %d\n", (int)lb, (int)ub, (int)ex1, sz1);
      err++;
   }
   else
      printf("Example 3.26 type1 correct\n" );

   lam_ddt_create_contiguous(2,dt1,&dt2);
   lam_ddt_type_lb(dt2, &lb);          lam_ddt_type_ub(dt2, &ub);
   lam_ddt_type_extent(dt2,&ex2);      lam_ddt_type_size(dt2,&sz2);
   /* Values should be lb = -3, ub = 15, extent = 18, size depends on implementation */
   if (lb != -3 || ub != 15 || ex2 != 18) {
      printf("Example 3.26 type2 lb %d ub %d extent %d size %d\n", (int)-3, (int)15, (int)18, 8);
      printf("Example 3.26 type2 lb %d ub %d extent %d size %d\n", (int)lb, (int)ub, (int)ex2, sz2);
      err++;
   }
   else
      printf("Example 3.26 type1 correct\n" );
   OBJ_RELEASE( dt2 ); assert( dt2 == NULL );
   lam_ddt_create_contiguous(2,dt1,&dt2);
   lam_ddt_type_lb(dt2, &lb);          lam_ddt_type_ub(dt2, &ub);
   lam_ddt_type_extent(dt2,&ex2);      lam_ddt_type_size(dt2,&sz2);
   /* Values should be lb = -3, ub = 15, extent = 18, size depends on implementation */
   if (lb != -3 || ub != 15 || ex2 != 18) {
      printf("Example 3.26 type2 lb %d ub %d extent %d size %d\n", (int)-3, (int)15, (int)18, 8);
      printf("Example 3.26 type2 lb %d ub %d extent %d size %d\n", (int)lb, (int)ub, (int)ex2, sz2);
      err++;
   }
   else
      printf( "Example 3.26 type2 correct\n" );

   types[0]=dt1;               types[1]=dt1;
   blocklen[0]=1;              blocklen[1]=1;
   disp[0]=0;                  disp[1]=ex1;

   lam_ddt_create_struct(2, blocklen, disp, types, &dt3);
   lam_ddt_commit(&dt3);

   lam_ddt_type_lb(dt3, &lb);          lam_ddt_type_ub(dt3, &ub);
   lam_ddt_type_extent(dt3,&ex3);      lam_ddt_type_size(dt3,&sz3);
   /* Another way to express type2 */
   if (lb != -3 || ub != 15 || ex3 != 18) {
      printf("type3 lb %d ub %d extent %d size %d\n", (int)-3, (int)15, (int)18, 8);
      printf("type3 lb %d ub %d extent %d size %d\n", (int)lb, (int)ub, (int)ex3, sz2);
      err++;
   }
   else
      printf( "type3 correct\n" );

   OBJ_RELEASE( dt1 ); assert( dt1 == NULL );
   OBJ_RELEASE( dt2 ); assert( dt2 == NULL );
   OBJ_RELEASE( dt3 ); assert( dt3 == NULL );
   return err;
}

int mpich_typeub3( void )
{
   int blocklen[2], sz, err = 0, idisp[3];
   long disp[3], lb, ub, ex;
   dt_desc_t *types[3], *dt1, *dt2, *dt3, *dt4, *dt5;

   /* Create a datatype with explicit LB and UB */
   blocklen[0] = 1;
   blocklen[1] = 1;
   blocklen[2] = 1;
   disp[0] = -3;
   disp[1] = 0; 
   disp[2] = 6;
   types[0] = &(basicDatatypes[DT_LB]);
   types[1] = &(basicDatatypes[DT_INT]);
   types[2] = &(basicDatatypes[DT_UB]);
   
   /* Generate samples for contiguous, hindexed, hvector, indexed, and vector (struct and contiguous tested in typeub2) */                                                                                                                         
   lam_ddt_create_struct(3,blocklen,disp, types,&dt1);
   lam_ddt_commit(&dt1);

   /* This type is the same as in typeub2, and is tested there */
   types[0]=dt1;               types[1]=dt1;
   blocklen[0]=1;              blocklen[1]=1;
   disp[0]=-4;                 disp[1]=7;
   idisp[0]=-4;                idisp[1]=7;

   lam_ddt_create_hindexed( 2, blocklen, disp, dt1, &dt2 );
   lam_ddt_commit( &dt2 );

   lam_ddt_type_lb( dt2, &lb );       lam_ddt_type_ub( dt2, &ub );
   lam_ddt_type_extent( dt2, &ex );   lam_ddt_type_size( dt2, &sz );

   if (lb != -7 || ub != 13 || ex != 20) {
      printf("hindexed lb %d ub %d extent %d size %d\n", (int)-7, (int)13, (int)20, sz);
      printf("hindexed lb %d ub %d extent %d size %d\n", (int)lb, (int)ub, (int)ex, sz);
      err++;
   }
   else
      printf( "hindexed ok\n" );

   lam_ddt_create_indexed( 2, blocklen, idisp, dt1, &dt3 );
   lam_ddt_commit( &dt3 );

   lam_ddt_type_lb( dt3, &lb );       lam_ddt_type_ub( dt3, &ub );
   lam_ddt_type_extent( dt3, &ex );   lam_ddt_type_size( dt3, &sz );

   if (lb != -39 || ub != 69 || ex != 108) {
      printf("indexed lb %d ub %d extent %d size %d\n", (int)-39, (int)69, (int)108, sz);
      printf("indexed lb %d ub %d extent %d size %d\n", (int)lb, (int)ub, (int)ex, sz);
      err++;
   }
   else
      printf( "indexed ok\n" );

   lam_ddt_create_hvector( 2, 1, 14, dt1, &dt4 );
   lam_ddt_commit( &dt4 );

   lam_ddt_type_lb( dt4, &lb );       lam_ddt_type_ub( dt4, &ub );
   lam_ddt_type_extent( dt4, &ex );   lam_ddt_type_size( dt4, &sz );

   if (lb != -3 || ub != 20 || ex != 23) {
      printf("hvector lb %d ub %d extent %d size %d\n", (int)-3, (int)20, (int)23, sz);
      printf("hvector lb %d ub %d extent %d size %d\n", (int)lb, (int)ub, (int)ex, sz);
      err++;
   }
   else
      printf( "hvector ok\n" );

   lam_ddt_create_vector( 2, 1, 14, dt1, &dt5 );
   lam_ddt_commit( &dt5 );

   lam_ddt_type_lb( dt5, &lb );       lam_ddt_type_ub( dt5, &ub );
   lam_ddt_type_extent( dt5, &ex );   lam_ddt_type_size( dt5, &sz );

   if (lb != -3 || ub != 132 || ex != 135) {
      printf("vector lb %d ub %d extent %d size %d\n", (int)-3, (int)132, (int)135, sz);
      printf("vector lb %d ub %d extent %d size %d\n", (int)lb, (int)ub, (int)ex, sz);
      err++;
   }
   else
      printf( "vector ok\n" );

   OBJ_RELEASE( dt1 ); assert( dt1 == NULL );
   OBJ_RELEASE( dt2 ); assert( dt2 == NULL );
   OBJ_RELEASE( dt3 ); assert( dt3 == NULL );
   OBJ_RELEASE( dt4 ); assert( dt4 == NULL );
   OBJ_RELEASE( dt5 ); assert( dt5 == NULL );
   return err;
}

void print_double_mat( size_t N, double* mat )
{
   int i, j;
   double* pMat;

   for( i = 0; i < N; i++ ) {
      printf( "(%4d) :", i * N * sizeof(double) );
      pMat = mat + i * N;
      for( j = 0; j < N; j++ ) {
         printf( "%5.1f ", *pMat );
         pMat++;
      }
      printf( "\n" );
   }
}

int init_random_upper_matrix( size_t N, double* mat )
{
    int i, j;

    srand( time(NULL) );
    for( i = 0; i < N; i++ ) {
        mat += i;
        for( j = i; j < N; j++ ) {
            *mat = (double)random();
            mat++;
        }
    }
    return 0;  
}

int check_diag_matrix( size_t N, double* mat1, double* mat2 )
{
   int i, j;

   for( i = 0; i < N; i++ ) {
      mat1 += i;
      mat2 += i;
      for( j = i; j < N; j++ ) {
         if( *mat1 != *mat2 ) {
            printf( "error in position (%d, %d) expect %f and find %f\n",
                    i, j, *mat1, *mat2 );
            return -1;
         }
         mat1++; mat2++;
      }
   }
   return 0;
}

dt_desc_t* upper_matrix( size_t mat_size )
{
   int *disp, i;
   size_t *blocklen;
   dt_desc_t* upper;

   disp = (int*)malloc( sizeof(int) * mat_size );
   blocklen = (size_t*)malloc( sizeof(size_t) * mat_size );

   for( i = 0; i < mat_size; i++ ) {
      disp[i] = i * mat_size + i;
      blocklen[i] = mat_size - i;
   }

   lam_ddt_create_indexed( mat_size, blocklen, disp, &(basicDatatypes[DT_DOUBLE]),
                      &upper );
   free( disp );
   free( blocklen );
   return upper;
}

dt_desc_t* lower_matrix( size_t mat_size )
{
   int *disp, i;
   size_t *blocklen;
   dt_desc_t* upper;

   disp = (int*)malloc( sizeof(int) * mat_size );
   blocklen = (size_t*)malloc( sizeof(size_t) * mat_size );

   for( i = 0; i < mat_size; i++ ) {
      disp[i] = i * mat_size;
      blocklen[i] = i;
   }

   lam_ddt_create_indexed( mat_size, blocklen, disp, &(basicDatatypes[DT_DOUBLE]),
                      &upper );
   free( disp );
   free( blocklen );
   return upper;
}

extern long conversion_elapsed;

int test_upper( size_t length )
{
   double *mat1, *mat2, *inbuf;
   dt_desc_t *pdt, *pdt1;
   lam_convertor_t * pConv;
   char *ptr;
   int i, j, split_chunk, total_length, rc;
   struct iovec a;
   TIMER_DATA_TYPE start, end;
   long total_time;

   printf( "test upper matrix\n" );
   pdt = upper_matrix( length );
   pdt1 = lower_matrix( length );
   /*dt_dump( pdt );*/

   mat1 = malloc( length * length * sizeof(double) );
   init_random_upper_matrix( length, mat1 );
   mat2 = calloc( length * length, sizeof(double) );

   total_length = length * (length + 1) * ( sizeof(double) / 2);
   inbuf = (double*)malloc( total_length );
   ptr = (char*)inbuf;
   /* copy upper matrix in the array simulating the input buffer */
   for( i = 0; i < length; i++ )
      for( j = i; j < length; j++ ) {
         *inbuf = mat1[i * length + j];
         inbuf++;
      }
   inbuf = (double*)ptr;
   pConv = lam_convertor_create( 0, 0 );
   lam_convertor_init_for_recv( pConv, 0, pdt, 1, mat2, 0 );

/* test the automatic destruction pf the data */
   lam_ddt_destroy( &pdt ); assert( pdt == NULL );
   lam_ddt_destroy( &pdt1 ); assert( pdt1 == NULL );

   GET_TIME( start );
   split_chunk = (length + 1) * sizeof(double);
/*    split_chunk = (total_length + 1) * sizeof(double); */
   for( i = total_length; i > 0; ) {
      if( i < split_chunk ) split_chunk = i;
      a.iov_base = ptr;
      a.iov_len = split_chunk;
      lam_convertor_unpack( pConv, &a, 1 );
      ptr += split_chunk;
      i -= split_chunk;
      if( mat2[0] != inbuf[0] ) assert(0);
   }
   GET_TIME( end );
   total_time = ELAPSED_TIME( start, end );
   printf( "complete unpacking in %ld microsec\n", total_time );
/*    printf( "conversion done in %ld microsec\n", conversion_elapsed ); */
/*    printf( "stack management in %ld microsec\n", total_time - conversion_elapsed ); */
   free( inbuf );
   rc = check_diag_matrix( length, mat1, mat2 );
   free( mat1 );
   free( mat2 );
   OBJ_RELEASE( pConv );
   return rc;
}

dt_desc_t* test_matrix_borders( unsigned int size, unsigned int width )
{
   dt_desc_t *pdt, *pdt_line;
   int disp[2];
   size_t blocklen[2];
   
   disp[0] = 0;
   blocklen[0] = width;
   disp[1] = (size - width) * sizeof(double);
   blocklen[1] = width;

   lam_ddt_create_indexed( 2, blocklen, disp, &(basicDatatypes[DT_DOUBLE]),
                      &pdt_line );
   lam_ddt_create_contiguous( size, pdt_line, &pdt );
   OBJ_RELEASE( pdt_line ); assert( pdt_line == NULL );
   return pdt;
}

dt_desc_t* test_contiguous( void )
{
   dt_desc_t *pdt, *pdt1, *pdt2;

   printf( "test contiguous (alignement)\n" );
   pdt1 = lam_ddt_create( -1 );
   lam_ddt_add( pdt1, &(basicDatatypes[DT_DOUBLE]), 1, 0, -1 );
   lam_ddt_dump( pdt1 );
   lam_ddt_add( pdt1, &(basicDatatypes[DT_CHAR]), 1, 8, -1 );
   lam_ddt_dump( pdt1 );
   lam_ddt_create_contiguous( 4, pdt1, &pdt2 );
   OBJ_RELEASE( pdt1 ); assert( pdt1 == NULL );
   lam_ddt_dump( pdt2 );
   lam_ddt_create_contiguous( 2, pdt2, &pdt );
   OBJ_RELEASE( pdt2 ); assert( pdt2 == NULL );
   lam_ddt_dump( pdt );
   lam_ddt_dump_complete( pdt );
   return pdt;
}

dt_desc_t* test_struct( void )
{
   dt_desc_t* types[] = { &(basicDatatypes[DT_FLOAT]),
                          NULL, 
                          &(basicDatatypes[DT_CHAR]) };
   int lengths[] = { 2, 1, 3 };
   long disp[] = { 0, 16, 26 };
   dt_desc_t* pdt, *pdt1;
   
   printf( "test struct\n" );
   pdt1 = lam_ddt_create( -1 );
   lam_ddt_add( pdt1, &(basicDatatypes[DT_DOUBLE]), 1, 0, -1 );
   lam_ddt_add( pdt1, &(basicDatatypes[DT_CHAR]), 1, 8, -1 );
   lam_ddt_dump_complete( pdt1 );

   types[1] = pdt1;

   lam_ddt_create_struct( 3, lengths, disp, types, &pdt );
   OBJ_RELEASE( pdt1 ); assert( pdt1 == NULL );
   lam_ddt_dump_complete( pdt );
   return pdt;
}

typedef struct {
   int i1;
   int gap;
   int i2;
} sdata_intern;

typedef struct {
   int counter;
   sdata_intern v[10];
   int last;
} sstrange;

#define SSTRANGE_CNT 10
#define USE_RESIZED

dt_desc_t* create_strange_dt( void )
{
    sdata_intern v[2];
    long displ[3];
    dt_desc_t* types[3] = { &(basicDatatypes[DT_INT]) };
    sstrange t[2];
    int pBlock[3] = {1, 10, 1}, dispi[3];
    dt_desc_t *pdt, *pdt1, *pdt2, *pdtTemp;

    dispi[0] = (int)((char*)&(v[0].i1) - (char*)&(v[0]));  /* 0 */
    dispi[1] = (int)(((char*)(&(v[0].i2)) - (char*)&(v[0])) / sizeof(int));  /* 2 */
    lam_ddt_create_indexed_block( 2, 1, dispi, &(basicDatatypes[DT_INT]), &pdtTemp );
#ifdef USE_RESIZED
    /* optional */
    displ[0] = 0;
    displ[1] = (char*)&(v[1]) - (char*)&(v[0]);
    lam_ddt_create_resized( pdtTemp, displ[0], displ[1], &pdt1 );
    OBJ_RELEASE( pdtTemp ); assert( pdtTemp == NULL );
#else
    pdt1 = pdtTemp;
#endif  /* USE_RESIZED */

    types[1] = pdt1;
    types[2] = &(basicDatatypes[DT_INT]);
    displ[0] = 0;
    displ[1] = (long)((char*)&(t[0].v[0]) - (char*)&(t[0]));
    displ[2] = (long)((char*)&(t[0].last) - (char*)&(t[0]));
    lam_ddt_create_struct( 3, pBlock, displ, types, &pdtTemp );
#ifdef USE_RESIZED
    /* optional */
    displ[1] = (char*)&(t[1]) - (char*)&(t[0]);
    lam_ddt_create_resized( pdtTemp, displ[0], displ[1], &pdt2 );
    OBJ_RELEASE( pdtTemp ); assert( pdtTemp == NULL );
#else
    pdt2 = pdtTemp;
#endif  /* USE_RESIZED */

    lam_ddt_create_contiguous( SSTRANGE_CNT, pdt2, &pdt );

    OBJ_RELEASE( pdt1 ); assert( pdt1 == NULL );
    OBJ_RELEASE( pdt2 ); assert( pdt2 == NULL );
    lam_ddt_dump( pdt );
    {
        dt_type_desc_t pElemDesc = { 0, 0, NULL };
        lam_ddt_optimize_short( pdt, 1, &pElemDesc );
        if( pElemDesc.desc != NULL ) free( pElemDesc.desc );
    }
    return pdt;
}

int local_copy_ddt_count( dt_desc_t* pdt, int count )
{
   long extent;
   void *pdst, *psrc;
   lam_ddt_type_extent( pdt, &extent );

   pdst = malloc( extent * count );
   psrc = malloc( extent * count );

   pdt = create_strange_dt();

   lam_ddt_copy_content_same_ddt( pdt, count, pdst, psrc );

   free( pdst );
   free( psrc );

   OBJ_RELEASE( pdt ); assert( pdt == NULL );
   return 0;
}
int main( int argc, char* argv[] )
{
   dt_desc_t *pdt, *pdt1, *pdt2, *pdt3;
   int rc, length = 500;

   lam_ddt_init();

   pdt = create_strange_dt();
   OBJ_RELEASE( pdt ); assert( pdt == NULL );
   
   pdt = upper_matrix(100);
   local_copy_ddt_count(pdt, 1);
   OBJ_RELEASE( pdt ); assert( pdt == NULL );

   mpich_typeub();
   mpich_typeub2();
   mpich_typeub3();

   rc = test_upper( length );
   if( rc == 0 )
      printf( "decode [PASSED]\n" );
   else
      printf( "decode [NOT PASSED]\n" );

   pdt = test_matrix_borders( length, 100 );
   lam_ddt_dump( pdt );
   OBJ_RELEASE( pdt ); assert( pdt == NULL );

   printf( ">>--------------------------------------------<<\n" );
   pdt = test_contiguous();
   OBJ_RELEASE( pdt ); assert( pdt == NULL );
   printf( ">>--------------------------------------------<<\n" );
   pdt = test_struct();
   OBJ_RELEASE( pdt ); assert( pdt == NULL );
   printf( ">>--------------------------------------------<<\n" );

   pdt1 = lam_ddt_create( -1 );
   pdt2 = lam_ddt_create( -1 );
   pdt3 = lam_ddt_create( -1 );
   lam_ddt_add( pdt3, &(basicDatatypes[DT_INT]), 10, 0, -1 );
   lam_ddt_add( pdt3, &(basicDatatypes[DT_FLOAT]), 5, 10 * sizeof(int), -1 );

   lam_ddt_add( pdt2, &(basicDatatypes[DT_INT]), 1, 0, -1 );
   lam_ddt_add( pdt2, pdt3, 3, sizeof(int) * 1, -1 );

   lam_ddt_add( pdt1, &(basicDatatypes[DT_LONG_LONG]), 5, 0, -1 );
   lam_ddt_add( pdt1, &(basicDatatypes[DT_LONG_DOUBLE]), 2, sizeof(long long) * 5, -1 );

   printf( ">>--------------------------------------------<<\n" );
   lam_ddt_dump( pdt1 );
   printf( ">>--------------------------------------------<<\n" );
   lam_ddt_dump( pdt2 );
   printf( ">>--------------------------------------------<<\n" );
   lam_ddt_dump( pdt3 );

   OBJ_RELEASE( pdt1 ); assert( pdt1 == NULL );
   OBJ_RELEASE( pdt2 ); assert( pdt2 == NULL );
   OBJ_RELEASE( pdt3 ); assert( pdt3 == NULL );

   /* clean-ups all data allocations */
   lam_ddt_finalize();

   return 0;
}

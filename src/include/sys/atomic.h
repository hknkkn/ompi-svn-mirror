/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University.
 *                         All rights reserved.
 * Copyright (c) 2004-2005 The Trustees of the University of Tennessee.
 *                         All rights reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

/** @file
 *
 * Atomic operations.
 *
 * This API is patterned after the FreeBSD kernel atomic interface
 * (which is influenced by Intel's ia64 architecture).  The
 * FreeBSD interface is documented at
 *
 * http://www.freebsd.org/cgi/man.cgi?query=atomic&sektion=9
 *
 * Only the necessary subset of functions are implemented here.
 *
 * The following #defines will be true / false based on
 * assembly support:
 *
 *  \c OMPI_HAVE_MEM_BARRIER atomic memory barriers
 *  \c OMPI_HAVE_ATOMIC_SPINLOCKS atomic spinlocks
 *  \c OMPI_HAVE_ATOMIC_MATH_32 if 32 bit add/sub/cmpset can be done "atomicly"
 *  \c OMPI_HAVE_ATOMIC_MATH_64 if 32 bit add/sub/cmpset can be done "atomicly"
 *
 * Note that for the Atomic math, atomic add/sub may be implemented as
 * C code using ompi_atomic_cmpset.  The appearance of atomic
 * operation will be upheld in these cases.
 */

#ifndef OMPI_SYS_ATOMIC_H
#define OMPI_SYS_ATOMIC_H 1

#include "ompi_config.h"

#include "include/sys/architecture.h"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif


#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

/**********************************************************************
 *
 * Load the appropriate architecture files and set some reasonable
 * default values for our support
 *
 *********************************************************************/
#if defined(DOXYGEN)
/* don't include system-level gorp when generating doxygen files */ 
#elif OMPI_ASSEMBLY_ARCH == OMPI_WINDOWS || defined(win32)
/* windows first, as they have API-level primitives for this stuff */
#include "include/sys/win32/atomic.h"
#elif OMPI_ASSEMBLY_ARCH == OMPI_ALPHA
#include "include/sys/alpha/atomic.h"
#elif OMPI_ASSEMBLY_ARCH == OMPI_AMD64
#include "include/sys/amd64/atomic.h"
#elif OMPI_ASSEMBLY_ARCH == OMPI_IA32
#include "include/sys/ia32/atomic.h"
#elif OMPI_ASSEMBLY_ARCH == OMPI_IA64
#include "include/sys/ia64/atomic.h"
#elif OMPI_ASSEMBLY_ARCH == OMPI_POWERPC32
#include "include/sys/powerpc/atomic.h"
#elif OMPI_ASSEMBLY_ARCH == OMPI_POWERPC64
#include "include/sys/powerpc/atomic.h"
#elif OMPI_ASSEMBLY_ARCH == OMPI_SPARC32
#error "32 bit Sparc support not implemented yet"
#elif OMPI_ASSEMBLY_ARCH == OMPI_SPARC64
#include "include/sys/sparc64/atomic.h"
#endif

/* compare and set operations can't really be emulated from software,
   so if these defines aren't already set, they should be set to 0
   now */
#ifndef OMPI_HAVE_ATOMIC_CMPSET_32
#define OMPI_HAVE_ATOMIC_CMPSET_32 0
#endif
#ifndef OMPI_HAVE_ATOMIC_CMPSET_64
#define OMPI_HAVE_ATOMIC_CMPSET_64 0
#endif

/**********************************************************************
 *
 * Memory Barriers - defined here if running doxygen or have barriers
 *                   but can't inline
 *
 *********************************************************************/
#ifndef OMPI_HAVE_ATOMIC_MEM_BARRIER
/* no way to emulate in C code */
#define OMPI_HAVE_ATOMIC_MEM_BARRIER 0
#endif

#if defined(DOXYGEN) || OMPI_HAVE_ATOMIC_MEM_BARRIER
/**
 * Memory barrier
 */
void ompi_atomic_mb(void);

/**
 * Read memory barrier
 */
void ompi_atomic_rmb(void);

/**
 * Write memory barrier.
 */
void ompi_atomic_wmb(void);

#endif /* defined(DOXYGEN) || OMPI_HAVE_MEM_BARRIER */


/**********************************************************************
 *
 * Atomic spinlocks - always inlined, if have atomic cmpset
 *
 *********************************************************************/
/**
 * Volatile lock object (with optional padding).
 */
struct ompi_lock_t {
    union {
        volatile int lock;         /**< The lock address (an integer) */
        char padding[sizeof(int)]; /**< Array for optional padding */
    } u;
};
typedef struct ompi_lock_t ompi_lock_t;

#ifndef OMPI_HAVE_ATOMIC_SPINLOCKS
#define OMPI_HAVE_ATOMIC_SPINLOCKS (OMPI_HAVE_ATOMIC_CMPSET_32 || OMPI_HAVE_ATOMIC_CMPSET_64)
#endif

#if defined(DOXYGEN) || OMPI_HAVE_ATOMIC_SPINLOCKS


/**
 * Enumeration of lock states
 */
enum {
    OMPI_ATOMIC_UNLOCKED = 0,
    OMPI_ATOMIC_LOCKED = 1
};


/**
 * Initialize a lock to value
 *
 * @param lock         Address of the lock
 * @param value        Initial value to set lock to
 */
static inline void ompi_atomic_init(ompi_lock_t* lock, int value);


/**
 * Try to acquire a lock.
 *
 * @param lock          Address of the lock.
 * @return              0 if the lock was acquired, 1 otherwise.
 */
static inline int ompi_atomic_trylock(ompi_lock_t *lock);


/**
 * Acquire a lock by spinning.
 *
 * @param lock          Address of the lock.
 */
static inline void ompi_atomic_lock(ompi_lock_t *lock);


/**
 * Release a lock.
 *
 * @param lock          Address of the lock.
 */
static inline void ompi_atomic_unlock(ompi_lock_t *lock);

#endif /* OMPI_HAVE_ATOMIC_SPINLOCKS */


/**********************************************************************
 *
 * Atomic math operations
 *
 *********************************************************************/
#ifndef OMPI_HAVE_ATOMIC_CMPSET_32
#define OMPI_HAVE_ATOMIC_CMPSET_32 0
#endif
#if defined(DOXYGEN) || OMPI_HAVE_ATOMIC_CMPSET_32
int ompi_atomic_cmpset_32(volatile int32_t *addr, int32_t oldval, 
                          int32_t newval);
int ompi_atomic_cmpset_acq_32(volatile int32_t *addr, int32_t oldval, 
                              int32_t newval);
int ompi_atomic_cmpset_rel_32(volatile int32_t *addr, int32_t oldval, 
                              int32_t newval);
#endif


#ifndef OMPI_HAVE_ATOMIC_CMPSET_64
#define OMPI_HAVE_ATOMIC_CMPSET_64 0
#endif
#if defined(DOXYGEN) || OMPI_HAVE_ATOMIC_CMPSET_64
int ompi_atomic_cmpset_64(volatile int64_t *addr, int64_t oldval, 
                          int64_t newval);
int ompi_atomic_cmpset_acq_64(volatile int64_t *addr, int64_t oldval, 
                              int64_t newval);
int ompi_atomic_cmpset_rel_64(volatile int64_t *addr, int64_t oldval, 
                              int64_t newval);
#endif

#ifndef OMPI_HAVE_ATOMIC_MATH_32
/* define to 0 for these tests.  WIll fix up later. */
#define OMPI_HAVE_ATOMIC_MATH_32 0
#endif
#if defined(DOXYGEN) ||  OMPI_HAVE_ATOMIC_MATH_32 || OMPI_HAVE_ATOMIC_CMPSET_32
#if ! OMPI_HAVE_ATOMIC_MATH_32
static inline
#endif
int32_t ompi_atomic_add_32(volatile int32_t *addr, int delta);
#if ! OMPI_HAVE_ATOMIC_MATH_32
static inline
#endif
int32_t ompi_atomic_sub_32(volatile int32_t *addr, int delta);
#endif /* OMPI_HAVE_ATOMIC_MATH_32 */
#if ! OMPI_HAVE_ATOMIC_MATH_32
/* fix up the value of ompi_have_atomic_math_32 to allow for C versions */
#undef OMPI_HAVE_ATOMIC_MATH_32
#define OMPI_HAVE_ATOMIC_MATH_32 OMPI_HAVE_ATOMIC_CMPSET_32
#endif

#ifndef OMPI_HAVE_ATOMIC_MATH_64
/* define to 0 for these tests.  WIll fix up later. */
#define OMPI_HAVE_ATOMIC_MATH_64 0
#endif
#if defined(DOXYGEN) || OMPI_HAVE_ATOMIC_MATH_64 || OMPI_HAVE_ATOMIC_CMPSET_64
#if OMPI_HAVE_ATOMIC_CMPSET_64
static inline
#endif
int64_t ompi_atomic_add_64(volatile int64_t *addr, int64_t delta);
#if OMPI_HAVE_ATOMIC_CMPSET_64
static inline 
#endif
int64_t ompi_atomic_sub_64(volatile int64_t *addr, int64_t delta);
#endif /* OMPI_HAVE_ATOMIC_MATH_32 */
#if ! OMPI_HAVE_ATOMIC_MATH_64
/* fix up the value of ompi_have_atomic_math_64 to allow for C versions */
#undef OMPI_HAVE_ATOMIC_MATH_64
#define OMPI_HAVE_ATOMIC_MATH_64 OMPI_HAVE_ATOMIC_CMPSET_64
#endif


#if defined(DOXYGEN) || (OMPI_HAVE_ATOMIC_CMPSET_32 || OMPI_HAVE_ATOMIC_CMPSET_64)
/* these are always done with inline functions, so always mark as
   static inline */
static inline int ompi_atomic_cmpset_xx(volatile void* addr, int64_t oldval,
                                        int64_t newval, size_t length);
static inline int ompi_atomic_cmpset_acq_xx(volatile void* addr, 
                                            int64_t oldval,  int64_t newval, 
                                            size_t length);
static inline int ompi_atomic_cmpset_rel_xx(volatile void* addr, 
                                            int64_t oldval, int64_t newval, 
                                            size_t length);
static inline void ompi_atomic_add_xx(volatile void* addr, 
                                      int32_t value, size_t length);
static inline void ompi_atomic_sub_xx(volatile void* addr, 
                                      int32_t value, size_t length);


/**
 * Atomic compare and set of pointer with relaxed semantics. This
 * macro detect at compile time the type of the first argument 
 * and choose the correct function to be called.
 *
 * @param addr          Address of <TYPE>.
 * @param oldval        Comparison value <TYPE>.
 * @param newval        New value to set if comparision is true <TYPE>.
 *
 * See ompi_atomic_cmpset_* for pseudo-code.
 */
#define ompi_atomic_cmpset( ADDR, OLDVAL, NEWVAL )                  \
   ompi_atomic_cmpset_xx( (volatile void*)(ADDR), (int64_t)(OLDVAL), \
                          (int64_t)(NEWVAL), sizeof(*(ADDR)) )


/**
 * Atomic compare and set of pointer with acquire semantics. This
 * macro detect at compile time the type of the first argument 
 * and choose the correct function to be called.
 *
 * @param addr          Address of <TYPE>.
 * @param oldval        Comparison value <TYPE>.
 * @param newval        New value to set if comparision is true <TYPE>.
 *
 * See ompi_atomic_cmpset_acq_* for pseudo-code.
 */
#define ompi_atomic_cmpset_acq( ADDR, OLDVAL, NEWVAL )           \
   ompi_atomic_cmpset_acq_xx( (volatile void*)(ADDR), (int64_t)(OLDVAL), \
                              (int64_t)(NEWVAL), sizeof(*(ADDR)) )


/**
 * Atomic compare and set of pointer with release semantics. This
 * macro detect at compile time the type of the first argument 
 * and choose the correct function to b
 *
 * @param addr          Address of <TYPE>.
 * @param oldval        Comparison value <TYPE>.
 * @param newval        New value to set if comparision is true <TYPE>.
 *
 * See ompi_atomic_cmpsetrel_* for pseudo-code.
 */
#define ompi_atomic_cmpset_rel( ADDR, OLDVAL, NEWVAL )           \
   ompi_atomic_cmpset_rel_xx( (volatile void*)(ADDR), (int64_t)(OLDVAL), \
                              (int64_t)(NEWVAL), sizeof(*(ADDR)) )


/**
 * Atomically increment the content depending on the type. This
 * macro detect at compile time the type of the first argument 
 * and choose the correct function to be called.
 *
 * @param addr          Address of <TYPE>
 * @param delta         Value to add (converted to <TYPE>).
 */
#define ompi_atomic_add( ADDR, VALUE )                                  \
   ompi_atomic_add_xx( (volatile void*)(ADDR), (int32_t)(VALUE), \
                       sizeof(*(ADDR)) )

/**
 * Atomically decrement the content depending on the type. This
 * macro detect at compile time the type of the first argument 
 * and choose the correct function to be called.
 *
 * @param addr          Address of <TYPE>
 * @param delta         Value to substract (converted to <TYPE>).
 */
#define ompi_atomic_sub( ADDR, VALUE )                                  \
   ompi_atomic_sub_xx( (volatile void*)(ADDR), (int32_t)(VALUE),        \
                      sizeof(*(ADDR)) )

#endif /* OMPI_HAVE_ATOMIC_MATH_32 || OMPI_HAVE_ATOMIC_MATH_64 */


/**********************************************************************
 *
 * Include system specific inline asm definitions. Otherwise
 * the definitions are in system specific .s files in src/util.
 *
 *********************************************************************/
#include "include/sys/atomic_impl.h"


#if defined(c_plusplus) || defined(__cplusplus)
}
#endif

#endif /* OMPI_SYS_ATOMIC_H */

/*
 * $HEADER$
 */

#ifndef _OMPI_FIFO
#define _OMPI_FIFO

#include "include/constants.h"
#include "include/sys/cache.h"
#include "os/atomic.h"
#include "mca/mpool/mpool.h"
#include "class/ompi_circular_buffer_fifo.h"


/** @file
 *
 *  This defines a set of functions to create, and manipulate a FIFO
 *  implemented as a link list of circular buffer FIFO's.  FIFO 
 *  elements are assumed to be pointers.  Pointers are written to 
 *  the head, and read from the tail.  For thread safety, a spin 
 *  lock is provided in the !!!!!ompi_cb_fifo_ctl_t!!!! structure, but it's use must be managed by
 *  the calling routines - this is not by these set of routines.
 *  When a write to a circular buffer queue will overflow that queue,
 *  the next cirular buffer queue if the link list is used, if it is
 *  empty, or a new one is inserted into the list.
 */

/*
 * Structure by the the  ompi_fifo routines to keep track of some
 * extra queue information not needed by the ompi_cb_fifo routines.
 */
struct ompi_cb_fifo_wrapper_t {

    /* pointer to ompi_cb_fifo_ctl_t structure in use */
    ompi_cb_fifo_ctl_t *cb_fifo;

    /* pointer to next ompi_cb_fifo_ctl_t structure */
    volatile struct ompi_cb_fifo_wrapper_t *next_fifo_wrapper;

    /* pointer to allocator functions */
    mca_mpool_base_module_t *fifo_allocator;

    /* flag indicating if cb_fifo has over flown - need this to force
     * release of entries already read */
    volatile bool cb_overflow;

};

typedef ompi_cb_fifo_wrapper_t ompi_cb_fifo_wrapper_t;

/* data structure used to describe the fifo */
struct ompi_fifo_t {

    /* pointer to head (write) ompi_cb_fifo_t structure */
    ompi_cb_fifo_wrapper_t *head;

    /* pointer to tail (read) ompi_cb_fifo_t structure */
    ompi_cb_fifo_wrapper_t *tail;

};

typedef struct ompi_fifo_t ompi_fifo_t;

/*
 * structure used to track which circular buffer slot to write to
 */
struct cb_slot_t {
    /* pointer to circular buffer fifo structures */
    ompi_cb_fifo_t *cb;

    /* index in circular buffer */
    int index;
};
typedef cb_slot_t cb_slot_t;

/**
 * Initialize a fifo 
 *
 * @param size_of_cb_fifo Length of fifo array (IN)
 *
 * @param fifo_memory_locality_index Locality index to apply to
 *                                   the fifo array.  Not currently
 *                                   in use (IN)
 *
 * @param tail_memory_locality_index Locality index to apply to the
 *                                   head control structure.  Not
 *                                   currently in use (IN)
 *
 * @param tail_memory_locality_index Locality index to apply to the
 *                                   tail control structure.  Not
 *                                   currently in use (IN)
 *
 * @param fifo Pointer to data structure defining this fifo (IN)
 *
 * @param memory_allocator Pointer to the memory allocator to use
 *                         to allocate memory for this fifo. (IN)
 *
 * @returncode Error code
 *
 */
static inline int ompi_fifo_init(int size_of_cb_fifo, int lazy_free_freq,
        int fifo_memory_locality_index, int head_memory_locality_index, 
        int tail_memory_locality_index, ompi_fifo_t *fifo, 
        mca_mpool_base_module_t *memory_allocator) 
{
    int error_code=OMPI_SUCCESS;

    /* allocate head ompi_cb_fifo_t structure */
    len_to_allocate=sizeof(ompi_cb_fifo_wrapper_t);
    fifo->head=memory_allocator->mpool_alloc(len_to_allocate,CACHE_LINE_SIZE);
    if ( NULL == fifo->head) {
        return OMPI_ERR_OUT_OF_RESOURCE;
    }

    /* initialize the circular buffer fifo head structure */
    error_code=ompi_cb_fifo_init(size_of_cb_fifo, lazy_free_freq,
        fifo_memory_locality_index, head_memory_locality_index, 
        tail_memory_locality_index, &(fifo->head.cb_fifo), memory_allocator);
    if ( OMPI_SUCCESS != error_code ) {
        return error_code;
    }

    /* finish head initialization */
    fifo->next_cb_fifo=fifo;  /* only one element in the link list */
    fifo->cb_overflow=false;  /* no attempt to overflow the queue */
    fifo->fifo_allocator=memory_allocator; /* set pointer to memory
                                              allocation functions */

    /* set the tail */
    fifo->tail=fifo->head;

    /* return */
    return errorCode;
}

/**
 * function to cleanup the fifo
 *
 * @param fifo Pointer to data structure defining this fifo (IN)
 *
 * @param memory_allocator Pointer to the memory allocator to use
 *                         to allocate memory for this fifo. (IN)
 *
 */
static inline int ompi_fifo_free( ompi_fifo_t *fifo, 
        mca_mpool_base_module_t *memory_allocator) 
{

    int error_code=OMPI_SUCCESS;
    ompi_cb_fifo_wrapper_t *starting_ff,*ff,*ff_tmp;

    /* loop over the link list of ompi_cb_fifo_wrapper_t structs */
    starting_ff=fifo->head;
    ff=starting_ff;
    do {

        /* free the resources associated with the ompi_cb_fifo_t structure */
        error_code=ompi_cb_fifo_free(&(ff->cb_fifo));
        if ( OMPI_SUCCESS != error_code ) {
            return error_code;
        }

        /* next structure */
        ff_tmp=ff->next_fifo_wrapper;

        /* free the element */
        memory_allocator->mpool_free(ff);

        ff=ff_tmp;

    } while (ff != starting_ff)

    /* return */
    return errorCode;
}


/**
 * Write pointer to the specified slot
 *
 * @param slot Slot addressing (IN)
 *
 * @param data Pointer value to write in specified slot (IN)
 *
 * @returncode Slot index data written to
 *
 */
static inline int ompi_fifo_write_to_slot(cb_slot_t *slot, void* data) 
{
    return ompi_cb_fifo_write_to_slot(slot->index,data,slot->cb);
}

/**
 * Try to write pointer to the head of the queue
 *
 * @param data Pointer value to write in specified slot (IN)
 *
 * @param fifo Pointer to data structure defining this fifo (IN)
 *
 * @returncode Slot index to which data is written
 *
 */
static inline int ompi_fifo_write_to_head(void *data, ompi_fifo_t
        *fifo)
{
    int error_code=OMPI_SUCCESS;
    size_t len_to_allocate;
    ompi_cb_fifo_wrapper_t *next_ff,tmp_ff;
    bool available;

    /* attempt to write data to head ompi_fifo_cb_fifo_t */
    error_code=ompi_cb_fifo_write_to_head(void *data,
            fifo->head->cb_fifo);
    if( OMPI_CB_ERROR == error_code ) {
        /* 
         * queue is full 
         */

        /* mark queue as overflown */
        fifo->head->cb_overflow=true;

        /* see if next queue is available - while the next queue
         * has not been emptied, it will be marked as overflowen*/
        next_ff=fifo->head->next_fifo_wrapper;
        available=next_ff->cb_overflow;

        /* if next queue not available, allocate new queue */
        if( !available ) {

            /* allocate head ompi_cb_fifo_t structure */
            len_to_allocate=sizeof(ompi_cb_fifo_wrapper_t);
            tmp_ff=fifo->head->fifo_allocator->mpool_alloc
                (len_to_allocate,CACHE_LINE_SIZE);
            if ( NULL == tmp_ff) {
                return OMPI_ERR_OUT_OF_RESOURCE;
            }

            /* initialize the circular buffer fifo head structure */
            error_code=ompi_cb_fifo_init(fifo->head->cb_fifo->size, 
                    fifo->head->cb_fifo->lazy_free_frequency,
                    fifo->head->cb_fifo->fifo_memory_locality_index, 
                    fifo->head->cb_fifo->head_memory_locality_index, 
                    fifo->head->cb_fifo->tail_memory_locality_index, 
                    &(tmp_ff->head.cb_fifo), 
                    fifo->head->fifo_allocator);
            if ( OMPI_SUCCESS != error_code ) {
                return error_code;
            }


            /* finish new element initialization */
            tmp_ff->next_cb_fifo=fifo->next_cb_fifo;  /* only one element in 
                                                         the link list */
            fifo->next_cb_fifo=tmp_ff;
            tmp_ff->cb_overflow=false;  /* no attempt to overflow the queue */
            tmp_ff->fifo_allocator=fifo->memory_allocator; /* set pointer to 
                                                              memory allocation
                                                              functions */
        }

        /* reset head pointer */
        fifo->head=tmp_ff;

        /* write data to new head structure */
        error_code=ompi_cb_fifo_write_to_head(void *data,
                fifo->head->cb_fifo);
        if( OMPI_CB_ERROR == error_code ) {
            return error_code;
        }
    }

    /* return */
    return error_code;
}


/**
 * Reserve slot in the fifo array
 *
 * @param fifo Pointer to data structure defining this fifo (IN)
 *
 * @returncode Slot index to which data is written
 *
 * @returncode OMPI_CB_ERROR failed to allocate index
 *
 */
static inline cb_slot_t ompi_fifo_get_slot(ompi_cb_fifo_t *fifo) {

    int error_code=OMPI_SUCCESS;
    size_t len_to_allocate;
    ompi_cb_fifo_wrapper_t *next_ff,tmp_ff;
    bool available;
    cb_slot_t return_params;

    /* attempt to write data to head ompi_fifo_cb_fifo_t */
    return_params.index=ompi_cb_fifo_get_slot(void *data,
            fifo->head->cb_fifo);
    if( OMPI_CB_ERROR == return_params.index ) {
        /* 
         * queue is full 
         */

        /* mark queue as overflown */
        fifo->head->cb_overflow=true;

        /* see if next queue is available - while the next queue
         * has not been emptied, it will be marked as overflowen*/
        next_ff=fifo->head->next_fifo_wrapper;
        available=next_ff->cb_overflow;

        /* if next queue not available, allocate new queue */
        if( !available ) {

            /* allocate head ompi_cb_fifo_t structure */
            len_to_allocate=sizeof(ompi_cb_fifo_wrapper_t);
            tmp_ff=fifo->head->fifo_allocator->mpool_alloc
                (len_to_allocate,CACHE_LINE_SIZE);
            if ( NULL == tmp_ff) {
                return_params.index=OMPI_ERR_OUT_OF_RESOURCE;
                return return_params;
            }

            /* initialize the circular buffer fifo head structure */
            return_params.index=ompi_cb_fifo_init(fifo->head->cb_fifo->size, 
                    fifo->head->cb_fifo->lazy_free_frequency,
                    fifo->head->cb_fifo->fifo_memory_locality_index, 
                    fifo->head->cb_fifo->head_memory_locality_index, 
                    fifo->head->cb_fifo->tail_memory_locality_index, 
                    &(tmp_ff->head.cb_fifo), 
                    fifo->head->fifo_allocator);
            if ( OMPI_SUCCESS != return_params.index ) {
                return return_params;
            }


            /* finish new element initialization */
            tmp_ff->next_cb_fifo=fifo->next_cb_fifo;  /* only one element in 
                                                         the link list */
            fifo->next_cb_fifo=tmp_ff;
            tmp_ff->cb_overflow=false;  /* no attempt to overflow the queue */
            tmp_ff->fifo_allocator=fifo->memory_allocator; /* set pointer to 
                                                              memory allocation
                                                              functions */
        }

        /* reset head pointer */
        fifo->head=tmp_ff;

        /* write data to new head structure */
        return_params.index=ompi_cb_fifo_get_slot(void *data,
                fifo->head->cb_fifo);
        if( OMPI_CB_ERROR == return_params.index ) {
            return return_params;
        }
    }

    /* return */
    return_params.cb=fifo->head->cb_fifo;
    return return_params;
}

/**
 * Try to read pointer from the tail of the queue
 *
 * @param data Pointer to where data was be written (out)
 *
 * @param fifo Pointer to data structure defining this fifo (IN)
 *
 * @returncode Slot index to which data is written
 *
 */
static inline void *ompi_fifo_read_from_tail(ompi_fifo_t *fifo) 
{
    /* local parameters */
    void *return_value;
    bool queue_empty;

    /* get next element */
    return_value=ompi_cb_fifo_read_from_tail(fifo->tail->cb_fifo,
            fifo->tail->cb_overflow,&queue_empty);
    /* check to see if need to move on to next cb_fifo in the link list */
    if( (void *)OMPI_CB_ERROR & queue_empty ) {
        /* queue_emptied - move on to next element in fifo */
        fifo->tail->cb_overflow=false;
        fifo->tail=fifo->tail->next_fifo_wrapper;
    }

    /* return */
    return return_value;
}

#endif				/* !_OMPI_FIFO */

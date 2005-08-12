/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University.
 *                         All rights reserved.
 * Copyright (c) 2004-2005 The Trustees of the University of Tennessee.
 *                         All rights reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

#include "ompi_config.h"
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <string.h>
#include <fcntl.h>
#include <time.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <sys/stat.h>
#include <sys/errno.h>
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#include "ompi/include/constants.h"
#include "common_sm_mmap.h"
#include "opal/util/output.h"
#include "util/sys_info.h"
#include "util/proc_info.h"


OBJ_CLASS_INSTANCE(
    mca_common_sm_mmap_t,
    opal_object_t,
    NULL,
    NULL
);

/*
 * Instance that is shared between components that use shared memory
 */
mca_common_sm_mmap_t *mca_common_sm_mmap = NULL;


static int mca_common_sm_mmap_open(char* path)
{
    int fd = -1;
    struct timespec ts;

    /* loop until file can be opened, or until an erro, other than
     * access error, occurs */
    while (fd < 0) {
        fd = open(path, O_CREAT|O_RDWR, 0000); 
        if (fd < 0 && errno != EACCES) {
            opal_output(0, 
                        "mca_ptl_sm_mmap_open: open %s failed with errno=%d\n",
                        path, errno);
            return -1;
        }
        ts.tv_sec = 0; 
        ts.tv_nsec = 500000;
        nanosleep(&ts, NULL);
    }

    return fd;

}


mca_common_sm_mmap_t* mca_common_sm_mmap_init(size_t size, char *file_name, 
        size_t size_ctl_structure, size_t data_seg_alignment)
{
    int fd,return_code=OMPI_SUCCESS;
    bool file_previously_opened;
    mca_common_sm_file_header_t* seg;
    mca_common_sm_mmap_t* map;
    struct stat s_stat;
    unsigned char *addr;
    size_t tmp,mem_offset;

    /* input parameter error checks */
    if( (size < sizeof(mca_common_sm_file_header_t) ) ||
                ( file_name == NULL ) || 
                ( size_ctl_structure <
                  sizeof(mca_common_sm_file_header_t ) )) {
        return NULL;
    }

    /* open the backing file.  The first process to succeed here will
       effectively block the others until most of the rest of the
       setup in this function is complete because the initial perms
       are 000 (an fchmod() is executed below, enabling the other
       processes to get in) */
    fd=mca_common_sm_mmap_open(file_name);
    if( -1 == fd ) {
        opal_output(0, "mca_common_sm_mmap_init: mca_common_sm_mmap_open failed \n");
        return NULL;
    }

    /* figure out if I am first to attach to file */
    file_previously_opened=false;
    return_code=fstat(fd,&s_stat);
    if( 0 > return_code ) {
        opal_output(0, "mca_common_sm_mmap_init: fstat failed with errno=%d\n", errno);
        close(fd);
        return NULL;
    }
    if (s_stat.st_size > 0){
        file_previously_opened=true;
    }

    /* first process to open the file, so needs to initialize it */
    if( !file_previously_opened ) {
        /* truncate the file to the requested size */
        if(ftruncate(fd, size) != 0) {
            opal_output(0, 
                    "mca_common_sm_mmap_init: ftruncate failed with errno=%d\n",
                    errno);
            close(fd);
            return NULL;
        }
    }

    /* map the file and initialize segment state */
    seg = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if( (void*)-1 == seg ) {
        opal_output(0, "mca_common_sm_mmap_init: mmap failed with errno=%d\n",
                errno);
        close(fd);
        return NULL;
    }

    /* set up the map object */
    map = OBJ_NEW(mca_common_sm_mmap_t);
    strncpy(map->map_path, file_name, OMPI_PATH_MAX);
    /* the first entry in the file is the control strcuture.  the first
       entry in the control structure is an mca_common_sm_file_header_t
       element */
    map->map_seg = seg;

    /* If we have a data segment (i.e., if 0 != data_seg_alignment),
       then make it the first aligned address after the control
       structure. */
    if (0 != data_seg_alignment) {
        addr = ((unsigned char *) seg) + size_ctl_structure;
        /* calculate how far off alignment we are */
        tmp = ((size_t) addr) % data_seg_alignment;
        /* if we're off alignment, then move up to the next alignment */
        if (tmp > 0) {
            addr += (data_seg_alignment - tmp);
        }

        /* is addr past end of file ? */
        if( (unsigned char*)seg+size < addr ){
            opal_output(0, "mca_common_sm_mmap_init: memory region too small len %d  addr %p\n",
                        size,addr);
            fchmod(fd, 0600);
            close(fd);
            munmap(seg,size);
            return NULL;
        }
        map->data_addr = addr;
    } else {
        map->data_addr = NULL;
    }
    mem_offset=addr-(unsigned char *)seg;
    map->map_addr = (unsigned char *)seg;
    map->map_size = size;

    /* initialize the segment - only the first process to open the file */
    if( !file_previously_opened ) {
        opal_atomic_unlock(&seg->seg_lock);
        seg->seg_inited = false;
        seg->seg_offset = mem_offset;
        seg->seg_size = size;
    }

    /* enable access by other processes on this host */
    if(fchmod(fd, 0600) != 0) {
        opal_output(0, "mca_common_sm_mmap_init: fchmod failed with errno=%d :: fd %d\n",
                errno,fd);
        OBJ_RELEASE(map);
        close(fd);
        return NULL;
    }
    close(fd);

    return map;
}


/**
 *  allocate memory from a previously allocated shared memory
 *  block.
 *
 *  @param size size of request, in bytes (IN)
 * 
 *  @retval addr virtual address
 */

void* mca_common_sm_mmap_seg_alloc(
    struct mca_mpool_base_module_t* mpool,
    size_t* size,
    mca_mpool_base_registration_t** registration)
{
    mca_common_sm_mmap_t* map = mca_common_sm_mmap;
    mca_common_sm_file_header_t* seg = map->map_seg;
    void* addr;

    opal_atomic_lock(&seg->seg_lock);
    if(seg->seg_offset + *size > map->map_size) {
        addr = NULL;
    } else {
        /* add base address to segment offset */
        addr = map->data_addr + seg->seg_offset;
        seg->seg_offset += *size;
    }
    if (NULL != registration) {
        *registration = NULL;
    }
    opal_atomic_unlock(&seg->seg_lock);
    return addr;
}


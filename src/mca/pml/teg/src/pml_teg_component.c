/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University.
 *                         All rights reserved.
 * Copyright (c) 2004-2005 The Trustees of the University of Tennessee.
 *                         All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

#include "ompi_config.h"

#include "event/event.h"
#include "mpi.h"
#include "mca/pml/pml.h"
#include "mca/ptl/ptl.h"
#include "mca/base/mca_base_param.h"
#include "mca/pml/base/pml_base_bsend.h"
#include "pml_teg.h"
#include "pml_teg_proc.h"
#include "pml_teg_sendreq.h"
#include "pml_teg_recvreq.h"


mca_pml_base_component_1_0_0_t mca_pml_teg_component = {

    /* First, the mca_base_component_t struct containing meta
       information about the component itself */

    {
      /* Indicate that we are a pml v1.0.0 component (which also implies
         a specific MCA version) */

      MCA_PML_BASE_VERSION_1_0_0,
    
      "teg", /* MCA component name */
      1,  /* MCA component major version */
      0,  /* MCA component minor version */
      0,  /* MCA component release version */
      mca_pml_teg_component_open,  /* component open */
      mca_pml_teg_component_close  /* component close */
    },

    /* Next the MCA v1.0.0 component meta data */

    {
      /* Whether the component is checkpointable or not */
      false
    },

    mca_pml_teg_component_init,  /* component init */
    mca_pml_teg_component_fini   /* component finalize */
};



static inline int mca_pml_teg_param_register_int(
    const char* param_name,
    int default_value)
{
    int id = mca_base_param_register_int("pml","teg",param_name,NULL,default_value);
    int param_value = default_value;
    mca_base_param_lookup_int(id,&param_value);
    return param_value;
}
                                                                                                                        

int mca_pml_teg_component_open(void)
{
#ifdef WIN32
     WSADATA win_sock_data;
     if (WSAStartup(MAKEWORD(2,2), &win_sock_data) != 0) {
         ompi_output (0, "mca_oob_tcp_component_init: failed to initialise windows sockets: %d\n", WSAGetLastError());
         return OMPI_ERROR;
      }
#endif
    OBJ_CONSTRUCT(&mca_pml_teg.teg_lock, ompi_mutex_t);
    OBJ_CONSTRUCT(&mca_pml_teg.teg_send_requests, ompi_free_list_t);
    OBJ_CONSTRUCT(&mca_pml_teg.teg_recv_requests, ompi_free_list_t);
    OBJ_CONSTRUCT(&mca_pml_teg.teg_procs, ompi_list_t);
    OBJ_CONSTRUCT(&mca_pml_teg.teg_send_pending, ompi_list_t);

    mca_pml_teg.teg_free_list_num =
        mca_pml_teg_param_register_int("free_list_num", 256);
    mca_pml_teg.teg_free_list_max =
        mca_pml_teg_param_register_int("free_list_max", -1);
    mca_pml_teg.teg_free_list_inc =
        mca_pml_teg_param_register_int("free_list_inc", 256);
    mca_pml_teg.teg_poll_iterations =
        mca_pml_teg_param_register_int("poll_iterations", 100000);
    return OMPI_SUCCESS;
}


int mca_pml_teg_component_close(void)
{
#ifdef WIN32
    WSACleanup();
#endif

#if OMPI_ENABLE_DEBUG
    if (mca_pml_teg.teg_recv_requests.fl_num_allocated !=
        mca_pml_teg.teg_recv_requests.super.ompi_list_length) {
        ompi_output(0, "teg recv requests: %d allocated %d returned\n",
            mca_pml_teg.teg_recv_requests.fl_num_allocated,
            mca_pml_teg.teg_recv_requests.super.ompi_list_length);
    }
#endif

    if(NULL != mca_pml_teg.teg_ptl_components) {
        free(mca_pml_teg.teg_ptl_components);
    }
    OBJ_DESTRUCT(&mca_pml_teg.teg_send_pending);
    OBJ_DESTRUCT(&mca_pml_teg.teg_send_requests);
    OBJ_DESTRUCT(&mca_pml_teg.teg_recv_requests);
    OBJ_DESTRUCT(&mca_pml_teg.teg_procs);
    OBJ_DESTRUCT(&mca_pml_teg.teg_lock);
    return OMPI_SUCCESS;
}


mca_pml_base_module_t* mca_pml_teg_component_init(int* priority, 
                                                  bool *allow_multi_user_threads,
                                                  bool *have_hidden_threads)
{
    uint32_t proc_arch;
    int rc;
    *priority = 0;
    *have_hidden_threads = false;

    mca_pml_teg.teg_ptl_components = NULL;
    mca_pml_teg.teg_num_ptl_components = 0;
    mca_pml_teg.teg_ptl_components = NULL;
    mca_pml_teg.teg_num_ptl_components = 0;

    /* recv requests */
    ompi_free_list_init(
        &mca_pml_teg.teg_recv_requests,
        sizeof(mca_pml_teg_recv_request_t),
        OBJ_CLASS(mca_pml_teg_recv_request_t), 
        mca_pml_teg.teg_free_list_num,
        mca_pml_teg.teg_free_list_max,
        mca_pml_teg.teg_free_list_inc,
        NULL);

    /* buffered send */
    if(mca_pml_base_bsend_init(allow_multi_user_threads) != OMPI_SUCCESS) {
        ompi_output(0, "mca_pml_teg_component_init: mca_pml_bsend_init failed\n");
        return NULL;
    }

    /* post this processes datatype */
    proc_arch = ompi_proc_local()->proc_arch;
    proc_arch = htonl(proc_arch);
    rc = mca_base_modex_send(&mca_pml_teg_component.pmlm_version, &proc_arch, sizeof(proc_arch));
    if(rc != OMPI_SUCCESS)
        return NULL;
    
    *allow_multi_user_threads &= true;
    return &mca_pml_teg.super;
}


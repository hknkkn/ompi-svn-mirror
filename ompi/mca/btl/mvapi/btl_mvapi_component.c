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
#include <unistd.h>
#include "ompi/include/constants.h"
#include "opal/event/event.h"
#include "opal/util/if.h"
#include "opal/util/argv.h"
#include "opal/util/output.h"
#include "mca/pml/pml.h"
#include "mca/btl/btl.h"

#include "mca/base/mca_base_param.h"
#include "mca/errmgr/errmgr.h"
#include "mca/mpool/base/base.h" 
#include "btl_mvapi.h"
#include "btl_mvapi_frag.h"
#include "btl_mvapi_endpoint.h" 
#include "mca/btl/base/base.h" 
#include <vapi.h> 
#include <vapi_common.h> 
#include "datatype/convertor.h" 
#include "mca/mpool/mvapi/mpool_mvapi.h" 
#include "btl_mvapi_endpoint.h"
#include "mca/pml/base/pml_base_module_exchange.h"
#include <malloc.h>

mca_btl_mvapi_component_t mca_btl_mvapi_component = {
    {
        /* First, the mca_base_component_t struct containing meta information
           about the component itself */

        {
            /* Indicate that we are a pml v1.0.0 component (which also implies a
               specific MCA version) */

            MCA_BTL_BASE_VERSION_1_0_0,

            "mvapi", /* MCA component name */
            OMPI_MAJOR_VERSION,  /* MCA component major version */
            OMPI_MINOR_VERSION,  /* MCA component minor version */
            OMPI_RELEASE_VERSION,  /* MCA component release version */
            mca_btl_mvapi_component_open,  /* component open */
            mca_btl_mvapi_component_close  /* component close */
        },

        /* Next the MCA v1.0.0 component meta data */

        {
            /* Whether the component is checkpointable or not */

            false
        },

        mca_btl_mvapi_component_init,  
        mca_btl_mvapi_component_progress,
    }
};


/*
 * utility routines for parameter registration
 */

static inline char* mca_btl_mvapi_param_register_string(
                                                     const char* param_name, 
                                                     const char* default_value)
{
    char *param_value;
    int id = mca_base_param_register_string("btl","mvapi",param_name,NULL,default_value);
    mca_base_param_lookup_string(id, &param_value);
    return param_value;
}

static inline int mca_btl_mvapi_param_register_int(
        const char* param_name, 
        int default_value)
{
    int id = mca_base_param_register_int("btl","mvapi",param_name,NULL,default_value);
    int param_value = default_value;
    mca_base_param_lookup_int(id,&param_value);
    return param_value;
}

/*
 *  Called by MCA framework to open the component, registers
 *  component parameters.
 */

int mca_btl_mvapi_component_open(void)
{

    int param, value; 
    
    /* initialize state */
    mca_btl_mvapi_component.ib_num_btls=0;
    mca_btl_mvapi_component.mvapi_btls=NULL;
    
    /* initialize objects */ 
    OBJ_CONSTRUCT(&mca_btl_mvapi_component.ib_procs, opal_list_t);

    /* register IB component parameters */
    mca_btl_mvapi_component.ib_free_list_num =
        mca_btl_mvapi_param_register_int ("free_list_num", 8);
    mca_btl_mvapi_component.ib_free_list_max =
        mca_btl_mvapi_param_register_int ("free_list_max", -1);
    mca_btl_mvapi_component.ib_free_list_inc =
        mca_btl_mvapi_param_register_int ("free_list_inc", 32);
    mca_btl_mvapi_component.ib_mpool_name = 
        mca_btl_mvapi_param_register_string("mpool", "mvapi"); 
    mca_btl_mvapi_component.ib_rr_buf_max = 
        mca_btl_mvapi_param_register_int("rr_buf_max", 16); 
    mca_btl_mvapi_component.ib_rr_buf_min = 
        mca_btl_mvapi_param_register_int("rr_buf_min", 8); 
    mca_btl_mvapi_component.reg_mru_len = 
        mca_btl_mvapi_param_register_int("reg_mru_len",  16); 
    mca_btl_mvapi_component.use_srq = 
        mca_btl_mvapi_param_register_int("use_srq", 0); 

    mca_btl_mvapi_component.ib_cq_size = 
        mca_btl_mvapi_param_register_int("ib_cq_size", 
                                      10000); 
    mca_btl_mvapi_component.ib_wq_size = 
        mca_btl_mvapi_param_register_int("ib_wq_size", 
                                      10000); 
    mca_btl_mvapi_component.ib_sg_list_size = 
        mca_btl_mvapi_param_register_int("ib_sg_list_size", 
                                      16); 
    mca_btl_mvapi_component.ib_pkey_ix = 
        mca_btl_mvapi_param_register_int("ib_pkey_ix", 
                                      0); 
    mca_btl_mvapi_component.ib_psn = 
        mca_btl_mvapi_param_register_int("ib_psn", 
                                      0); 
    mca_btl_mvapi_component.ib_qp_ous_rd_atom = 
        mca_btl_mvapi_param_register_int("ib_qp_ous_rd_atom", 
                                         4); 
    mca_btl_mvapi_component.ib_mtu = 
        mca_btl_mvapi_param_register_int("ib_mtu", 
                                      MTU1024); 
    mca_btl_mvapi_component.ib_min_rnr_timer = 
        mca_btl_mvapi_param_register_int("ib_min_rnr_timer", 
                                         24);
    mca_btl_mvapi_component.ib_timeout = 
        mca_btl_mvapi_param_register_int("ib_timeout", 
                                         10); 
    mca_btl_mvapi_component.ib_retry_count = 
        mca_btl_mvapi_param_register_int("ib_retry_count", 
                                      7); 
    mca_btl_mvapi_component.ib_rnr_retry = 
        mca_btl_mvapi_param_register_int("ib_rnr_retry", 
                                      7); 
    mca_btl_mvapi_component.ib_max_rdma_dst_ops = 
        mca_btl_mvapi_param_register_int("ib_max_rdma_dst_ops", 
                                      16); 

    mca_btl_mvapi_component.ib_service_level = 
        mca_btl_mvapi_param_register_int("ib_service_level", 
                                      0); 
    mca_btl_mvapi_component.ib_static_rate = 
        mca_btl_mvapi_param_register_int("ib_static_rate", 
                                      0); 
    mca_btl_mvapi_component.ib_src_path_bits = 
        mca_btl_mvapi_param_register_int("ib_src_path_bits", 
                                      0); 
    
    mca_btl_mvapi_component.rd_per_peer = 
        mca_btl_mvapi_param_register_int("rd_per_peer", 
                                         16); 
    

    mca_btl_mvapi_module.super.btl_exclusivity =
                                         mca_btl_mvapi_param_register_int ("exclusivity", MCA_BTL_EXCLUSIVITY_DEFAULT);
    mca_btl_mvapi_module.super.btl_eager_limit = 
        mca_btl_mvapi_param_register_int ("eager_limit", (32*1024)) 
        - sizeof(mca_btl_mvapi_header_t); 

    mca_btl_mvapi_module.super.btl_min_send_size =
        mca_btl_mvapi_param_register_int ("min_send_size", (32*1024))
        - sizeof(mca_btl_mvapi_header_t);

    mca_btl_mvapi_module.super.btl_max_send_size =
        mca_btl_mvapi_param_register_int ("max_send_size", (128*1024)) 
        - sizeof(mca_btl_mvapi_header_t);

    mca_btl_mvapi_module.super.btl_min_rdma_size = 
        mca_btl_mvapi_param_register_int("min_rdma_size", 
                                      1024*1024); 
    mca_btl_mvapi_module.super.btl_max_rdma_size = 
        mca_btl_mvapi_param_register_int("max_rdma_size", 
                                      1024*1024); 
    mca_btl_mvapi_module.super.btl_flags  = 
        mca_btl_mvapi_param_register_int("flags", 
                                      MCA_BTL_FLAGS_PUT|MCA_BTL_FLAGS_GET); 
    mca_btl_mvapi_module.super.btl_bandwidth = 
        mca_btl_mvapi_param_register_int("bandwidth",  800);
    
    param = mca_base_param_find("mpi", NULL, "leave_pinned"); 
    mca_base_param_lookup_int(param, &value); 
    mca_btl_mvapi_component.leave_pinned = value; 
    mca_base_param_reg_int(&mca_btl_mvapi_component.super.btl_version, 
                           "max_wr_sq_tokens", 
                           "Maximum number of send/rdma work request tokens", 
                           false, 
                           false, 
                           16, 
                           &(mca_btl_mvapi_component.max_wr_sq_tokens)); 
    
   mca_base_param_reg_int(&mca_btl_mvapi_component.super.btl_version, 
                          "max_total_wr_sq_tokens", 
                          "Maximum number of send/rdma work request tokens peer btl", 
                          false, 
                          false, 
                          32, 
                          &(mca_btl_mvapi_component.max_total_wr_sq_tokens)); 
    
    mca_btl_mvapi_component.max_send_size = mca_btl_mvapi_module.super.btl_max_send_size; 
    mca_btl_mvapi_component.eager_limit = mca_btl_mvapi_module.super.btl_eager_limit; 
    
    return OMPI_SUCCESS;
}

/*
 * component cleanup - sanity checking of queue lengths
 */

int mca_btl_mvapi_component_close(void)
{
    return OMPI_SUCCESS;
}



/*
 *  Register MVAPI port information. The MCA framework
 *  will make this available to all peers.
 */

static int
mca_btl_mvapi_modex_send(void)
{
    int         rc;
    size_t      i;
    size_t      size;
    mca_btl_mvapi_port_info_t *ports = NULL;

    size = mca_btl_mvapi_component.ib_num_btls * sizeof (mca_btl_mvapi_port_info_t);
    if (size != 0) {
        ports = (mca_btl_mvapi_port_info_t *)malloc (size);
        if (NULL == ports) {
            return OMPI_ERR_OUT_OF_RESOURCE;
        }

        for (i = 0; i < mca_btl_mvapi_component.ib_num_btls; i++) {
            mca_btl_mvapi_module_t *btl = &mca_btl_mvapi_component.mvapi_btls[i];
            ports[i] = btl->port_info;
        }
    }
    rc = mca_pml_base_modex_send (&mca_btl_mvapi_component.super.btl_version, ports, size);
    if (NULL != ports) {
        free (ports);
    }
    return rc;
}


/*
 *  IB component initialization:
 *  (1) read interface list from kernel and compare against component parameters
 *      then create a BTL instance for selected interfaces
 *  (2) setup IB listen socket for incoming connection attempts
 *  (3) register BTL parameters with the MCA
 */

mca_btl_base_module_t** mca_btl_mvapi_component_init(int *num_btl_modules, 
                                                  bool enable_progress_threads,
                                                  bool enable_mpi_threads)
{
    VAPI_ret_t vapi_ret;
    VAPI_hca_id_t* hca_ids;
    VAPI_hca_hndl_t hca_hndl; 
    VAPI_hca_vendor_t hca_vendor; 
    VAPI_hca_cap_t hca_cap; 
    VAPI_hca_port_t hca_port; 
    uint32_t num_hcas; 
    mca_btl_base_module_t** btls;
    uint32_t i,j, length;
    struct mca_mpool_base_resources_t hca_pd; 
    opal_list_t btl_list; 
    mca_btl_mvapi_module_t * mvapi_btl; 
    mca_btl_base_selected_module_t* ib_selected; 
    opal_list_item_t* item; 

    /* ugly HACK!! */ 
    /* mallopt(M_TRIM_THRESHOLD, -1); */
/*     mallopt(M_MMAP_MAX, 0); */
    /* initialization */
    *num_btl_modules = 0;

    /* Determine the number of hca's available on the host */
    vapi_ret=EVAPI_list_hcas(0, &num_hcas, NULL);
    if( VAPI_EAGAIN != vapi_ret || 0 == num_hcas ) {
        mca_btl_base_error_no_nics("MVAPI", "HCA");
        mca_btl_mvapi_component.ib_num_btls = 0;
        mca_btl_mvapi_modex_send();
        return NULL;
    }

    /* Allocate space for the hca's */ 
    hca_ids = (VAPI_hca_id_t*) malloc(num_hcas * sizeof(VAPI_hca_id_t));
    if(NULL == hca_ids) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return NULL;
    }

    /* obtain a list of the hca's on this host */ 
    vapi_ret=EVAPI_list_hcas(num_hcas, &num_hcas, hca_ids);
    if( VAPI_OK != vapi_ret ) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return NULL;
    }
        
    /** We must loop through all the hca id's, get there handles and 
        for each hca we query the number of ports on the hca and set up 
        a distinct btl module for each hca port */ 

    OBJ_CONSTRUCT(&btl_list, opal_list_t); 
    OBJ_CONSTRUCT(&mca_btl_mvapi_component.ib_lock, opal_mutex_t);


    for(i = 0; i < num_hcas; i++){  
        vapi_ret = EVAPI_get_hca_hndl(hca_ids[i], &hca_hndl); 
        if(VAPI_OK != vapi_ret) { 
            BTL_ERROR(("error getting hca handle: %s", VAPI_strerror(vapi_ret))); 
            return NULL; 
        } 
        

        vapi_ret = VAPI_query_hca_cap(hca_hndl, &hca_vendor, &hca_cap); 
         if(VAPI_OK != vapi_ret) { 
            BTL_ERROR(("error getting hca properties %s", VAPI_strerror(vapi_ret))); 
            return NULL; 
        } 
         
         
         /* Note ports are 1 based hence j = 1 */
         for(j = 1; j <= hca_cap.phys_port_num; j++){ 
             vapi_ret = VAPI_query_hca_port_prop(hca_hndl, (IB_port_t) j, &hca_port);  
             if(VAPI_OK != vapi_ret) { 
                 BTL_ERROR(("error getting hca port properties %s", VAPI_strerror(vapi_ret))); 
                 return NULL; 
             } 
             
             if( PORT_ACTIVE == hca_port.state ){ 
                 
                 mvapi_btl = (mca_btl_mvapi_module_t*) malloc(sizeof(mca_btl_mvapi_module_t)); 
                 memcpy(mvapi_btl, &mca_btl_mvapi_module, sizeof(mca_btl_mvapi_module));
                 
                 ib_selected = OBJ_NEW(mca_btl_base_selected_module_t); 
                 ib_selected->btl_module = (mca_btl_base_module_t*) mvapi_btl; 
                 memcpy(mvapi_btl->hca_id,   hca_ids[i], sizeof(VAPI_hca_id_t)); 
                 mvapi_btl->nic = hca_hndl; 
                 mvapi_btl->port_id = (IB_port_t) j; 
                 mvapi_btl->port = hca_port; 
                 mvapi_btl->port_info.subnet = hca_port.sm_lid;
                 
                 opal_list_append(&btl_list, (opal_list_item_t*) ib_selected);
                 mca_btl_mvapi_component.ib_num_btls ++; 
            } 
        }
 
    }
    
    if(0 == mca_btl_mvapi_component.ib_num_btls){ 
        mca_btl_base_error_no_nics("MVAPI", "HCA");
        mca_btl_mvapi_modex_send();
        return NULL; 
    }
    /* Allocate space for btl modules */
    mca_btl_mvapi_component.mvapi_btls = (mca_btl_mvapi_module_t*) malloc(sizeof(mca_btl_mvapi_module_t) * 
                                                                          mca_btl_mvapi_component.ib_num_btls);
    
    if(NULL == mca_btl_mvapi_component.mvapi_btls) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return NULL;
    }
    btls = (struct mca_btl_base_module_t**) 
        malloc(mca_btl_mvapi_component.ib_num_btls * sizeof(struct mca_btl_mvapi_module_t*));
    if(NULL == btls) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return NULL;
    }

    
    
    for(i = 0; i < mca_btl_mvapi_component.ib_num_btls; i++){
                 
        
        item = opal_list_remove_first(&btl_list); 
        ib_selected = (mca_btl_base_selected_module_t*)item; 
        mvapi_btl = (mca_btl_mvapi_module_t*) ib_selected->btl_module; 
        memcpy(&(mca_btl_mvapi_component.mvapi_btls[i]), mvapi_btl , sizeof(mca_btl_mvapi_module_t)); 
        free(ib_selected); 
        free(mvapi_btl); 

        mvapi_btl = &mca_btl_mvapi_component.mvapi_btls[i];
        mvapi_btl->rd_buf_max = mca_btl_mvapi_component.ib_rr_buf_max; 
        mvapi_btl->rd_buf_min = mca_btl_mvapi_component.ib_rr_buf_min;
        mvapi_btl->num_peers = 0; 
        mvapi_btl->wr_sq_tokens_hp = 
            mvapi_btl->wr_sq_tokens_lp = mca_btl_mvapi_component.max_total_wr_sq_tokens;

        /* Initialize module state */

        OBJ_CONSTRUCT(&mvapi_btl->pending_frags_hp, opal_list_t);
        OBJ_CONSTRUCT(&mvapi_btl->pending_frags_lp, opal_list_t);
    

        OBJ_CONSTRUCT(&mvapi_btl->ib_lock, opal_mutex_t); 
        OBJ_CONSTRUCT(&mvapi_btl->send_free_eager, ompi_free_list_t);
        OBJ_CONSTRUCT(&mvapi_btl->send_free_max, ompi_free_list_t);
        OBJ_CONSTRUCT(&mvapi_btl->send_free_frag, ompi_free_list_t);
        
        OBJ_CONSTRUCT(&mvapi_btl->recv_free_eager, ompi_free_list_t);
        OBJ_CONSTRUCT(&mvapi_btl->recv_free_max, ompi_free_list_t);
        

        OBJ_CONSTRUCT(&mvapi_btl->repost, opal_list_t);
        OBJ_CONSTRUCT(&mvapi_btl->reg_mru_list, opal_list_t); 
        

        if(mca_btl_mvapi_module_init(mvapi_btl) != OMPI_SUCCESS) {
            free(hca_ids);
            return NULL;
        }
                         
        hca_pd.hca = mvapi_btl->nic; 
        hca_pd.pd_tag = mvapi_btl->ptag; 
        
        /* initialize the memory pool using the hca */ 
        mvapi_btl->super.btl_mpool = 
            mca_mpool_base_module_create(mca_btl_mvapi_component.ib_mpool_name,
                                         &mvapi_btl->super, 
                                         &hca_pd); 
        
        if(NULL == mvapi_btl->super.btl_mpool) { 
            BTL_ERROR(("error creating vapi memory pool! aborting mvapi btl initialization")); 
            return NULL; 
        }
        /* Initialize pool of send fragments */ 
        
        length = sizeof(mca_btl_mvapi_frag_t) + 
            sizeof(mca_btl_mvapi_header_t) + 
            mvapi_btl->super.btl_eager_limit+ 
            2*MCA_BTL_IB_FRAG_ALIGN; 
        
        ompi_free_list_init(&mvapi_btl->send_free_eager,
                            length, 
                            OBJ_CLASS(mca_btl_mvapi_send_frag_eager_t),
                            2*mvapi_btl->rd_buf_max,
                            mca_btl_mvapi_component.ib_free_list_max,
                            mca_btl_mvapi_component.ib_free_list_inc,
                            mvapi_btl->super.btl_mpool);
        
        ompi_free_list_init(&mvapi_btl->recv_free_eager,
                            length, 
                            OBJ_CLASS(mca_btl_mvapi_recv_frag_eager_t),
                            2*mvapi_btl->rd_buf_max,
                            mca_btl_mvapi_component.ib_free_list_max,
                            mca_btl_mvapi_component.ib_free_list_inc,
                            mvapi_btl->super.btl_mpool);
        
        
        
        length = sizeof(mca_btl_mvapi_frag_t) + 
            sizeof(mca_btl_mvapi_header_t) + 
            mvapi_btl->super.btl_max_send_size+
            2*MCA_BTL_IB_FRAG_ALIGN; 
        
        
        ompi_free_list_init(&mvapi_btl->send_free_max,
                            length, 
                            OBJ_CLASS(mca_btl_mvapi_send_frag_max_t),
                            2*mvapi_btl->rd_buf_max,
                            mca_btl_mvapi_component.ib_free_list_max,
                            mca_btl_mvapi_component.ib_free_list_inc,
                            mvapi_btl->super.btl_mpool);
        
        
        
        /* Initialize pool of receive fragments */
        ompi_free_list_init (&mvapi_btl->recv_free_max, 
                             length, 
                             OBJ_CLASS (mca_btl_mvapi_recv_frag_max_t),
                             2*mvapi_btl->rd_buf_max,
                             mca_btl_mvapi_component.ib_free_list_max,
                             mca_btl_mvapi_component.ib_free_list_inc, 
                             mvapi_btl->super.btl_mpool);

        
        
        length = sizeof(mca_btl_mvapi_frag_t) + 
            sizeof(mca_btl_mvapi_header_t)+ 
            2*MCA_BTL_IB_FRAG_ALIGN; 

        
        
        
        ompi_free_list_init(&mvapi_btl->send_free_frag,
                            length, 
                            OBJ_CLASS(mca_btl_mvapi_send_frag_frag_t),
                            mca_btl_mvapi_component.ib_free_list_num,
                            mca_btl_mvapi_component.ib_free_list_max,
                            mca_btl_mvapi_component.ib_free_list_inc,
                            mvapi_btl->super.btl_mpool);
        
        
        /* Initialize the rr_desc_post array for posting of rr*/ 
        mvapi_btl->rr_desc_post = (VAPI_rr_desc_t*) malloc((mvapi_btl->rd_buf_max * sizeof(VAPI_rr_desc_t))); 
        btls[i] = &mvapi_btl->super;
    
    }

    /* Post OOB receive to support dynamic connection setup */
    mca_btl_mvapi_post_recv();
    
    mca_btl_mvapi_modex_send(); 
    *num_btl_modules = mca_btl_mvapi_component.ib_num_btls;
    free(hca_ids);
    return btls;
}

/*
 *  IB component progress.
 */


int mca_btl_mvapi_component_progress()
{
    uint32_t i;
    int count = 0;
    mca_btl_mvapi_frag_t* frag;
    mca_btl_mvapi_endpoint_t* endpoint; 
    /* Poll for completions */
    for(i = 0; i < mca_btl_mvapi_component.ib_num_btls; i++) {
        VAPI_ret_t ret; 
        VAPI_wc_desc_t comp; 
        mca_btl_mvapi_module_t* mvapi_btl = &mca_btl_mvapi_component.mvapi_btls[i];
        
        
        /* we have two completion queues, one for "high" priority and one for "low". 
         *   we will check the high priority and process them until there are none left. 
         *   note that low priority messages are only processed one per progress call. 
         */ 
        ret = VAPI_poll_cq(mvapi_btl->nic, mvapi_btl->cq_hndl_high, &comp); 
        if(VAPI_OK == ret) { 
            if(comp.status != VAPI_SUCCESS) { 
                BTL_ERROR(("Got error : %s, Vendor code : %d Frag : %p", 
                          VAPI_wc_status_sym(comp.status), 
                          comp.vendor_err_syndrome, comp.id));  
                return OMPI_ERROR; 
            }
            
            /* Handle work completions */
            switch(comp.opcode) {
            case VAPI_CQE_RQ_RDMA_WITH_IMM: 
                BTL_ERROR(("Got an RDMA with Immediate data!, not supported!")); 
                return OMPI_ERROR; 
           
            case VAPI_CQE_SQ_SEND_DATA :
            case VAPI_CQE_SQ_RDMA_READ:
            case VAPI_CQE_SQ_RDMA_WRITE:
                frag = (mca_btl_mvapi_frag_t*) (unsigned long) comp.id; 
                /* Process a completed send or an rdma write  */
                frag->rc = OMPI_SUCCESS; 
                frag->base.des_cbfunc(&mvapi_btl->super, frag->endpoint, &frag->base, frag->rc); 
                count++;
                /* check and see if we need to progress pending sends */ 
                if( !mca_btl_mvapi_component.use_srq && 
                    OPAL_THREAD_ADD32(&frag->endpoint->wr_sq_tokens_hp, 1) > 0
                    && !opal_list_is_empty(&(frag->endpoint->pending_frags_hp))) { 
                    opal_list_item_t *frag_item;
                    OPAL_THREAD_LOCK(&frag->endpoint->endpoint_lock); 
                    frag_item = opal_list_remove_first(&(frag->endpoint->pending_frags_hp));
                    OPAL_THREAD_UNLOCK(&frag->endpoint->endpoint_lock); 
                    frag = (mca_btl_mvapi_frag_t *) frag_item;

                    if(OMPI_SUCCESS !=  mca_btl_mvapi_endpoint_send(frag->endpoint, frag)) { 
                        BTL_ERROR(("error in posting pending send\n"));
                    }
                }
                if( mca_btl_mvapi_component.use_srq && 
                    OPAL_THREAD_ADD32(&mvapi_btl->wr_sq_tokens_hp, 1)  > 0 
                    && !opal_list_is_empty(&mvapi_btl->pending_frags_hp)) {
                    opal_list_item_t *frag_item;
                    frag_item = opal_list_remove_first(&mvapi_btl->pending_frags_hp);
                    frag = (mca_btl_mvapi_frag_t *) frag_item;
                    if(OMPI_SUCCESS !=  mca_btl_mvapi_endpoint_send(frag->endpoint, frag)) { 
                        BTL_ERROR(("error in posting pending send\n"));
                    }
                } 
                   
                
                break;
                
            case VAPI_CQE_RQ_SEND_DATA:
                
                /* Process a RECV  */ 
                BTL_VERBOSE(("Got a recv completion")); 
                frag = (mca_btl_mvapi_frag_t*) (unsigned long) comp.id;
                endpoint = (mca_btl_mvapi_endpoint_t*) frag->endpoint; 

                frag->rc=OMPI_SUCCESS; 
                frag->segment.seg_len =  comp.byte_len-((unsigned char*) frag->segment.seg_addr.pval  - (unsigned char*) frag->hdr); 
                /* advance the segment address past the header and subtract from the length..*/ 
                mvapi_btl->ib_reg[frag->hdr->tag].cbfunc(&mvapi_btl->super, frag->hdr->tag, &frag->base, mvapi_btl->ib_reg[frag->hdr->tag].cbdata);         
                
                OMPI_FREE_LIST_RETURN(&(mvapi_btl->recv_free_eager), (opal_list_item_t*) frag); 
                
                
                if(mca_btl_mvapi_component.use_srq) { 
                    OPAL_THREAD_ADD32(&mvapi_btl->srr_posted_high, -1); 
                    MCA_BTL_MVAPI_POST_SRR_HIGH(mvapi_btl, 0); 
                } else { 
                    OPAL_THREAD_ADD32(&endpoint->rr_posted_high, -1); 
                    MCA_BTL_MVAPI_ENDPOINT_POST_RR_HIGH(((mca_btl_mvapi_frag_t*) (unsigned long) comp.id)->endpoint, 0); 
                }
                count++; 
                break;
                
            default:
                BTL_ERROR(("Unhandled work completion opcode is %d", comp.opcode));
                break;
            }
        }
        
        ret = VAPI_poll_cq(mvapi_btl->nic, mvapi_btl->cq_hndl_low, &comp); 
        if(VAPI_OK == ret) { 
            if(comp.status != VAPI_SUCCESS) { 
                BTL_ERROR(("Got error : %s, Vendor code : %d Frag : %p", 
                            VAPI_wc_status_sym(comp.status), 
                            comp.vendor_err_syndrome, comp.id));  
                return OMPI_ERROR; 
            }
            
            /* Handle n/w completions */
            switch(comp.opcode) {
            case VAPI_CQE_RQ_RDMA_WITH_IMM: 
                BTL_ERROR(("Got an RDMA with Immediate data!, not supported!")); 
                return OMPI_ERROR; 
            
            case VAPI_CQE_SQ_RDMA_READ:
            case VAPI_CQE_SQ_RDMA_WRITE:
            case VAPI_CQE_SQ_SEND_DATA :
                
                /* Process a completed send */
                frag = (mca_btl_mvapi_frag_t*) (unsigned long) comp.id; 
                frag->rc = OMPI_SUCCESS; 
                frag->base.des_cbfunc(&mvapi_btl->super, frag->endpoint, &frag->base, frag->rc); 
                count++;
                /* check and see if we need to progress pending sends */ 
                if(!mca_btl_mvapi_component.use_srq && 
                   OPAL_THREAD_ADD32(&frag->endpoint->wr_sq_tokens_lp, 1) > 0 && 
                   !opal_list_is_empty(&(frag->endpoint->pending_frags_lp))) { 
                    opal_list_item_t *frag_item;
                    OPAL_THREAD_LOCK(&frag->endpoint->endpoint_lock); 
                    frag_item = opal_list_remove_first(&(frag->endpoint->pending_frags_lp));
                    OPAL_THREAD_UNLOCK(&frag->endpoint->endpoint_lock); 
                    frag = (mca_btl_mvapi_frag_t *) frag_item;
                    switch(frag->sr_desc.opcode){
                    case VAPI_SEND: 
                        if(OMPI_SUCCESS !=  mca_btl_mvapi_endpoint_send(frag->endpoint, frag)) { 
                            BTL_ERROR(("error in posting pending send\n"));
                        }
                        break; 
                    case VAPI_RDMA_WRITE: 
                        if(OMPI_SUCCESS !=  mca_btl_mvapi_put((mca_btl_base_module_t*) mvapi_btl, 
                                                              frag->endpoint, 
                                                              (mca_btl_base_descriptor_t*) frag)) { 
                            BTL_ERROR(("error in posting pending rdma write\n"));
                        }
                        break; 
                    case VAPI_RDMA_READ: 
                        if(OMPI_SUCCESS !=  mca_btl_mvapi_put((mca_btl_base_module_t *) mvapi_btl, 
                                                              frag->endpoint,
                                                              (mca_btl_base_descriptor_t*) frag)) { 
                            BTL_ERROR(("error in posting pending rdma read\n"));
                        }
                        break;
                    default: 
                        BTL_ERROR(("error in posting pending operation, invalide opcode %d\n", frag->sr_desc.opcode)); 
                    }
                }
                if(mca_btl_mvapi_component.use_srq && 
                   OPAL_THREAD_ADD32(&mvapi_btl->wr_sq_tokens_lp, 1)  > 0 
                   && !opal_list_is_empty(&mvapi_btl->pending_frags_lp)) {
                    opal_list_item_t *frag_item;
                    frag_item = opal_list_remove_first(&mvapi_btl->pending_frags_lp);
                    frag = (mca_btl_mvapi_frag_t *) frag_item;
                    switch(frag->sr_desc.opcode){
                    case VAPI_SEND: 
                        if(OMPI_SUCCESS !=  mca_btl_mvapi_endpoint_send(frag->endpoint, frag)) { 
                            BTL_ERROR(("error in posting pending send\n"));
                        }
                        break; 
                    case VAPI_RDMA_WRITE: 
                        if(OMPI_SUCCESS !=  mca_btl_mvapi_put((mca_btl_base_module_t*) mvapi_btl, 
                                                              frag->endpoint, 
                                                              (mca_btl_base_descriptor_t*) frag)) { 
                            BTL_ERROR(("error in posting pending rdma write\n"));
                        }
                        break; 
                    case VAPI_RDMA_READ: 
                        if(OMPI_SUCCESS !=  mca_btl_mvapi_put((mca_btl_base_module_t *) mvapi_btl, 
                                                              frag->endpoint,
                                                              (mca_btl_base_descriptor_t*) frag)) { 
                            BTL_ERROR(("error in posting pending rdma read\n"));
                        }
                        break;
                    default: 
                        BTL_ERROR(("error in posting pending operation, invalide opcode %d\n", frag->sr_desc.opcode)); 
                    }
                }
                
                break;
                
            case VAPI_CQE_RQ_SEND_DATA:
                
                BTL_VERBOSE(("Got a recv completion")); 
                frag = (mca_btl_mvapi_frag_t*) (unsigned long) comp.id;
                endpoint = (mca_btl_mvapi_endpoint_t*) frag->endpoint; 
                frag->rc=OMPI_SUCCESS; 
                frag->segment.seg_len =  comp.byte_len-((unsigned char*) frag->segment.seg_addr.pval  - (unsigned char*) frag->hdr); 
                /* advance the segment address past the header and subtract from the length..*/ 
                mvapi_btl->ib_reg[frag->hdr->tag].cbfunc(&mvapi_btl->super, frag->hdr->tag, &frag->base, mvapi_btl->ib_reg[frag->hdr->tag].cbdata);         
                
                OMPI_FREE_LIST_RETURN(&(mvapi_btl->recv_free_max), (opal_list_item_t*) frag); 
                
                
                if(mca_btl_mvapi_component.use_srq) { 
                    OPAL_THREAD_ADD32(&mvapi_btl->srr_posted_low, -1); 
                    MCA_BTL_MVAPI_POST_SRR_LOW(mvapi_btl, 0); 
                } else {
                    OPAL_THREAD_ADD32(&endpoint->rr_posted_low, -1); 
                    MCA_BTL_MVAPI_ENDPOINT_POST_RR_LOW(((mca_btl_mvapi_frag_t*) (unsigned long) comp.id)->endpoint, 0); 
                }
                count++; 
                break;
                
            default:
                BTL_ERROR(("Errorneous network completion"));
                break;
            }
        }
        
    }
    return count;
}

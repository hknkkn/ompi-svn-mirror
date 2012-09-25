/*
 * Copyright (c) 2009-2012 Oak Ridge National Laboratory.  All rights reserved.
 * Copyright (c) 2009-2012 Mellanox Technologies.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */
/**
 * @file
 *
 * Most of the description of the data layout is in the
 * coll_ml_module.c file.
 */

#include "ompi_config.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

#include "ompi/constants.h"
#include "ompi/communicator/communicator.h"
#include "ompi/mca/coll/coll.h"
#include "ompi/mca/coll/base/base.h"
#include "ompi/mca/dpm/dpm.h"
#include "ompi/mca/sbgp/base/base.h"
#include "ompi/mca/bcol/base/base.h"
#include "ompi/mca/sbgp/sbgp.h"
#include "ompi/mca/common/commpatterns/common_coll_ops.h"
#include "ompi/mca/coll/ml/coll_ml.h"

#include "orte/mca/rml/rml.h"
#include "orte/util/proc_info.h"
#include "orte/util/name_fns.h"
#include "orte/mca/grpcomm/grpcomm.h"

#include "opal/util/argv.h"
#include "opal/datatype/opal_datatype.h"
#include "opal/util/output.h"
#include "opal/util/arch.h"

#include "coll_ml.h"
#include "coll_ml_inlines.h"
#include "coll_ml_select.h"
#include "coll_ml_custom_utils.h"
#include "coll_ml_allocation.h"


/* #define NEW_LEADER_SELECTION */

static inline double ret_us(void)
{
    struct timeval t;

    gettimeofday(&t, NULL);

    return t.tv_sec * 1e6 + t.tv_usec;
}

struct ranks_proxy_t {
    /* number of subgroups for which the rank is a proxy */
    int number_subgroups;
    /* subgrou indecies */
    int *subgroup_index;
};
typedef struct rank_proxy_t rank_proxy_t;

#define PROVIDE_SUFFICIENT_MEMORY(ptr, dummy_ptr, ptr_size, unit_type, in_use,  \
                                                            n_to_add,n_to_grow) \
do {                                                                            \
    if ((in_use) + (n_to_add) > (ptr_size)) {                                   \
        (dummy_ptr) = (unit_type *)                                             \
                  realloc(ptr, sizeof(unit_type) * ((ptr_size) + (n_to_grow))); \
        if (NULL != (dummy_ptr)) {                                              \
            (ptr) = (dummy_ptr);                                                \
            (ptr_size) += (n_to_grow);                                          \
        }                                                                       \
    }                                                                           \
} while (0)

/*
 * Local functions
 */

static int ml_module_enable(mca_coll_base_module_t *module,
        struct ompi_communicator_t *comm);

static int mca_coll_ml_fill_in_route_tab(mca_coll_ml_topology_t *topo,
        ompi_communicator_t *comm);

static void
mca_coll_ml_module_construct(mca_coll_ml_module_t *module)
{
    int index_topo, coll_i, st_i;
    mca_coll_ml_topology_t *topo;

    module->max_fn_calls = 0;
    module->initialized = false;
    module->comm = NULL;
    module->collective_sequence_num = 0;
    module->no_data_collective_sequence_num = 0;
    module->payload_block = NULL;

    module->reference_convertor = NULL;

    /* It's critical to reset data_offset to zero */
    module->data_offset = -1;

    module->coll_ml_barrier_function = NULL;

    /* If the topology support zero level and no fragmentation was requested */
    for (index_topo = 0; index_topo < COLL_ML_TOPO_MAX; index_topo++) {
        topo = &module->topo_list[index_topo];
        topo->global_lowest_hier_group_index = -1;
        topo->global_highest_hier_group_index = -1;
        topo->number_of_all_subgroups = -1;
        topo->n_levels = -1;
        topo->sort_list = NULL;
        topo->hier_layout_info = NULL;
        topo->all_bcols_mode = ~(0); /* set to all bits */
        topo->route_vector = NULL;
        topo->array_of_all_subgroups = NULL;
        topo->component_pairs = NULL;
        topo->hier_layout_info = NULL;
        topo->status = COLL_ML_TOPO_DISABLED; /* all topologies are not used by default */

        /* Init ordering info */
        topo->topo_ordering_info.next_inorder = 0;
        topo->topo_ordering_info.next_order_num = 0;
        topo->topo_ordering_info.num_bcols_need_ordering = 0;

        memset(topo->hierarchical_algorithms, 0,
                BCOL_NUM_OF_FUNCTIONS * sizeof(coll_ml_collective_description_t *));
    }

    for (coll_i = 0; coll_i < ML_NUM_OF_FUNCTIONS; coll_i++) {
        for (st_i = 0; st_i < MCA_COLL_MAX_NUM_SUBTYPES; st_i++) {
            module->collectives_topology_map[coll_i][st_i] = ML_UNDEFINED;
        }
    }

    for (coll_i = 0; coll_i < BCOL_NUM_OF_FUNCTIONS; ++coll_i) {
        module->small_message_thresholds[coll_i] = BCOL_THRESHOLD_UNLIMITED;
    }

    OBJ_CONSTRUCT(&(module->active_bcols_list), opal_list_t);
    OBJ_CONSTRUCT(&(module->waiting_for_memory_list), opal_list_t);
}

static void
mca_coll_ml_module_destruct(mca_coll_ml_module_t *module)
{
    int i, j, k,fnc, index_topo;
    mca_coll_ml_topology_t *topo;

    ML_VERBOSE(4, ("ML module destruct"));

    if (module->initialized) {
        for (index_topo = 0; index_topo < COLL_ML_TOPO_MAX; index_topo++) {
            topo = &module->topo_list[index_topo];
            if (COLL_ML_TOPO_DISABLED == topo->status) {
                /* skip the topology */
                continue;
            }

            if (NULL != topo->component_pairs) {
                for(i = 0; i < topo->n_levels; ++i) {
                    for(j = 0; j < topo->component_pairs[i].num_bcol_modules; ++j) {
                        OBJ_RELEASE(topo->component_pairs[i].bcol_modules[j]);
                    }
                    /* free the array of bcol module */
                    free(topo->component_pairs[i].bcol_modules);

                    OBJ_RELEASE(topo->component_pairs[i].subgroup_module);
                }

                free(topo->component_pairs);
            }

            /* gvm Leak FIX Free collective algorithms structure */
            for (fnc = 0; fnc < BCOL_NUM_OF_FUNCTIONS; fnc++) {
                if (NULL != topo->hierarchical_algorithms[fnc]){
                    free(topo->hierarchical_algorithms[fnc]);
                }
            }

            /* free up the route vector memory */
            if (NULL != topo->route_vector) {
                free(topo->route_vector);
            }
            /* free resrouce description */
            if(NULL != topo->array_of_all_subgroups) {
                for( k=0 ; k < topo->number_of_all_subgroups ; k++ ) {
                    if(0 < topo->array_of_all_subgroups[k].n_ranks) {
                        for(i=0 ; i < topo->array_of_all_subgroups[k].n_ranks ; i++ )
                        {
                            if(0 < topo->array_of_all_subgroups[k].rank_data[i].n_connected_subgroups) {
                                free(topo->array_of_all_subgroups[k].rank_data[i].list_connected_subgroups);
                                topo->array_of_all_subgroups[k].rank_data[i].list_connected_subgroups=NULL;
                            }
                        }
                        free(topo->array_of_all_subgroups[k].rank_data);
                        topo->array_of_all_subgroups[k].rank_data = NULL;
                    }
                }
                free(topo->array_of_all_subgroups);
                topo->array_of_all_subgroups = NULL;
            }

            if(NULL != topo->sort_list) {
                free(topo->sort_list);
                topo->sort_list=NULL;
            }
        }

        OBJ_DESTRUCT(&(module->active_bcols_list));
        OBJ_DESTRUCT(&(module->waiting_for_memory_list));

        /* gvm Leak FIX Remove fragment free list */
        OBJ_DESTRUCT(&(module->fragment_descriptors));
        OBJ_DESTRUCT(&(module->message_descriptors));
        /* push ml_memory_block_desc_t back on list manager */
        mca_coll_ml_free_block(module->payload_block);
        /* release the cinvertor if it was allocated */
        if (NULL != module->reference_convertor) {
            OBJ_RELEASE(module->reference_convertor);
        }

        OBJ_DESTRUCT(&(module->coll_ml_collective_descriptors));

        if (NULL != module->coll_ml_barrier_function) {
            free(module->coll_ml_barrier_function);
        }
    }
}


static int mca_coll_ml_request_free(ompi_request_t** request)
{
    /* local variables */
    mca_coll_ml_collective_operation_progress_t *ml_request=
        (mca_coll_ml_collective_operation_progress_t *)(*request);
    mca_coll_ml_module_t *ml_module = OP_ML_MODULE(ml_request);

    /* The ML memory bank recycling check done, no we may
     * return request and signal completion */

    /* this fragement does not hold the message data, so ok to return */
    assert(0 == ml_request->pending);
    assert(0 == ml_request->fragment_data.offset_into_user_buffer);
    assert(ml_request->dag_description.status_array[0].item.opal_list_item_refcount == 0);
    ML_VERBOSE(10, ("Releasing Master %p", ml_request));
    OMPI_FREE_LIST_RETURN(&(ml_module->coll_ml_collective_descriptors),
            (ompi_free_list_item_t *)ml_request);

    /* MPI needs to return with the request object set to MPI_REQUEST_NULL
     */
    *request = MPI_REQUEST_NULL;

    return OMPI_SUCCESS;
}

/* constructor for collective managment descriptor */
static void mca_coll_ml_collective_operation_progress_construct
(mca_coll_ml_collective_operation_progress_t *desc) {

    /* initialize pointer */
    desc->dag_description.status_array = NULL;

    OBJ_CONSTRUCT(&desc->full_message.super, ompi_request_t);
    OBJ_CONSTRUCT(&desc->full_message.send_convertor, opal_convertor_t);
    OBJ_CONSTRUCT(&desc->full_message.recv_convertor, opal_convertor_t);

    OBJ_CONSTRUCT(&desc->full_message.dummy_convertor, opal_convertor_t);

    /* intialize request free pointer */
    desc->full_message.super.req_free = mca_coll_ml_request_free;

    /* no cancel function */
    desc->full_message.super.req_cancel = NULL;
    /* Collective request type */
    desc->full_message.super.req_type = OMPI_REQUEST_COLL;
    /* RLG: Do we need to set req_mpi_object ? */

    /* If not null , we have to release next fragment */
    desc->next_to_process_frag = NULL;

    /* pointer to previous fragment */
    desc->prev_frag = NULL;

    /* Pasha: moreinit */
    desc->pending = 0;
}

/* destructor for collective managment descriptor */
static void mca_coll_ml_collective_operation_progress_destruct
               (mca_coll_ml_collective_operation_progress_t *desc) {
    mca_coll_ml_module_t *ml_module =
            (mca_coll_ml_module_t *) desc->coll_module;

    int i, max_dag_size = ml_module->max_dag_size;

    if (NULL != desc->dag_description.status_array) {
        for (i = 0; i < max_dag_size; ++i) {
            OBJ_DESTRUCT(&desc->dag_description.status_array[i].item);
        }

        free(desc->dag_description.status_array);
        desc->dag_description.status_array = NULL;
    }

    OBJ_DESTRUCT(&desc->full_message.super);
    OBJ_DESTRUCT(&desc->full_message.send_convertor);
    OBJ_DESTRUCT(&desc->full_message.recv_convertor);

    OBJ_DESTRUCT(&desc->full_message.dummy_convertor);
}
/* initialize the full message descriptor - can pass in module specific
 * initialization data
 */
static void init_ml_fragment_desc(ompi_free_list_item_t *desc , void* ctx);
static void init_ml_message_desc(ompi_free_list_item_t *desc , void* ctx)
{
   mca_coll_ml_module_t *module= (mca_coll_ml_module_t *) ctx;
   mca_coll_ml_descriptor_t *msg_desc = (mca_coll_ml_descriptor_t *) desc;

   /* finish setting up the fragment descriptor */
   init_ml_fragment_desc((ompi_free_list_item_t*)&(msg_desc->fragment),module);
}

/* initialize the fragment descriptor - can pass in module specific
 * initialization data
 */
static void init_ml_fragment_desc(ompi_free_list_item_t *desc , void* ctx)
{
   mca_coll_ml_module_t *module= (mca_coll_ml_module_t *) ctx;
   mca_coll_ml_fragment_t *frag_desc = (mca_coll_ml_fragment_t *) desc;

  /* allocated array of function arguments */
  /* RLG - we have a problem if we don't get the memory */
   /* malloc-debug does not like zero allocations */
   if (module->max_fn_calls > 0) {
       frag_desc->fn_args = (bcol_function_args_t *)
                      malloc(sizeof(bcol_function_args_t) * module->max_fn_calls);
   }

}
static void mca_coll_ml_bcol_list_item_construct(mca_coll_ml_bcol_list_item_t *item)
{
    item->bcol_module = NULL;
}
OBJ_CLASS_INSTANCE(mca_coll_ml_bcol_list_item_t,
                   opal_list_item_t,
                   mca_coll_ml_bcol_list_item_construct,
                   NULL);

static void generate_active_bcols_list(mca_coll_ml_module_t *ml_module)
{
    int i, j, index_topo;
    mca_coll_ml_topology_t *topo;
    bool bcol_was_found;
    mca_coll_ml_bcol_list_item_t *bcol_item = NULL;
    mca_bcol_base_module_t *bcol_module = NULL;

    ML_VERBOSE(10, ("Generating active bcol list "));

    for (index_topo = 0; index_topo < COLL_ML_TOPO_MAX; index_topo++) {
        topo = &ml_module->topo_list[index_topo];
        if (COLL_ML_TOPO_DISABLED == topo->status) {
            /* skip the topology */
            continue;
        }
        for( i = 0; i < topo->n_levels; i++) {

            for( j = 0; j < topo->component_pairs[i].num_bcol_modules; j++) {
                bcol_module = topo->component_pairs[i].bcol_modules[j];

                /* Check if the bcol provides synchronization function, if the
                 * function is not provided we skip this bcol, since it isn't used
                 * for memory synchronization (for instance - ptpcoll )*/
                if (NULL == GET_BCOL_SYNC_FN(bcol_module)) {
                    ML_VERBOSE(10,(" No sync function was provided by bcol %s\n",
                                bcol_module->bcol_component->bcol_version.mca_component_name));
                    continue;
                }

                bcol_was_found = false;
                for(bcol_item = (mca_coll_ml_bcol_list_item_t *)opal_list_get_first(&ml_module->active_bcols_list);
                    !bcol_was_found &&
                    bcol_item != (mca_coll_ml_bcol_list_item_t *)opal_list_get_end(&ml_module->active_bcols_list);
                    bcol_item = (mca_coll_ml_bcol_list_item_t *)opal_list_get_next((opal_list_item_t *)bcol_item)) {
                    if (bcol_module == bcol_item->bcol_module) {
                        bcol_was_found = true;
                    }
                }

                /* append the item to the list if it was not found */
                if (!bcol_was_found) {
                    bcol_item = OBJ_NEW(mca_coll_ml_bcol_list_item_t);
                    bcol_item->bcol_module = bcol_module;
                    opal_list_append(&ml_module->active_bcols_list, (opal_list_item_t *)bcol_item);
                }

            }
        }
    }
}

static int calculate_buffer_header_size(mca_coll_ml_module_t *ml_module)
{
    mca_coll_ml_topology_t *topo;
    mca_bcol_base_module_t *bcol_module;

    uint32_t offset = 0;
    int i, j, *ranks_in_comm, kount = 0,
        rc, data_offset = 0, index_topo,
        comm_size = ompi_comm_size(ml_module->comm);

    ML_VERBOSE(10, ("Calculating offset for the ML"));

    /* probably a stupid thing to do, but we have to loop over twice */

    for (index_topo = 0; index_topo < COLL_ML_TOPO_MAX; index_topo++) {
        topo = &ml_module->topo_list[index_topo];
        if (COLL_ML_TOPO_DISABLED == topo->status) {
            /* skip the topology */
            continue;
        }

        for (i = 0; i < topo->n_levels; i++) {
            for (j = 0; j < topo->component_pairs[i].num_bcol_modules; j++) {
                bcol_module = topo->component_pairs[i].bcol_modules[j];
                if (0 < bcol_module->header_size) {
                    /* bump the kounter */
                    kount++;
                    /* find the largest header request */
                    if (offset < bcol_module->header_size) {
                        offset = bcol_module->header_size;
                    }
               }

                /* Set bcol mode bits */
                topo->all_bcols_mode &= bcol_module->supported_mode;
           }
        }

        offset = ((offset + BCOL_HEAD_ALIGN - 1) / BCOL_HEAD_ALIGN) * BCOL_HEAD_ALIGN;
        /* select largest offset between multiple topologies */
        if (data_offset < (int) offset) {
            data_offset = (int) offset;
        }
    }

    ranks_in_comm = (int *) malloc(comm_size * sizeof(int));
    if (OPAL_UNLIKELY(NULL == ranks_in_comm)) {
        ML_ERROR(("Memory allocation failed."));
        return OMPI_ERROR;
    }

    for (i = 0; i < comm_size; ++i) {
        ranks_in_comm[i] = i;
    }

    rc = comm_allreduce_pml(&data_offset, &data_offset, 1,
                            MPI_INT, ompi_comm_rank(ml_module->comm),
                            MPI_MAX, comm_size,
                            ranks_in_comm, ml_module->comm);

    if (OPAL_UNLIKELY(OMPI_SUCCESS != rc)) {
        ML_ERROR(("comm_allreduce_pml failed."));
        return OMPI_ERROR;
    }

    ml_module->data_offset = (uint32_t) data_offset;
    free(ranks_in_comm);

    ML_VERBOSE(10, ("The offset is %d", ml_module->data_offset));

    return OMPI_SUCCESS;
}

static int mca_coll_ml_register_bcols(mca_coll_ml_module_t *ml_module)
{
    /* local variables */
    int i, j, index_topo;
    int ret = OMPI_SUCCESS;
    mca_bcol_base_module_t *bcol_module;
    mca_coll_ml_topology_t *topo;

    /* loop over all bcols and register the ml memory block which each */
    for (index_topo = 0; index_topo < COLL_ML_TOPO_MAX; index_topo++) {
        topo = &ml_module->topo_list[index_topo];
        if (COLL_ML_TOPO_DISABLED == topo->status) {
            /* skip the topology */
            continue;
        }

        for (i = 0; i < topo->n_levels; i++) {
            for (j = 0; j < topo->component_pairs[i].num_bcol_modules; j++) {
                bcol_module = topo->component_pairs[i].bcol_modules[j];
                if (NULL != bcol_module->bcol_memory_init) {
                    ret = bcol_module->bcol_memory_init(ml_module,
                            bcol_module,
                            (NULL != bcol_module->network_context) ?
                            bcol_module->network_context->context_data: NULL);
                    if (OMPI_SUCCESS != ret) {
                        ML_ERROR(("Bcol registration failed on ml level!!"));
                        return ret;
                    }
                }
            }
        }
    }

    return OMPI_SUCCESS;
}

static int ml_module_memory_initialization(mca_coll_ml_module_t *ml_module)
{
    int ret;
    int nbanks, nbuffers, buf_size;
    mca_coll_ml_component_t *cs = &mca_coll_ml_component;

    ml_module->payload_block = mca_coll_ml_allocate_block(cs,ml_module->payload_block);

    if (NULL == ml_module->payload_block) {
        ML_ERROR(("mca_coll_ml_allocate_block exited with error.\n"));
        return OMPI_ERROR;
    }

    /* get memory block parameters */
    nbanks = cs->n_payload_mem_banks;
    nbuffers = cs->n_payload_buffs_per_bank;
    buf_size = cs->payload_buffer_size;

    ML_VERBOSE(10, ("Call for initialize block.\n"));

    ret = mca_coll_ml_initialize_block(ml_module->payload_block,
            nbuffers, nbanks, buf_size, ml_module->data_offset,
            NULL);
    if (OMPI_SUCCESS != ret) {
        return ret;
    }

    ML_VERBOSE(10, ("Call for register bcols.\n"));

    /* inititialize the memory with all of the bcols:
       loop through the bcol modules and invoke the memory init */
    ret = mca_coll_ml_register_bcols(ml_module);
    if (OMPI_SUCCESS != ret) {
        ML_ERROR(("mca_coll_ml_register_bcols returned an error.\n"));
        /* goto CLEANUP; */
        return ret;
    }

    return OMPI_SUCCESS;
}

/* do some sanity checks */
static int check_global_view_of_subgroups( int n_procs_selected,
        int n_procs_in, int ll_p1, int* all_selected,
        mca_sbgp_base_module_t *module )
{
    /* local variables */
    int ret=OMPI_SUCCESS;
    int i, sum;

    bool local_leader_found=false;

    /* is there a single local-leader */
    for (i = 0; i < n_procs_selected; i++) {
        if( ll_p1 == -all_selected[module->group_list[i]]) {
            /* found the local leader */
            if( local_leader_found ) {
                /* more than one local leader - don't know how to
                 * handle this, so bail
                 */
                ML_VERBOSE(0, ("More than a single leader for a group.\n"));
                ret=OMPI_ERROR;
                goto ERROR;
            } else {
                local_leader_found=true;
            }
        }
    }

    /* check to make sure that all agree on the same size of
     * the group
     */
    sum=0;
    for (i = 0; i < n_procs_in; i++) {
        if(ll_p1==all_selected[i]) {
            sum++;
        } else if( ll_p1 == -all_selected[i]) {
            sum++;
        }
    }
    if( sum != n_procs_selected ) {
        fprintf(stderr,"n procs in %d\n",n_procs_in);
        ML_VERBOSE(0, ("number of procs in the group unexpeted.  Expected %d Got %d\n",n_procs_selected,sum));
        ret=OMPI_ERROR;
        goto ERROR;
    }
    /* check to make sure that all have the same list of ranks.
     */
    for (i = 0; i < n_procs_selected; i++) {
        if(ll_p1!=all_selected[module->group_list[i]] &&
                ll_p1!=-all_selected[module->group_list[i]] ) {
            ret=OMPI_ERROR;
            ML_VERBOSE(0, ("Mismatch in rank list - element #%d - %d \n",i,all_selected[module->group_list[i]]));
            goto ERROR;
        }
    }

    /* return */
    return ret;

ERROR:
    /* return */
    return ret;
}


static void ml_init_k_nomial_trees(mca_coll_ml_topology_t *topo, int *list_of_ranks_in_all_subgroups, int my_rank_in_list)
{
    int *list_n_connected;
    int *list;
    int group_size, rank, i, j, knt, offset, k, my_sbgp = 0;
    int my_root;
    int level_one_knt;
    sub_group_params_t *array_of_all_subgroup_ranks = topo->
                                      array_of_all_subgroups;
    int num_total_subgroups = topo->number_of_all_subgroups;
    int n_hier = topo->n_levels;

    hierarchy_pairs *pair = NULL;
    mca_coll_ml_leader_offset_info_t *loc_leader = (mca_coll_ml_leader_offset_info_t *)
                                                  malloc(sizeof(mca_coll_ml_leader_offset_info_t)*(n_hier+1));

    /* first thing I want to know is where does the first level end */
    level_one_knt = 0;
    while( 0 == array_of_all_subgroup_ranks[level_one_knt].level_in_hierarchy &&
            level_one_knt < num_total_subgroups){
        level_one_knt++;
    }
    /*
    fprintf(stderr,"PPP %d %d %d \n", level_one_knt, array_of_all_subgroup_ranks[0].level_in_hierarchy, num_total_subgroups);
     */
    /* I want to cache this number for unpack*/
    array_of_all_subgroup_ranks->level_one_index = level_one_knt;

    /* determine whether or not ranks are contiguous */
    topo->ranks_contiguous = true;
    knt = 0;
    for( i = 0; i < level_one_knt; i++){
       for( j =0; j < array_of_all_subgroup_ranks[i].n_ranks; j++){
           if(knt != list_of_ranks_in_all_subgroups[knt]){
               topo->ranks_contiguous = false;
               i = level_one_knt;
               break;
           }
           knt++;
       }
    }

    /* now find my first level offset, and my index in level one */
    knt = 0;
    for(i = 0; i < level_one_knt; i++){
        offset = array_of_all_subgroup_ranks[i].index_of_first_element;
        for( k = 0; k < array_of_all_subgroup_ranks[i].n_ranks; k++){
            rank = list_of_ranks_in_all_subgroups[k + offset];
            if(rank == my_rank_in_list){
                loc_leader[0].offset = knt;
                loc_leader[0].level_one_index = k;
                i = level_one_knt;
                break;
            }
        }
        knt += array_of_all_subgroup_ranks[i].n_ranks;
    }



    for(i = 0; i < n_hier; i++){
        pair = &topo->component_pairs[i];
        /* find the size of the group */
        group_size = pair->subgroup_module->group_size;
        /* malloc some memory for the new list to cache
           on the bcol module
         */
        list_n_connected = (int *) malloc(sizeof(int)*group_size);
        /* pointer to group list */
        list = pair->subgroup_module->group_list;
        /* next thing to do is to find out which subgroup I'm in
         * at this particular level
         */
        knt = 0;
        for( j = 0; j < num_total_subgroups; j++){
            offset = array_of_all_subgroup_ranks[j].index_of_first_element;
            for( k = 0; k < array_of_all_subgroup_ranks[j].n_ranks; k++){
                rank = list_of_ranks_in_all_subgroups[k+offset];
                if(rank == my_rank_in_list){
                    knt++;
                }
                if(knt == (i+1)){
                    my_sbgp = j;
                    /* tag whether I am a local leader or not at this level */
                    if( my_rank_in_list == array_of_all_subgroup_ranks[j].root_rank_in_comm){
                        loc_leader[i].leader = true;
                    } else {
                        loc_leader[i].leader = false;
                    }
                    j = num_total_subgroups;
                    break;
                }
            }
        }

        for( j = 0; j < group_size; j++ ) {
            list_n_connected[j] = array_of_all_subgroup_ranks[my_sbgp].
                rank_data[j].num_of_ranks_represented;
        }

        /* now find all sbgps that the root of this sbgp belongs to
           previous to this "my_sbgp"
         */

        my_root = array_of_all_subgroup_ranks[my_sbgp].root_rank_in_comm;
        knt=0;
        for(j = 0; j < my_sbgp; j++){
            if(array_of_all_subgroup_ranks[j].root_rank_in_comm ==
                    my_root){
                for(k = 1; k < array_of_all_subgroup_ranks[j].n_ranks;
                        k++){
                    knt += array_of_all_subgroup_ranks[j].rank_data[k].
                        num_of_ranks_represented;
                }

            }
        }
        /* and then I add one for the root itself */
        list_n_connected[0] = knt+1;



        /* now cache this on the bcol module */
        pair->bcol_modules[0]->list_n_connected = list_n_connected;


        /*  I should do one more round here and figure out my offset at this level
         *  the calculation is simple: Am I a local leader in this level? If so, then I keep the offset
         *  from the previous level. Else, I find out how "far away" the local leader is from me and set
         *  this as the new offset.
         */
        /* do this after first level */
        if (i > 0) {
            /* if I'm not the local leader */
            if( !loc_leader[i].leader) {
                knt = 0;
                /* then I am not a local leader at this level */
                offset = array_of_all_subgroup_ranks[my_sbgp].index_of_first_element;
                for( k = 0; k < array_of_all_subgroup_ranks[my_sbgp].n_ranks; k++){
                    rank = list_of_ranks_in_all_subgroups[k+offset];
                    if(rank == my_rank_in_list){
                        break;
                    } else {

                        knt += list_n_connected[k];
                    }
                }
                loc_leader[i].offset = loc_leader[i-1].offset - knt;
                pair->bcol_modules[0]->hier_scather_offset = loc_leader[i].offset;
            }else{
               /* if I am the local leader, then keep the same offset */
               loc_leader[i].offset = loc_leader[i-1].offset;
                pair->bcol_modules[0]->hier_scather_offset = loc_leader[i-1].offset;
            }
        } else {

           pair->bcol_modules[0]->hier_scather_offset = loc_leader[0].offset;
        }

        /*setup the tree */
        pair->bcol_modules[0]->k_nomial_tree(pair->bcol_modules[0]);


    }

    /* see if I am in the last subgroup, if I am,
     * then I am a root for the bcast operation
     */
    offset = array_of_all_subgroup_ranks[n_hier - 1].index_of_first_element;
    for( i = 0; i < array_of_all_subgroup_ranks[n_hier - 1].n_ranks; i++){
        rank = list_of_ranks_in_all_subgroups[i + offset];
        if( rank == my_rank_in_list ){
            loc_leader[n_hier - 1].offset = 0;
            loc_leader[n_hier - 1].leader = true;
        }
    }

    /* set the last offset to 0 and set the leader according to your top level position */
    loc_leader[n_hier].offset = 0;
    if(loc_leader[n_hier - 1].leader){
        loc_leader[n_hier].leader = true;
    } else {
       loc_leader[n_hier].leader = false;
    }

    /* what other goodies do I want to cache on the ml-module? */
    topo->hier_layout_info = loc_leader;
}

/* for a given rank in a subgroup, find out the number of ranks this subgroup
 * represents.  It uses a depth-first search to recursively traverse
 * subgroups conncted to the subgroup.
 */

static int ml_compute_number_unique_proxy_ranks(
        int subgroup_index, int rank_index,
        int *sub_groups_in_lineage,int *len_sub_groups_in_lineage,
        sub_group_params_t *array_of_all_subgroup_ranks)
{
    /* local variables */
    int total=0, i_rank, sg_i, sub_grp, depth;
    bool found;

    /* Do I represent several subgroups ? */
    if( array_of_all_subgroup_ranks[subgroup_index].rank_data[rank_index].
            n_connected_subgroups ) {
        for( sg_i = 0 ; sg_i <
                array_of_all_subgroup_ranks[subgroup_index].
                rank_data[rank_index].n_connected_subgroups ; sg_i++ ) {
            sub_grp= array_of_all_subgroup_ranks[subgroup_index].
                rank_data[rank_index].list_connected_subgroups[sg_i];

            /* make sure we don't loop back on ourselves */
            found=false;
            for(depth=0 ; depth < *len_sub_groups_in_lineage
                    ; depth++ ){
                if(sub_groups_in_lineage[depth]==sub_grp)
                {
                    found=true;
                    break;
                }
            }
            if(found) {
                continue;
            }

            sub_groups_in_lineage[(*len_sub_groups_in_lineage)]=sub_grp;
            (*len_sub_groups_in_lineage)++;
            for(i_rank = 0 ;
                    i_rank < array_of_all_subgroup_ranks[sub_grp].n_ranks ;
                    i_rank++) {
                total+=ml_compute_number_unique_proxy_ranks(
                        sub_grp, i_rank, sub_groups_in_lineage,
                        len_sub_groups_in_lineage, array_of_all_subgroup_ranks);
            }
            (*len_sub_groups_in_lineage)--;
        }
    }
    /* if I am a leaf, count me */
    if( array_of_all_subgroup_ranks[subgroup_index].rank_data[rank_index].
            leaf ) {
        total++;
    }

    /* return */
    return total;

}

static void ml_compute_create_unique_proxy_rank_list(
        int subgroup_index,
        int *sub_groups_in_lineage,int *len_sub_groups_in_lineage,
        sub_group_params_t *array_of_all_subgroup_ranks,
        int *current_list_length, int *sorted_rank_list)
{
    /* local variables */
    int i_rank, sg_i, sub_grp, depth;
    bool found;

    /* loop over all the element of ths subgroup */
    for(i_rank = 0 ; i_rank < array_of_all_subgroup_ranks[subgroup_index].n_ranks ;
            i_rank++) {
        if(array_of_all_subgroup_ranks[subgroup_index].rank_data[i_rank].leaf){
            /* found leaf - add to the list */
            sorted_rank_list[(*current_list_length)]=
                array_of_all_subgroup_ranks[subgroup_index].rank_data[i_rank].rank;
            (*current_list_length)++;
        }
        if( array_of_all_subgroup_ranks[subgroup_index].rank_data[i_rank].
                n_connected_subgroups ) {
            /* loop over all connected subgroups */
            for( sg_i = 0 ; sg_i <
                    array_of_all_subgroup_ranks[subgroup_index].
                    rank_data[i_rank].n_connected_subgroups ; sg_i++ ) {
                sub_grp= array_of_all_subgroup_ranks[subgroup_index].
                    rank_data[i_rank].list_connected_subgroups[sg_i];

                /* make sure we don't loop back on ourselves */
                found=false;
                for(depth=0 ; depth < *len_sub_groups_in_lineage
                        ; depth++ ){
                    if(sub_groups_in_lineage[depth]==sub_grp)
                    {
                        found=true;
                        break;
                    }
                }
                if(found) {
                    continue;
                }

                sub_groups_in_lineage[(*len_sub_groups_in_lineage)]=sub_grp;
                (*len_sub_groups_in_lineage)++;
                ml_compute_create_unique_proxy_rank_list(
                        sub_grp, sub_groups_in_lineage,
                        len_sub_groups_in_lineage, array_of_all_subgroup_ranks,
                        current_list_length, sorted_rank_list);
                (*len_sub_groups_in_lineage)--;
            }
        }
    }
    return;

}

static int ml_setup_full_tree_data(mca_coll_ml_topology_t *topo,
        ompi_communicator_t *comm,
        int my_highest_group_index, int *map_to_comm_ranks,
        int *num_total_subgroups, sub_group_params_t **array_of_all_subgroup_ranks,
        int **list_of_ranks_in_all_subgroups)
{

    int ret = OMPI_SUCCESS;
    int i, j, k, in_buf, root, my_rank,sum;
    int in_num_total_subgroups = *num_total_subgroups;
    int i_sg, i_cnt, i_rank, i_offset, i_level, j_sg, j_rank,
        j_level, j_root,cnt, rank, rank_cnt;
    int *scratch_space = NULL;
    bool found;
    /* figure out who holds all the sub-group information - only those
     * ranks in the top level know this data at this point */
    my_rank = ompi_comm_rank(comm);
    if( (my_highest_group_index == topo->global_highest_hier_group_index )
            &&
            ( my_rank ==
            topo->component_pairs[topo->n_levels-1].subgroup_module->group_list[0])
            ) {
        in_buf=my_rank;
    } else {
        /* since this will be a sum allreduce - contributing 0 will not
         * change the value */
        in_buf=0;
    }
    ret = comm_allreduce_pml(&in_buf, &root, 1, MPI_INT,
            my_rank, MPI_SUM,
            ompi_comm_size(comm), map_to_comm_ranks,
            comm);
    if (OPAL_UNLIKELY(OMPI_SUCCESS != ret)) {
        ML_VERBOSE(10, ("comm_allreduce_pml failed. root reduction\n"));
        goto ERROR;
    }

    /* broadcast the number of groups */
    ret=comm_bcast_pml(num_total_subgroups, root, 1,
            MPI_INT, my_rank, ompi_comm_size(comm),
            map_to_comm_ranks,comm);
    if (OPAL_UNLIKELY(OMPI_SUCCESS != ret)) {
        ML_VERBOSE(10, ("comm_bcast_pml failed. num_total_subgroups bcast\n"));
        goto ERROR;
    }

    scratch_space=(int *)malloc(4*sizeof(int)*(*num_total_subgroups));
    if (OPAL_UNLIKELY(NULL == scratch_space)) {
        ML_VERBOSE(10, ("Cannot allocate memory scratch_space.\n"));
        ret = OMPI_ERR_OUT_OF_RESOURCE;
        goto ERROR;
    }
    if( my_rank == root ) {
        sum=0;
        for(i=0 ; i < (*num_total_subgroups) ; i++ ) {
            scratch_space[4*i]=(*array_of_all_subgroup_ranks)[i].root_rank_in_comm;
            scratch_space[4*i+1]=(*array_of_all_subgroup_ranks)[i].n_ranks;
            scratch_space[4*i+2]=(*array_of_all_subgroup_ranks)[i].index_of_first_element;
            scratch_space[4*i+3]=(*array_of_all_subgroup_ranks)[i].level_in_hierarchy;
        }
    }
    ret=comm_bcast_pml(scratch_space, root, 4*(*num_total_subgroups),
            MPI_INT, my_rank, ompi_comm_size(comm),
            map_to_comm_ranks, comm);
    if (OPAL_UNLIKELY(OMPI_SUCCESS != ret)) {
        ML_VERBOSE(10, ("comm_allreduce_pml failed. scratch_space bcast\n"));
        goto ERROR;
    }
    if( my_rank != root ) {
        if( in_num_total_subgroups != (*num_total_subgroups) ) {
            /* free old array_of_all_subgroup_ranks array - need to fill it
             * with the global data - assume that if the array size is the
             * same, all data is correct, and in the same order */
            free((*array_of_all_subgroup_ranks));
            (*array_of_all_subgroup_ranks)=(sub_group_params_t *)
                malloc(sizeof(sub_group_params_t)*(*num_total_subgroups));
            if (OPAL_UNLIKELY(NULL == (*array_of_all_subgroup_ranks))) {
                ML_VERBOSE(10, ("Cannot allocate memory array_of_all_subgroup_ranks.\n"));
                ret = OMPI_ERR_OUT_OF_RESOURCE;
                goto ERROR;
            }
            for(i=0 ; i < (*num_total_subgroups) ; i++ ) {
                (*array_of_all_subgroup_ranks)[i].root_rank_in_comm=scratch_space[4*i];
                (*array_of_all_subgroup_ranks)[i].n_ranks=scratch_space[4*i+1];
                (*array_of_all_subgroup_ranks)[i].index_of_first_element=scratch_space[4*i+2];
                (*array_of_all_subgroup_ranks)[i].level_in_hierarchy=scratch_space[4*i+3];
            }
        }
    }
    /* figure out how many entries in all the subgroups - ranks that apear
     * in k subgroups appear k times in the list */
    sum=0;
    for(i=0 ; i < (*num_total_subgroups) ; i++ ) {
        sum+=(*array_of_all_subgroup_ranks)[i].n_ranks;
    }
    if( in_num_total_subgroups != (*num_total_subgroups) ) {
        (*list_of_ranks_in_all_subgroups)=(int *)
            realloc((*list_of_ranks_in_all_subgroups),sizeof(int)*sum);
            if (OPAL_UNLIKELY(NULL == (*list_of_ranks_in_all_subgroups))) {
                ML_VERBOSE(10, ("Cannot allocate memory *list_of_ranks_in_all_subgroups.\n"));
                ret = OMPI_ERR_OUT_OF_RESOURCE;
                goto ERROR;
            }
    }
    ret=comm_bcast_pml(*list_of_ranks_in_all_subgroups, root, sum,
            MPI_INT, my_rank, ompi_comm_size(comm),
            map_to_comm_ranks, comm);
    if (OPAL_UNLIKELY(OMPI_SUCCESS != ret)) {
        ML_VERBOSE(10, ("Bcast failed for list_of_ranks_in_all_subgroups \n"));
        goto ERROR;
    }

    /* fill in subgroup ranks */
    for(i=0 ; i < (*num_total_subgroups) ; i++ ) {
        int k=(*array_of_all_subgroup_ranks)[i].index_of_first_element;
        sum=(*array_of_all_subgroup_ranks)[i].n_ranks;
        (*array_of_all_subgroup_ranks)[i].rank_data=(rank_properties_t *)
            malloc(sizeof(rank_properties_t)*sum);
        if (OPAL_UNLIKELY(NULL ==
                    (*array_of_all_subgroup_ranks)[i].rank_data ) ) {

            ML_VERBOSE(10, ("Cannot allocate memory for rank_data \n"));
            ret = OMPI_ERR_OUT_OF_RESOURCE;
            goto ERROR;
        }
        for(j=0 ; j < (*array_of_all_subgroup_ranks)[i].n_ranks ; j++ ) {
            (*array_of_all_subgroup_ranks)[i].rank_data[j].rank=
                (*list_of_ranks_in_all_subgroups)[k+j];
            /* initial value - an element is not a leaf only at the
             * first lowest level that it shows up in the tree */
            (*array_of_all_subgroup_ranks)[i].rank_data[j].leaf=0;
        }
    }

    /* find the first occurance of a rank in the tree */
    for(rank = 0; rank < ompi_comm_size(comm); rank++) {
        for( i=0 ; i < (*num_total_subgroups) ; i++ ) {
            for(j=0 ; j < (*array_of_all_subgroup_ranks)[i].n_ranks ; j++ ) {
                if( rank ==
                        (*array_of_all_subgroup_ranks)[i].rank_data[j].rank ) {
                    (*array_of_all_subgroup_ranks)[i].rank_data[j].leaf=1;
                    goto NextRank;
                }
            }
        }
NextRank:
        continue;
    }

    /* figure out the index of the root in the subgroup */
    for(i_sg=0 ; i_sg < (*num_total_subgroups); i_sg++) {
        int root=(*array_of_all_subgroup_ranks)[i_sg].root_rank_in_comm;
        i_cnt=(*array_of_all_subgroup_ranks)[i_sg].n_ranks;
        i_offset=(*array_of_all_subgroup_ranks)[i_sg].index_of_first_element;
        for(i_rank =0 ; i_rank < i_cnt ; i_rank++ ) {
            rank=(*list_of_ranks_in_all_subgroups)[i_offset+i_rank];
            if(rank==root) {
                /* this is the root */
                (*array_of_all_subgroup_ranks)[i_sg].root_index=i_rank;
            }
        }
    }


    /*
     *  The data that is needed for a given rooted operation is:
     *    - subgroup,rank information for the source of the data.
     *      That is, which rank in the subgroup will recieve the
     *      data and distribute to the rest of the ranks.
     *    - the ranks that this data will be sent to.  This is
     *      described by the ranks in the current subgroups, and
     *      the subroups for which each rank is a proxy for,
     *      recursively in the communication tree.
     *
     *  The assumption is that data will be delived to each subgroup
     *    in an order, that is, all the data destined to subgroup rank 0
     *    will appear 1st, then that for rank 1, etc.  This implies that
     *    the data destined to rank 0, for example, will include the
     *    data for rank 0, as well as all the ranks that appear following
     *    it in the tree - in order.
     *
     *  Proxies: A rank may be a proxy for more than a single subgroup.
     *    When a rank is proxy for more than a single subgroup, we
     *    maintain a fixed order of subgroups for which this is a
     *    proxy, with an assumption that the data for the first subgroup
     *    appears first in the list, then that for the second, etc.
     *    Since the data for the proxy (which is a member of this subgroup)
     *    appears only once in the data list, the assumption is that the
     *    proxy will be the root for this operation, and it is the first
     *    set of data in the data list.  This means, that the data offset
     *    for the second ranks in each subgroup will include all the data
     *    for the previous subgroups, recursively.  This lets us maintain
     *    the simple addressing scheme of contigous data per rank in
     *    the subcommunicator.
     *
     *  The information needed for each rank in the subgroup are the
     *    group indicies for which it is a proxy.
     */
    /*
     * fill in the vertecies in the hierarchichal communications graph
     */

    /* figure out how detailed connection information, so that we can
     * can figure out how the data needs to be ordered for sending it
     * though the tree in various collective algorithms that have per-rank
     * data associated with them.
     */

    /* initialize the array */
    for(i_sg=0 ; i_sg < (*num_total_subgroups); i_sg++) {
        i_cnt=(*array_of_all_subgroup_ranks)[i_sg].n_ranks;
        (*array_of_all_subgroup_ranks)[i_sg].n_connected_nodes=0;
        for(i_rank =0 ; i_rank < i_cnt ; i_rank++ ) {
            (*array_of_all_subgroup_ranks)[i_sg].rank_data[i_rank].
                n_connected_subgroups=0;
        }
    }

    for(i_sg=(*num_total_subgroups)-1; i_sg >= 0 ; i_sg--) {
        i_cnt=(*array_of_all_subgroup_ranks)[i_sg].n_ranks;
        i_level=(*array_of_all_subgroup_ranks)[i_sg].level_in_hierarchy;
        i_offset=(*array_of_all_subgroup_ranks)[i_sg].index_of_first_element;
        for(i_rank =0 ; i_rank < i_cnt ; i_rank++ ) {
            rank=(*list_of_ranks_in_all_subgroups)[i_offset+i_rank];
            for(j_sg=i_sg-1; j_sg >= 0 ; j_sg--) {
                j_level=(*array_of_all_subgroup_ranks)[j_sg].level_in_hierarchy;
                j_root=(*array_of_all_subgroup_ranks)[j_sg].root_rank_in_comm;
                if(i_level == j_level ) {
                    /* no overlap ==> not connections between groups at the
                     * same level
                     */
                    continue;
                }
                if(rank == j_root ){
                    /* do not connect to i_sg, if there is already a connection
                     * to a subgroup with the same root higher up in the tree */
                    found=false;
                    for( k= i_sg-1 ; k > j_sg ; k-- ) {
                        if( rank ==
                                (*array_of_all_subgroup_ranks)[k].root_rank_in_comm ) {
                            found=true;
                            break;
                        }
                    }
                    if(found) {
                        /* the is not a vertex */
                        continue;
                    }
                    /* found vertex */
                    (*array_of_all_subgroup_ranks)[i_sg].n_connected_nodes++;
                    (*array_of_all_subgroup_ranks)[i_sg].rank_data[i_rank].
                        n_connected_subgroups++;

                    (*array_of_all_subgroup_ranks)[j_sg].n_connected_nodes++;
                    /* the connection "down" is to the local leader */
                    j_rank=(*array_of_all_subgroup_ranks)[j_sg].root_index;
                    (*array_of_all_subgroup_ranks)[j_sg].rank_data[j_rank].
                        n_connected_subgroups++;

                }
            }
        }
    }
    /* fill in connected nodes */
    /* allocate memory for lists */
    for(i_sg=0 ; i_sg < (*num_total_subgroups); i_sg++) {
        i_cnt=(*array_of_all_subgroup_ranks)[i_sg].n_connected_nodes;
        if( i_cnt > 0 ) {
            (*array_of_all_subgroup_ranks)[i_sg].list_connected_nodes=
                (int *)malloc(sizeof(int)*i_cnt);
            if (OPAL_UNLIKELY(NULL ==
                        (*array_of_all_subgroup_ranks)[i_sg].list_connected_nodes)) {
                ML_VERBOSE(10, ("Cannot allocate memory for list_connected_nodes - i_cnt %d\n",i_cnt));
                ret = OMPI_ERR_OUT_OF_RESOURCE;
                goto ERROR;
            }
        } else {
            (*array_of_all_subgroup_ranks)[i_sg].list_connected_nodes=NULL;
        }
        /* we will use this as a counter when we fill in the list of ranks */
        (*array_of_all_subgroup_ranks)[i_sg].n_connected_nodes=0;

        i_cnt=(*array_of_all_subgroup_ranks)[i_sg].n_ranks;
        i_offset=(*array_of_all_subgroup_ranks)[i_sg].index_of_first_element;
        for(i_rank =0 ; i_rank < i_cnt ; i_rank++ ) {
            cnt= (*array_of_all_subgroup_ranks)[i_sg].rank_data[i_rank].
                n_connected_subgroups;
            if( 0 == cnt) {
                /* no memory to allocate */
                (*array_of_all_subgroup_ranks)[i_sg].rank_data[i_rank].list_connected_subgroups=NULL;
                continue;
            }
            (*array_of_all_subgroup_ranks)[i_sg].rank_data[i_rank].list_connected_subgroups=
                (int *)malloc(sizeof(int)*cnt);
            if (OPAL_UNLIKELY(NULL ==
                        (*array_of_all_subgroup_ranks)[i_sg].rank_data[i_rank].list_connected_subgroups) ) {
                ML_VERBOSE(10, ("Cannot allocate memory for rank list_connected_subgroups - cnt %d\n",cnt));
                ret = OMPI_ERR_OUT_OF_RESOURCE;
                goto ERROR;
            }
            /* reset the conuter, so can fill it in on the fly */
            (*array_of_all_subgroup_ranks)[i_sg].rank_data[i_rank].
                n_connected_subgroups=0;
        }
    }

    /* fill in the list of connected nodes */
    for(i_sg=(*num_total_subgroups)-1; i_sg >= 0 ; i_sg--) {
        i_cnt=(*array_of_all_subgroup_ranks)[i_sg].n_ranks;
        i_level=(*array_of_all_subgroup_ranks)[i_sg].level_in_hierarchy;
        i_offset=(*array_of_all_subgroup_ranks)[i_sg].index_of_first_element;
        for(i_rank =0 ; i_rank < i_cnt ; i_rank++ ) {
            rank=(*list_of_ranks_in_all_subgroups)[i_offset+i_rank];
            for(j_sg=i_sg-1; j_sg >= 0 ; j_sg--) {
                j_level=(*array_of_all_subgroup_ranks)[j_sg].level_in_hierarchy;
                j_root=(*array_of_all_subgroup_ranks)[j_sg].root_rank_in_comm;
                if(i_level == j_level ) {
                    /* no overlap ==> not connections between groups at the
                     * same level
                     */
                    continue;
                }
                if(rank == j_root ){
                    /* do not connect to i_sg, if there is already a connection
                     * to a subgroup with the same root higher up in the tree */
                    found=false;
                    for( k= i_sg-1 ; k > j_sg ; k-- ) {
                        if( rank ==
                                (*array_of_all_subgroup_ranks)[k].root_rank_in_comm ) {
                            found=true;
                            break;
                        }
                    }
                    if(found) {
                        /* the is not a vertex */
                        continue;
                    }
                    /* found vertex */
                    /*
                     * connection "down"
                     */
                    cnt=(*array_of_all_subgroup_ranks)[i_sg].n_connected_nodes;
                    (*array_of_all_subgroup_ranks)[i_sg].list_connected_nodes[cnt]
                        =j_sg;
                    (*array_of_all_subgroup_ranks)[i_sg].n_connected_nodes++;

                    /* detailed per-rank information */
                    cnt=(*array_of_all_subgroup_ranks)[i_sg].rank_data[i_rank].
                        n_connected_subgroups;
                    (*array_of_all_subgroup_ranks)[i_sg].rank_data[i_rank].
                        list_connected_subgroups[cnt]=j_sg;
                    (*array_of_all_subgroup_ranks)[i_sg].rank_data[i_rank].
                        n_connected_subgroups++;

                    /* connection "up" */
                    cnt=(*array_of_all_subgroup_ranks)[j_sg].n_connected_nodes;
                    (*array_of_all_subgroup_ranks)[j_sg].list_connected_nodes[cnt]
                        =i_sg;
                    (*array_of_all_subgroup_ranks)[j_sg].n_connected_nodes++;

                    /* detailed per-rank information */
                    j_rank=(*array_of_all_subgroup_ranks)[j_sg].root_index;
                    cnt=(*array_of_all_subgroup_ranks)[j_sg].rank_data[j_rank].
                        n_connected_subgroups;
                    (*array_of_all_subgroup_ranks)[j_sg].rank_data[j_rank].
                        list_connected_subgroups[cnt]=i_sg;
                    (*array_of_all_subgroup_ranks)[j_sg].rank_data[j_rank].
                        n_connected_subgroups++;
                }
            }
        }
    }

    /* figure out the number of ranks that each rank in the subgroups
     * represnt.  scratch_space - is large enough for the scratch
     * space that we need.
     */
    for( i=0 ; i < (*num_total_subgroups) ; i++ ) {
        for(j=0 ; j < (*array_of_all_subgroup_ranks)[i].n_ranks ; j++ ) {
            scratch_space[0]=i;
            cnt=1;
            (*array_of_all_subgroup_ranks)[i].rank_data[j].num_of_ranks_represented=
                ml_compute_number_unique_proxy_ranks(i,j,
                        scratch_space,&cnt, *array_of_all_subgroup_ranks);
        }
    }

    /* compute the sort list when I am root */
    topo->sort_list=(int *)
        malloc(sizeof(int) * ompi_comm_size(comm));
    if (OPAL_UNLIKELY(NULL == topo->sort_list)) {
        ML_VERBOSE(10, ("Cannot allocate memory for sort_list.\n"));
        ret = OMPI_ERR_OUT_OF_RESOURCE;
        goto ERROR;
    }

    /* find subgroup index, and rank within that subgroup where I am
     * a leaf.
     */
    i_rank = -1;
    i_level = -1;
    found = false;
    for(i = 0; i < (*num_total_subgroups); i++ ) {
        for(j = 0; j < (*array_of_all_subgroup_ranks)[i].n_ranks; j++ ) {
            if((ompi_comm_rank(comm) ==
                        (*array_of_all_subgroup_ranks)[i].rank_data[j].rank)
                    &&
                    (*array_of_all_subgroup_ranks)[i].rank_data[j].leaf){
                found = true;
                /* rank */
                i_rank = j;
                /* subgroup index */
                i_level = i;
                break;
            }
        }
        if(found){
            break;
        }
    }
    assert(found);

    scratch_space[0] = i_level;
    cnt = 1;
    rank_cnt = 0;
    ml_compute_create_unique_proxy_rank_list(
        i_level, scratch_space, &cnt, *array_of_all_subgroup_ranks,
        &rank_cnt, topo->sort_list);

    /* return */
    if(scratch_space) {
        free(scratch_space);
    };
    scratch_space=NULL;

    return ret;

ERROR:
    if(scratch_space) {
        free(scratch_space);
    };
    scratch_space=NULL;

    for(i_sg=0 ; i_sg < (*num_total_subgroups)-1; i_sg++) {
        if((*array_of_all_subgroup_ranks)[i_sg].list_connected_nodes){
            free((*array_of_all_subgroup_ranks)[i_sg].list_connected_nodes);
            (*array_of_all_subgroup_ranks)[i_sg].list_connected_nodes=NULL;
        }
    }
    return ret;
}

static int get_new_subgroup_data (int32_t *all_selected, int size_of_all_selected,
        sub_group_params_t **sub_group_meta_data,
        int *size_of_sub_group_meta_data,
        int **list_of_ranks_in_all_subgroups,
        int *size_of_list_of_ranks_in_all_subgroups,
        int *num_ranks_in_list_of_ranks_in_all_subgroups,
        int *num_total_subgroups,
        int *map_to_comm_ranks, int level_in_hierarchy
         ) {

    /* local data */
    int rc=OMPI_SUCCESS;
    int rank_in_list,old_sg_size=(*num_total_subgroups);
    int sg_index, array_id, offset, sg_id;
    bool found_sg;
    sub_group_params_t *dummy1 = NULL;
    int32_t **dummy2 = NULL;
    int32_t *dummy3 = NULL;
    int32_t **temp = NULL;
    int knt1 = 0,
        knt2 = 0,
        knt3 = 0;

    /* loop over all elements in the array of ranks selected, looking for
     * newly selected ranks - these form the new subgroups */
    for(rank_in_list = 0 ; rank_in_list < size_of_all_selected ; rank_in_list++ ) {
        int sg_root, current_rank_in_comm;
        /* get root's rank in the communicator */
        sg_root=all_selected[rank_in_list];

        if( 0 == sg_root ) {
            /* this rank not selected - go to the next rank */
            continue;
        }

        if( sg_root < 0 ) {
            sg_root=-sg_root-1;
        } else {
            sg_root-=1;
        }

        current_rank_in_comm=map_to_comm_ranks[rank_in_list];

        /* loop over existing groups, and see if this is a member of a new group
         * or if this group has already been found.
         */
        found_sg=false;
        sg_id=-1;
        for( sg_index = old_sg_size ; sg_index < (*num_total_subgroups) ;
                sg_index++ ) {
            if( (*sub_group_meta_data)[sg_index].root_rank_in_comm ==
                    sg_root) {
                /* add rank to the list */
                (*sub_group_meta_data)[sg_index].n_ranks++;
                sg_id=sg_index;
                found_sg=true;
                break;
            }
        }
        if( !found_sg) {
            /* did not find existing sub-group, create new one */
            /* intialize new subgroup */
            PROVIDE_SUFFICIENT_MEMORY((*sub_group_meta_data), dummy1,
                    (*size_of_sub_group_meta_data),
                    sub_group_params_t, (*num_total_subgroups), 1, 5);
            /* do this for the temporary memory slots */
            PROVIDE_SUFFICIENT_MEMORY(temp, dummy2,
                    knt1, int32_t *, knt2, 1, 5);
            if (OPAL_UNLIKELY(NULL == (*sub_group_meta_data))) {
                ML_VERBOSE(10, ("Cannot allocate memory for sub_group_meta_data.\n"));
                rc = OMPI_ERR_OUT_OF_RESOURCE;
                goto ERROR;
            }
            (*sub_group_meta_data)[(*num_total_subgroups)].root_rank_in_comm=sg_root;
            (*sub_group_meta_data)[(*num_total_subgroups)].n_ranks=1;
            /* no need for this here - use a temporary ptr */
            temp[knt2]=
                (int *)malloc(sizeof(int)*size_of_all_selected);
            if (OPAL_UNLIKELY(NULL == temp[knt2] ) ){
                ML_VERBOSE(10, ("Cannot allocate memory for sub_group_meta_data.\n"));
                rc = OMPI_ERR_OUT_OF_RESOURCE;
                goto ERROR;
            }
            sg_id=(*num_total_subgroups);
            (*num_total_subgroups)++;
            knt2++;
            knt3 = knt2;
        } else {
            knt3 = sg_id - old_sg_size + 1;
        }
        array_id=(*sub_group_meta_data)[sg_id].n_ranks-1;
        temp[knt3-1][array_id] = current_rank_in_comm;
        /* JSL This fixes a nasty memory bug thay vexed us for 3 hours */
        /* XXX */

        /*
        (*sub_group_meta_data)[sg_id].list_ranks[array_id]=
            current_rank_in_comm;
        */
    }

    /* linearize the data - one rank will ship this to all the other
     * ranks the communicator
     */
    /* make sure there is enough memory to hold the list */
    PROVIDE_SUFFICIENT_MEMORY((*list_of_ranks_in_all_subgroups),dummy3,
            (*size_of_list_of_ranks_in_all_subgroups),
            int, (*num_ranks_in_list_of_ranks_in_all_subgroups),
            size_of_all_selected,size_of_all_selected);
    if (OPAL_UNLIKELY(NULL == (*list_of_ranks_in_all_subgroups))) {
        ML_VERBOSE(10, ("Cannot allocate memory for list_of_ranks_in_all_subgroups.\n"));
        rc = OMPI_ERR_OUT_OF_RESOURCE;
        goto ERROR;
    }

    /* loop over new subgroups */
    for( sg_id=old_sg_size ; sg_id < (*num_total_subgroups) ; sg_id++ ) {
        offset=(*num_ranks_in_list_of_ranks_in_all_subgroups);

        (*sub_group_meta_data)[sg_id].index_of_first_element=offset;

        for( array_id=0 ; array_id < (*sub_group_meta_data)[sg_id].n_ranks ;
                array_id++ ) {
            (*list_of_ranks_in_all_subgroups)[offset+array_id]=
                temp[sg_id-old_sg_size][array_id];
        }
        (*num_ranks_in_list_of_ranks_in_all_subgroups)+=
            (*sub_group_meta_data)[sg_id].n_ranks;
        (*sub_group_meta_data)[sg_id].level_in_hierarchy=level_in_hierarchy;
        /* this causes problems on XT5 starting at 6144 cores */
        free(temp[sg_id-old_sg_size]);
    }
    /* clean up temporary storage */
    if(NULL != temp) {
        free(temp);
        temp = NULL;
    }

    /* return */
    return rc;

ERROR:
    return rc;

}

static int append_new_network_context(hierarchy_pairs *pair)
{
    int i;
    int rc;
    mca_coll_ml_lmngr_t *memory_manager = &mca_coll_ml_component.memory_manager;
    bcol_base_network_context_t *nc = NULL;

    for (i = 0; i < pair->num_bcol_modules; i++) {
        nc = pair->bcol_modules[i]->network_context;
        if (NULL != nc) {
            rc = mca_coll_ml_lmngr_append_nc(memory_manager, nc);
            if (OMPI_SUCCESS != rc) {
                return OMPI_ERROR;
            }
            /* caching the network context id on bcol */
            pair->bcol_modules[i]->context_index = nc->context_id;
        }
    }

    return OMPI_SUCCESS;
}

static int ml_module_set_small_msg_thresholds(mca_coll_ml_module_t *ml_module)
{
    const mca_coll_ml_topology_t *topo_info;
    mca_bcol_base_module_t *bcol_module;
    hierarchy_pairs *pair;

    int i, j, rc, hier, *ranks_in_comm, n_hier, tp,
        comm_size = ompi_comm_size(ml_module->comm);

    for (tp = 0; tp < COLL_ML_TOPO_MAX; ++tp) {
        topo_info = &ml_module->topo_list[tp];
        if (COLL_ML_TOPO_DISABLED == topo_info->status) {
            /* Skip the topology */
            continue;
        }

        n_hier = topo_info->n_levels;
        for (hier = 0; hier < n_hier; ++hier) {
            pair = &topo_info->component_pairs[hier];

            for (i = 0; i < pair->num_bcol_modules; ++i) {
                bcol_module = pair->bcol_modules[i];

                if (NULL != bcol_module->set_small_msg_thresholds) {
                    bcol_module->set_small_msg_thresholds(bcol_module);
                }

                for (j = 0; j < BCOL_NUM_OF_FUNCTIONS; ++j) {
                    if (ml_module->small_message_thresholds[j] >
                            bcol_module->small_message_thresholds[j]) {
                        ml_module->small_message_thresholds[j] =
                                        bcol_module->small_message_thresholds[j];
                    }
                }
            }

        }
    }

    ranks_in_comm = (int *) malloc(comm_size * sizeof(int));
    if (OPAL_UNLIKELY(NULL == ranks_in_comm)) {
        ML_ERROR(("Memory allocation failed."));
        return OMPI_ERROR;
    }

    for (i = 0; i < comm_size; ++i) {
        ranks_in_comm[i] = i;
    }

    rc = comm_allreduce_pml(ml_module->small_message_thresholds,
                            ml_module->small_message_thresholds,
                            BCOL_NUM_OF_FUNCTIONS, MPI_INT,
                            ompi_comm_rank(ml_module->comm), MPI_MIN,
                            comm_size, ranks_in_comm, ml_module->comm);

    if (OPAL_UNLIKELY(OMPI_SUCCESS != rc)) {
        ML_ERROR(("comm_allreduce_pml failed."));
        return OMPI_ERROR;
    }

    free(ranks_in_comm);

    return OMPI_SUCCESS;
}

static int mca_coll_ml_read_allbcols_settings(mca_coll_ml_module_t *ml_module,
        int n_hierarchies)
{
    int i, j,
        ret = OMPI_SUCCESS;
    int *ranks_map = NULL,
        *bcols_in_use = NULL,
        *bcols_in_use_all_ranks = NULL;
    bool use_user_bufs, limit_size_user_bufs;
    ssize_t length_ml_payload;
    int64_t frag_size;
    const mca_bcol_base_component_2_0_0_t *bcol_component = NULL;
    mca_base_component_list_item_t *bcol_cli = NULL;

    /* If this assert fails, it means that you changed initialization
     * order and the date offset , that is critical for this section of code,
     * have not been initilized.
     * DO NOT REMOVE THIS ASSERT !!!
     */
    assert(ml_module->data_offset >= 0);

    /* need to figure out which bcol's are participating
     * in the hierarchy across the communicator, so that we can set
     * appropriate segmantation parameters.
     */
    bcols_in_use = (int *) malloc(sizeof(int) * 2 * n_hierarchies);
    if (OPAL_UNLIKELY(NULL == bcols_in_use)) {
        ML_VERBOSE(10, ("Cannot allocate memory for bcols_in_use.\n"));
        ret = OMPI_ERR_OUT_OF_RESOURCE;
        goto ERROR;
    }
    /* setup pointers to arrays that will hold bcol parameters.  Since
     * given bols are not instantiated in all processes, need to get this
     * information from those ranks that have instantiated these
     * parameters
     */
    bcols_in_use_all_ranks = bcols_in_use+n_hierarchies;

    /* get list of bcols that I am using */
    for(i = 0; i < n_hierarchies; i++) {
        bcols_in_use[i] = 0;
    }

    for (j = 0; j < COLL_ML_TOPO_MAX; j++) {
        mca_coll_ml_topology_t *topo_info = &ml_module->topo_list[j];
        if (COLL_ML_TOPO_DISABLED == topo_info->status) {
            /* skip the topology */
            continue;
        }

        for(i = 0; i < topo_info->n_levels; i++ ) {
            int ind;
            ind = topo_info->component_pairs[i].bcol_index;
            bcols_in_use[ind] = 1;
        }
    }

    /* set one to one mapping */
    ranks_map = (int *) malloc(sizeof(int) * ompi_comm_size(ml_module->comm));
    if (NULL == ranks_map) {
        ret = OMPI_ERR_OUT_OF_RESOURCE;
        goto ERROR;
    }
    for (i = 0; i < ompi_comm_size(ml_module->comm); i++) {
        ranks_map[i] = i;
    }

    /* reduce over all the ranks to figure out which bcols are
     * participating at this level
     */
    ret = comm_allreduce_pml(bcols_in_use, bcols_in_use_all_ranks,
            n_hierarchies, MPI_INT, ompi_comm_rank(ml_module->comm),
            MPI_MAX, ompi_comm_size(ml_module->comm),
            ranks_map, ml_module->comm);
    if (OPAL_UNLIKELY(OMPI_SUCCESS != ret)) {
        ML_VERBOSE(10, ("comm_allreduce_pml failed. bcols_in_use reduction\n"));
        goto ERROR;
    }

    /*
     * figure out fragmenation parameters
     */
    /* can user buffers be used */
    use_user_bufs = true;
    bcol_cli = (mca_base_component_list_item_t *) opal_list_get_first(&mca_bcol_base_components_in_use);

    for (i = 0; i < n_hierarchies; i++) {
        if(!bcols_in_use_all_ranks) {
            /* this bcol is not being used - do nothing */
            continue;
        }
        bcol_component = (mca_bcol_base_component_2_0_0_t *) bcol_cli->cli_component;
        /* check to see if user buffers can be used */
        if(!bcol_component->can_use_user_buffers) {
            /* need to use library buffers, so all will do this */
            use_user_bufs = false;
            break;
        }

        bcol_cli = (mca_base_component_list_item_t *) opal_list_get_next((opal_list_item_t *) bcol_cli);
    }

    /* size of ml buffer */
    length_ml_payload = mca_coll_ml_component.payload_buffer_size - ml_module->data_offset;

    if (use_user_bufs) {
        /*
         * using user buffers
         */
        ml_module->use_user_buffers = 1;

        /* figure out if data will be segmented for pipelining -
         * for non-contigous data will just use a fragment the size
         * of the ml payload buffer */

        /* check to see if any bcols impose a limit */
        limit_size_user_bufs = false;
        bcol_cli = (mca_base_component_list_item_t *)opal_list_get_first(&mca_bcol_base_components_in_use);
        for (i = 0; i < n_hierarchies; i++) {
            bcol_component = (mca_bcol_base_component_2_0_0_t *) bcol_cli->cli_component;
            if(bcol_component->max_frag_size != FRAG_SIZE_NO_LIMIT ){
                limit_size_user_bufs = true;
                break;
            }
            bcol_cli = (mca_base_component_list_item_t *) opal_list_get_next((opal_list_item_t *) bcol_cli);
        }

        if (limit_size_user_bufs) {
            /* figure out fragement size */
            frag_size = 0;
            bcol_cli = (mca_base_component_list_item_t *) opal_list_get_first(&mca_bcol_base_components_in_use);
            for (i = 0; i < n_hierarchies; i++) {
                /* check to see if this bcol is being used */
                if(!bcols_in_use_all_ranks[i]) {
                    /* not in use */
                    continue;
                }
                bcol_component = (mca_bcol_base_component_2_0_0_t *) bcol_cli->cli_component;
                if (FRAG_SIZE_NO_LIMIT == bcol_component->max_frag_size) {
                    /* no limit - will not determine fragement size */
                    continue;
                }
                if (0 != bcol_component->max_frag_size) {
                    /* nothing set yet */
                    frag_size = bcol_component->max_frag_size;
                } else {
                    if(frag_size < bcol_component->max_frag_size) {
                        /* stricter constraint on fragment size */
                        frag_size = bcol_component->max_frag_size;
                    }
                }
                bcol_cli = (mca_base_component_list_item_t *)opal_list_get_next((opal_list_item_t *)bcol_cli);
            }
            ml_module->fragment_size = frag_size;
        } else {
            /* entire message may be processed in single chunk */
            ml_module->fragment_size = FRAG_SIZE_NO_LIMIT;
        }
        /* for non-contigous data - just use the ML buffers */
        ml_module->ml_fragment_size = length_ml_payload;

    } else {
        /*
         * using library buffers
         */
        ml_module->use_user_buffers = 0;

        /* figure out buffer size */
        ml_module->fragment_size = length_ml_payload;
        /* see if this is too large */
        bcol_cli = (mca_base_component_list_item_t *) opal_list_get_first(&mca_bcol_base_components_in_use);
        for (i = 0; i < n_hierarchies; i++) {
            /* check to see if this bcol is being used */
            if(!bcols_in_use_all_ranks[i]) {
                /* not in use */
                continue;
            }
            bcol_component = (mca_bcol_base_component_2_0_0_t *) bcol_cli->cli_component;
            bcol_cli = (mca_base_component_list_item_t *) opal_list_get_next((opal_list_item_t *) bcol_cli);
            if (FRAG_SIZE_NO_LIMIT == bcol_component->max_frag_size) {
                /* no limit - will not affect fragement size */
                continue;
            }
            if (bcol_component->max_frag_size < (int)ml_module->fragment_size)
            {
                /* frag size set too large */
                ml_module->fragment_size = bcol_component->max_frag_size;
            }
        }
        /* for non-contigous data - just use the ML buffers */
        ml_module->ml_fragment_size = ml_module->fragment_size;
    }

    ML_VERBOSE(10, ("Seting payload size to %d %d [%d %d]",
                     ml_module->ml_fragment_size, length_ml_payload,
                     mca_coll_ml_component.payload_buffer_size,
                     ml_module->data_offset));

ERROR:
    if (NULL != ranks_map) {
        free(ranks_map);
    }
    if (NULL != bcols_in_use) {
        free(bcols_in_use);
    }

    return ret;
}

static int ml_discover_hierarchy(mca_coll_ml_module_t *ml_module)
{
    ompi_proc_t *my_proc = NULL;

    int n_hierarchies = 0,
        i = 0, ret = OMPI_SUCCESS;

    int size_bcol_list, size_sbgp_list;

    size_bcol_list = opal_list_get_size(&mca_bcol_base_components_in_use);
    size_sbgp_list = opal_list_get_size(&mca_sbgp_base_components_in_use);

    if ((size_bcol_list != size_sbgp_list) || size_sbgp_list < 1 || size_bcol_list < 1) {
        ML_ERROR(("Error: (size of mca_bcol_base_components_in_use = %d)"
                       " != (size of mca_sbgp_base_components_in_use = %d) or zero.\n",
                size_bcol_list, size_sbgp_list));
        return OMPI_ERROR;
    }

    n_hierarchies = size_sbgp_list;

    my_proc = ompi_proc_local();
    /* create the converter, for current implementation we
       support homogenius comunicators only */
    ml_module->reference_convertor =
        opal_convertor_create(my_proc->proc_arch, 0);

    if (OPAL_UNLIKELY(NULL == ml_module->reference_convertor)) {
        return OMPI_ERROR;
    }

    /* Do loop over all supported hiearchies.
       To Do. We would like to have mca parameter that will allow control list
       of topolgies that user would like use. Right now we will run
     */
    for (i = 0; i < COLL_ML_TOPO_MAX; i++) {
        if (COLL_ML_TOPO_ENABLED == ml_module->topo_list[i].status) {
            ret = mca_coll_ml_component.topo_discovery_fn[i](ml_module, n_hierarchies);
            if (OPAL_UNLIKELY(OMPI_SUCCESS != ret)) {
                return ret;
            }
        }
    }

    /* Local query for bcol header size */
    ret = calculate_buffer_header_size(ml_module);
    if (OPAL_UNLIKELY(OMPI_SUCCESS != ret)) {
        return ret;
    }

    /* Get BCOL tuning, like support for zero copy, fragment size, and etc.
     * This query involves global synchronization over all processes */
    ret = mca_coll_ml_read_allbcols_settings(ml_module, n_hierarchies);
    if (OPAL_UNLIKELY(OMPI_SUCCESS != ret)) {
        return ret;
    }
    /* Here is the safe point to call ml_module_memory_initialization , please
       be very careful,if you decide to move this arround.*/
    ret = ml_module_memory_initialization(ml_module);
    if (OPAL_UNLIKELY(OMPI_SUCCESS != ret)) {
        /* make sure to release just allocated memory */
        mca_coll_ml_free_block(ml_module->payload_block);
        return ret;
    }

    ret = ml_module_set_small_msg_thresholds(ml_module);
    if (OPAL_UNLIKELY(OMPI_SUCCESS != ret)) {
        /* make sure to release just allocated memory */
        mca_coll_ml_free_block(ml_module->payload_block);
        return ret;
    }

    {
        /* Syncronization barrier to make sure that all sides finsihed
         * to register the memory */
        int ret, i;
        int *comm_ranks = NULL;

        comm_ranks = (int *)calloc(ompi_comm_size(ml_module->comm), sizeof(int));
        if (OPAL_UNLIKELY(NULL == comm_ranks)) {
            ML_VERBOSE(10, ("Cannot allocate memory.\n"));
            return OMPI_ERR_OUT_OF_RESOURCE;
        }

        for (i = 0; i < ompi_comm_size(ml_module->comm); i++) {
            comm_ranks[i] = i;
        }

        ret = comm_allreduce_pml(&ret, &i,
                1, MPI_INT, ompi_comm_rank(ml_module->comm),
                MPI_MIN, ompi_comm_size(ml_module->comm), comm_ranks,
                ml_module->comm);

        if (OMPI_SUCCESS != ret) {
            ML_ERROR(("comm_allreduce - failed to collect max_comm data"));
            return ret;
        }
        /* Barrier done */
    }

    return ret;
}

static int mca_coll_ml_tree_hierarchy_discovery(mca_coll_ml_module_t *ml_module,
        mca_coll_ml_topology_t *topo, int n_hierarchies,
        const char *exclude_sbgp_name, const char *include_sbgp_name)
{
    /* local variables */
    char *ptr_output = NULL;
    sbgp_base_component_keyval_t   *sbgp_cli = NULL;
    mca_base_component_list_item_t *bcol_cli = NULL;
    hierarchy_pairs *pair = NULL;

    mca_sbgp_base_module_t *module = NULL;
    ompi_proc_t **procs = NULL,
                **copy_procs = NULL,
                *my_proc = NULL;

    const mca_sbgp_base_component_2_0_0_t *sbgp_component = NULL;
    const mca_bcol_base_component_2_0_0_t *bcol_component = NULL;


    int i_hier = 0, n_hier = 0, ll_p1,
        n_procs_in = 0, group_index = 0, n_remain = 0,
        i, j, ret = OMPI_SUCCESS, my_rank_in_list = 0,
        n_procs_selected = 0, original_group_size = 0, i_am_done = 0,
        local_leader, my_rank_in_subgroup, my_rank_in_remaining_list = 0;

    int32_t my_lowest_group_index = -1, my_highest_group_index = -1;

    int *map_to_comm_ranks = NULL, *bcols_in_use = NULL;

    int32_t *all_selected = NULL,
             *index_proc_selected = NULL;

    short all_reduce_buffer2_in[2];
    short all_reduce_buffer2_out[2];
    sub_group_params_t *array_of_all_subgroup_ranks=NULL;
    /* this pointer should probably be an int32_t and not an int type */
    int32_t *list_of_ranks_in_all_subgroups=NULL;
    int cum_number_ranks_in_all_subgroups=0,num_total_subgroups=0;
    int size_of_array_of_all_subgroup_ranks=0;
    int size_of_list_of_ranks_in_all_subgroups=0;
    int32_t in_allgather_value;

    if (NULL != exclude_sbgp_name && NULL != include_sbgp_name) {
        ret = OMPI_ERROR;
        goto ERROR;
    }

    ML_VERBOSE(10,("include %s exclude %s size %d", include_sbgp_name, exclude_sbgp_name, n_hierarchies));

    /* allocates scratch space */
    all_selected = (int32_t *) calloc(ompi_comm_size(ml_module->comm), sizeof(int32_t));
    if (OPAL_UNLIKELY(NULL == all_selected)) {
        ML_VERBOSE(10, ("Cannot allocate memory.\n"));
        ret = OMPI_ERR_OUT_OF_RESOURCE;
        goto ERROR;
    }

    map_to_comm_ranks = (int *) calloc(ompi_comm_size(ml_module->comm), sizeof(int));
    if (OPAL_UNLIKELY(NULL == map_to_comm_ranks)) {
        ML_VERBOSE(10, ("Cannot allocate memory.\n"));
        ret = OMPI_ERR_OUT_OF_RESOURCE;
        goto ERROR;
    }

    /*
    ** obtain list of procs
    */
    procs = ml_module->comm->c_local_group->grp_proc_pointers;

    /* create private copy for manipulation */
    copy_procs = (ompi_proc_t **) calloc(ompi_comm_size(ml_module->comm),
                                                    sizeof(ompi_proc_t *));
    if (OPAL_UNLIKELY(NULL == copy_procs)) {
        ML_VERBOSE(10, ("Cannot allocate memory.\n"));
        ret = OMPI_ERR_OUT_OF_RESOURCE;
        goto ERROR;
    }

    for (i = 0; i < ompi_comm_size(ml_module->comm); i++) {
        copy_procs[i] = procs[i];
        map_to_comm_ranks[i] = i;
    }

    n_procs_in = ompi_comm_size(ml_module->comm);
    original_group_size = n_procs_in;

    /* setup information for all-reduce over out of band */
    index_proc_selected = (int32_t *) malloc(sizeof(int32_t) * n_procs_in);
    if (OPAL_UNLIKELY(NULL == index_proc_selected)) {
        ML_VERBOSE(10, ("Cannot allocate memory.\n"));
        ret = OMPI_ERR_OUT_OF_RESOURCE;
        goto ERROR;
    }

    /* get my proc pointer - used to identify myself in the list */
    my_proc = ompi_proc_local();
    my_rank_in_list = ompi_comm_rank(ml_module->comm);

    topo->component_pairs = (hierarchy_pairs *) calloc(n_hierarchies, sizeof(hierarchy_pairs));
    if (OPAL_UNLIKELY(NULL == topo->component_pairs)) {
        ML_VERBOSE(10, ("Cannot allocate memory.\n"));
        ret = OMPI_ERR_OUT_OF_RESOURCE;
        goto ERROR;
    }

    n_hier = 0;
    /*
     * Algorithm for subgrouping:
     *  1) Start with all the ranks in the communicator
     *  2) iterate over all (exclusive) hierarchy selection rules
     *     A) Apply subgrouping function to the remaining set of ranks
     *       - After the call to subgrouping subgroup_module->group_list
     *         has the index of ranks selected, from the list or ranks
     *         passed in.
     *       - map_to_comm_ranks maintains the mapping of the remaining
     *         ranks, to their rank in the communicator
     *     B) Each rank initializes a scratch array the size of the
     *        remaining ranks to 0, and then fills in the entry that
     *        corresponds to itself only with the value -/+R.  If the
     *        rank is the local leader for the subgroup, the value of -R
     *        is entered, other wise R is entered.  R is the root of the
     *        selected subgroup plus 1, so that for rank 0, +R has a
     *        different value than -R.
     *     C) The vector is then reduced, with the results going to all
     *        ranks, over the list of remaining ranks.  As a result,
     *        the ranks of a given subgroup will show up with the value R,
     *        for all but the local-leader, which will have the value of -R.
     *        This is also used for error checking.
     *     D) subgroup_module->group_list is changed to contain the ranks
     *        of each member of the group within the communicator.
     *     E) Local rank with the group is determined.
     *     F) the list or remaining ranks is compacted, removing all selected
     *        ranks that are not the local-leader of the group.
     *        map_to_comm_ranks is also compacted.
     *  3) This is terminated once all ranks are selected.
     */

    /* loop over hierarchies */
    sbgp_cli = (sbgp_base_component_keyval_t *) opal_list_get_first(&mca_sbgp_base_components_in_use);
    bcol_cli = (mca_base_component_list_item_t *) opal_list_get_first(&mca_bcol_base_components_in_use);

    ML_VERBOSE(10, ("Loop over hierarchies.\n"));

    i_hier = 0;
    while ((opal_list_item_t *) sbgp_cli != opal_list_get_end(&mca_sbgp_base_components_in_use)){

        /*
         ** obtain the list of  ranks in the current level
         */

        sbgp_component = (mca_sbgp_base_component_2_0_0_t *) sbgp_cli->component.cli_component;
        bcol_component = (mca_bcol_base_component_2_0_0_t *) bcol_cli->cli_component;

        /* Skip excluded levels */
        if (NULL != exclude_sbgp_name) {
            
            ML_VERBOSE(10,("EXCLUDE compare %s to %s", include_sbgp_name,
                       sbgp_component->sbgp_version.mca_component_name));
            if(0 == strcmp(exclude_sbgp_name,
                        sbgp_component->sbgp_version.mca_component_name)) {
                /* take the next element */
                sbgp_cli = (sbgp_base_component_keyval_t *) opal_list_get_next((opal_list_item_t *) sbgp_cli);
                bcol_cli = (mca_base_component_list_item_t *) opal_list_get_next((opal_list_item_t *) bcol_cli);
                continue;
            }
        }

        if (NULL != include_sbgp_name) {
            ML_VERBOSE(10,("INCLUDE compare %s to %s", include_sbgp_name,
                       sbgp_component->sbgp_version.mca_component_name));
            if(0 != strcmp(include_sbgp_name,
                        sbgp_component->sbgp_version.mca_component_name)) {
                /* take the next element */
                sbgp_cli = (sbgp_base_component_keyval_t *) opal_list_get_next((opal_list_item_t *) sbgp_cli);
                bcol_cli = (mca_base_component_list_item_t *) opal_list_get_next((opal_list_item_t *) bcol_cli);
                continue;
            }
        }

        ML_VERBOSE(10,("Passed include %s exclude %s", include_sbgp_name, exclude_sbgp_name));

        /* discover subgroup */
        ML_VERBOSE(10, ("Discover subgroup: hier level - %d.\n", i_hier));
        module = sbgp_component->select_procs(copy_procs, n_procs_in,
                ml_module->comm,
                sbgp_cli->key_value, &ptr_output);
        if (NULL == module) {
            /* no module created */
            n_procs_selected = 0;
            /* We must continue and participate in the allgather. 
             * It's not clear that one can enter this conditional 
             * during "normal" execution. We need to review 
             * all modules.  
             */  

            /* THE CODE SNIPPET COMMENTED OUT BELOW IS DANGEROUS CODE THAT 
             * COULD RESULT IN A HANG - THE "CONTINUE" STATEMENT MAY RESULT IN 
             * RANKS BYPASSING THE ALLGATHER IN NON-SYMMETRIC CASES
             */

            /*
            sbgp_cli = (sbgp_base_component_keyval_t *) opal_list_get_next((opal_list_item_t *) sbgp_cli);
            bcol_cli = (mca_base_component_list_item_t *) opal_list_get_next((opal_list_item_t *) bcol_cli);
            continue;
            */
        } else if (
                (1 == module->group_size) && ( module->group_size != n_procs_in) )
        {
            /* we bypass groups of lenth 1, unless those are the only ones
             * remaining */
            n_procs_selected = 0;
            OBJ_RELEASE(module);
            module=NULL;
        } else {
            n_procs_selected = module->group_size;
        }

        ML_VERBOSE(10, ("Hier level - %d; group size - %d\n", i_hier, n_procs_selected));

        /* setup array indicating all procs that were selected */
        for (i = 0; i < n_procs_in; i++) {
            index_proc_selected[i] = 0;
        }

        /* figure out my rank in the subgroup */
        my_rank_in_subgroup=-1;
        ll_p1=-1;
        in_allgather_value = 0;
        if( n_procs_selected) {
            /* I need to contribute to the vector */
            for (group_index = 0; group_index < n_procs_selected; group_index++) {
                /* set my rank within the group */
                if (map_to_comm_ranks[module->group_list[group_index]] ==
                        ompi_comm_rank(ml_module->comm)) {
                    my_rank_in_subgroup=group_index;
                    module->my_index = group_index;
                    /* currently the indecies are still given in terms of
                     * the rank in the list of remaining ranks */
                    my_rank_in_remaining_list=module->group_list[group_index];
                }
            }

            if( -1 != my_rank_in_subgroup ) {
                /* I am contributing to this subgroup */

#ifdef NEW_LEADER_SELECTION
                int lleader_index;
                /* Select the local leader */
                lleader_index = coll_ml_select_leader(ml_module,module, map_to_comm_ranks,
                        copy_procs,n_procs_selected);

                local_leader = module->group_list[lleader_index];

#else

                /* local leader is rank within list or remaining ranks */
                local_leader=module->group_list[0];

#endif
                ML_VERBOSE(10,("The local leader selected for hierarchy %d is %d \n",
                            i_hier, local_leader));

                if(local_leader == my_rank_in_remaining_list ) {

                    /* transform to rank within the communicator */
                    local_leader=map_to_comm_ranks[local_leader];
                    ll_p1=local_leader+1;
                    in_allgather_value =
                        index_proc_selected[my_rank_in_remaining_list] = -ll_p1;
                } else {
                    /* transform to rank within the communicator */
                    local_leader=map_to_comm_ranks[local_leader];
                    ll_p1=local_leader+1;
                    in_allgather_value =
                        index_proc_selected[my_rank_in_remaining_list] = ll_p1;
                }
            }
        }

        /* gather the information from all the other remaining ranks */
        ML_VERBOSE(10, ("Call for comm_allreduce_pml.\n"));
        ret = comm_allgather_pml(&in_allgather_value,
                all_selected, 1, MPI_INT, my_rank_in_list,
                n_procs_in, map_to_comm_ranks ,ml_module->comm);
        if (OPAL_UNLIKELY(OMPI_SUCCESS != ret)) {
            ML_VERBOSE(10, ("comm_allreduce_pml failed.\n"));
            goto ERROR;
        }

        /* do some sanity checks */
        if( -1 != my_rank_in_subgroup ) {
            ret = check_global_view_of_subgroups(n_procs_selected,
                    n_procs_in, ll_p1, all_selected, module );
            if (OPAL_UNLIKELY(OMPI_SUCCESS != ret)) {
                ML_VERBOSE(10, ("check_global_view_of_subgroups failed.\n"));
                goto ERROR;
            }
        }

        /*
         ** change the list of procs stored on the module to ranks within
         ** the communicator.
         */

        ML_VERBOSE(10, ("Change the list of procs; hier level - %d.\n", i_hier));
        for (group_index = 0; group_index < n_procs_selected; group_index++) {
            module->group_list[group_index] = map_to_comm_ranks[module->group_list[group_index]];
            /* set my rank within the group */
            if (module->group_list[group_index] == ompi_comm_rank(ml_module->comm)) {
                module->my_index = group_index;
            }
        }

        /*
         * accumulate data on the new subgroups created
         */
        /*XXX*/
        ret = get_new_subgroup_data(all_selected, n_procs_in,
                &array_of_all_subgroup_ranks,
                &size_of_array_of_all_subgroup_ranks,
                &list_of_ranks_in_all_subgroups,
                &size_of_list_of_ranks_in_all_subgroups,
                &cum_number_ranks_in_all_subgroups,
                &num_total_subgroups, map_to_comm_ranks,i_hier);

        if( OMPI_SUCCESS != ret ) {
            ML_VERBOSE(10, (" Error: get_new_subgroup_data returned %d \n",ret));
            goto ERROR;
        }

        /* am I done ? */
        i_am_done=0;
        if ( (all_selected[my_rank_in_list] == ll_p1) &&
                /* if I was not a member of any group, still need to continue */
                n_procs_selected ){
            i_am_done = 1;
        }
        /* get my rank in the list */
        n_remain = 0;
        my_rank_in_list = -1;
        for (i = 0; i < n_procs_in; i++) {
            if (all_selected[i] > 0 ) {
                /* this proc will not be used in the next hierarchy */
                continue;
            }
            /* reset my_rank_in_list, n_procs_in */
            copy_procs[n_remain] = copy_procs[i];
            map_to_comm_ranks[n_remain] = map_to_comm_ranks[i];

            if (my_proc == copy_procs[n_remain]){
                my_rank_in_list = n_remain;
            }

            n_remain++;
        }

        /* check to make sure we did not get a size 1 group if more than
         * one rank are still remaning to be grouped */
        if ((1 == n_procs_selected) && n_remain > 1) {
            OBJ_RELEASE(module);
            n_procs_selected = 0;

        }

        if( 0 < n_procs_selected ) {
            /* increment the level counter */
            pair = &topo->component_pairs[n_hier];

            /* add this to the list of sub-group/bcol pairs in use */
            pair->subgroup_module = module;
            pair->bcol_component = (mca_bcol_base_component_t *)
                ((mca_base_component_list_item_t *) bcol_cli)->cli_component;

            pair->bcol_index = i_hier;

            /* create bcol modules */
            ML_VERBOSE(10, ("Create bcol modules.\n"));
            pair->bcol_modules = pair->bcol_component->collm_comm_query(
                    module, &pair->num_bcol_modules);
            /* failed to create a new module */
            if (OPAL_UNLIKELY(NULL == pair->bcol_modules)) {
                ML_VERBOSE(10, ("Failed to create new modules.\n"));
                ret = OMPI_ERROR;
                goto ERROR;
            }

            if (pair->bcol_component->need_ordering) {
                topo->topo_ordering_info.num_bcols_need_ordering += pair->num_bcol_modules;
            }

            /* Append new network contexts to our memory managment */
            ML_VERBOSE(10, ("Append new network contexts to our memory managment.\n"));
            if (OPAL_UNLIKELY(OMPI_SUCCESS != append_new_network_context(pair))) {
                ML_VERBOSE(10, ("Exit with error. - append new network context\n"));
                ret = OMPI_ERROR;
                goto ERROR;
            }

            for (i = 0; i < pair->num_bcol_modules; ++i) {
                /* set the starting sequence number */
                pair->bcol_modules[i]->squence_number_offset =
                    mca_coll_ml_component.base_sequence_number;

                /* cache the sub-group size */
                pair->bcol_modules[i]->size_of_subgroup=
                    module->group_size;

                /* set the bcol id */
                pair->bcol_modules[i]->bcol_id = (int16_t) i_hier;
            }

            /*
             * set largest power of 2 for this group
             */
            module->n_levels_pow2 = ml_fls(module->group_size);
            module->pow_2 = 1 << module->n_levels_pow2;

            n_hier++;

            if (-1 == my_lowest_group_index) {
                my_lowest_group_index = i_hier;
            }

            my_highest_group_index = i_hier;
        }

        /* if n_remain is 1, and the communicator size is not 1, and module
         ** is not NULL, I am done
         */
        if ((1 == n_remain) && (1 < original_group_size) &&
                (NULL != module)) {
            i_am_done = 1;
        }

        n_procs_in = n_remain;

        /* am I done ? */
        if (1 == i_am_done) {
            /* nothing more to do */
            goto SelectionDone;
        }

        /* take the next element */
        sbgp_cli = (sbgp_base_component_keyval_t *) opal_list_get_next((opal_list_item_t *) sbgp_cli);
        bcol_cli = (mca_base_component_list_item_t *) opal_list_get_next((opal_list_item_t *) bcol_cli);
        i_hier++;
    }

    SelectionDone:

    if (topo->topo_ordering_info.num_bcols_need_ordering > 0) {
        for (j = 0; j < n_hier; ++j) {
            pair = &topo->component_pairs[j];
            if (pair->bcol_component->need_ordering) {
                for (i = 0; i < pair->num_bcol_modules; ++i) {
                    pair->bcol_modules[i]->next_inorder = &topo->topo_ordering_info.next_inorder;
                }
            }
        }
    }

    /*
     * The memory allocation in this debug code is broken,
     * it is why we keep it disabled by default even for debug mode,
     * but it is good to have this information
     */
#if (OPAL_ENABLE_DEBUG)
#define COLL_ML_HIER_BUFF_SIZE (1024*1024)
        {
            int ii, jj;
            char buff[COLL_ML_HIER_BUFF_SIZE];
            char *output = buff;

            memset(buff, 0, COLL_ML_HIER_BUFF_SIZE);
            for (ii = 0; ii < n_hier; ++ii) {
                module = topo->component_pairs[ii].subgroup_module;
                if (NULL != module) {
                    sprintf(output, "\nsbgp num %d, num of bcol modules %d, my rank in this comm %d, ranks: ",
                              ii + 1, topo->component_pairs[ii].num_bcol_modules, ompi_comm_rank(ml_module->comm));

                    output = buff + strlen(buff);
                    assert(COLL_ML_HIER_BUFF_SIZE + buff > output);

                    for(jj = 0; jj < module->group_size; ++jj) {
                        sprintf(output, " %d", module->group_list[jj]);

                        output = buff + strlen(buff);
                        assert(COLL_ML_HIER_BUFF_SIZE + buff > output);
                    }

                    sprintf(output, "\nbcol modules: ");

                    output = buff + strlen(buff);
                    assert(COLL_ML_HIER_BUFF_SIZE + buff > output);

                    for(jj = 0; jj < topo->component_pairs[ii].num_bcol_modules; ++jj) {
                        sprintf(output, " %p", (void *)topo->component_pairs[ii].bcol_modules[jj]);

                        output = buff + strlen(buff);
                        assert(COLL_ML_HIER_BUFF_SIZE + buff > output);
                    }

                } else {
                    sprintf(output, "\nsbgp num %d, sbgp module is NULL", ii + 1);

                    output = buff + strlen(buff);
                    assert(COLL_ML_HIER_BUFF_SIZE + buff > output);
                }
            }

            ML_VERBOSE(10, ("\nn_hier = %d\ncommunicator %p, ML module %p%s.\n",
                                      n_hier, ml_module->comm, ml_module, buff));
        }
#endif
        /* If I was not done, it means that we skipped all subgroups and no hierarchy was build */
        if (0 == i_am_done) {
            if (NULL != include_sbgp_name || NULL != exclude_sbgp_name) {
                /* User explicitly asked for specific type of topology, which generates empty group */
                ML_ERROR(("ML topology configuration explicitly requested to %s subgroup %s. "
                           "Such configuration results in a creation of empty groups. As a result, ML framework can't "
                           "configure requested collective operations. ML framework will be disabled.",
                            NULL != include_sbgp_name ? "include only" : "exclude",
                            NULL != include_sbgp_name ? include_sbgp_name : exclude_sbgp_name
                            ));
                ret = OMPI_ERROR;
                goto ERROR;
            }
            ML_VERBOSE(10, ("Empty hierarchy..."));
            ret = OMPI_SUCCESS;
            goto ERROR;
        }

        topo->n_levels = n_hier;

        /* Find lowest and highest index of the groups in this communicator.
        ** This will be needed in deciding where in the hierarchical collective
        ** sequence of calls these particular groups belong.
        ** It is done with one allreduce call to save allreduce overhead.
        */
        all_reduce_buffer2_in[0] = (short)my_lowest_group_index;
        all_reduce_buffer2_in[1] = (short)-my_highest_group_index;
        /* restore map to ranks for the original communicator */
        for (i = 0; i < ompi_comm_size(ml_module->comm); i++) {
            map_to_comm_ranks[i] = i;
        }

        ret = comm_allreduce_pml(all_reduce_buffer2_in, all_reduce_buffer2_out,
                                 2, MPI_SHORT, ompi_comm_rank(ml_module->comm),
                                 MPI_MIN, original_group_size,
                                 map_to_comm_ranks, ml_module->comm);
        if (OPAL_UNLIKELY(OMPI_SUCCESS != ret)) {
            ML_VERBOSE(10, ("comm_allreduce_pml failed. all_reduce_buffer2_in reduction\n"));
            goto ERROR;
        }

        topo->global_lowest_hier_group_index = all_reduce_buffer2_out[0];
        topo->global_highest_hier_group_index = -all_reduce_buffer2_out[1];

        ML_VERBOSE(10, ("The lowest index and highest index was successfully found.\n"));

        ML_VERBOSE(10, ("ml_discover_hierarchy done, n_levels %d lowest_group_index %d highest_group_index %d,"
                    " original_group_size %d my_lowest_group_index %d my_highest_group_index %d",
                    topo->n_levels, topo->global_lowest_hier_group_index,
                    topo->global_highest_hier_group_index,
                    original_group_size,
                    my_lowest_group_index,
                    my_highest_group_index));

        /*
         * setup detailed subgroup information
         */
        ret = ml_setup_full_tree_data(topo, ml_module->comm, my_highest_group_index,
            map_to_comm_ranks,&num_total_subgroups,&array_of_all_subgroup_ranks,
            &list_of_ranks_in_all_subgroups);

        if (OPAL_UNLIKELY(OMPI_SUCCESS != ret)) {
            ML_VERBOSE(10, ("comm_allreduce_pml failed:  bcols_in_use reduction %d \n",ret));
            goto ERROR;
        }

        /* cache the ML hierarchical description on the tree */
        topo->number_of_all_subgroups = num_total_subgroups;
        topo->array_of_all_subgroups = array_of_all_subgroup_ranks;

        ml_init_k_nomial_trees(topo, list_of_ranks_in_all_subgroups, ompi_comm_rank(ml_module->comm));
        /* Set the route table if know-root type of algorithms is used */
        if (mca_coll_ml_component.use_static_bcast) {
            ret = mca_coll_ml_fill_in_route_tab(topo, ml_module->comm);
            if (OMPI_SUCCESS != ret) {
                ML_ERROR(("mca_coll_ml_fill_in_route_tab returned an error.\n"));
                goto ERROR;
            }
        }

        /*
        ** If all ranks are selected, there will be a single rank that remains -
        ** the root of the last group.  Check to make sure that all ranks are
        ** selected, and if not, return an error.  We can't handle the collectives
        ** correctly with this module.
        */

ERROR:

        ML_VERBOSE(10, ("Discovery done\n"));

        /* free temp resources */
        if (NULL != all_selected) {
            free(all_selected);
            all_selected = NULL;
        }

        if (NULL != copy_procs) {
            free(copy_procs);
            copy_procs = NULL;
        }

        if (NULL != map_to_comm_ranks) {
            free(map_to_comm_ranks);
            map_to_comm_ranks = NULL;
        }

        if (NULL != index_proc_selected) {
            free(index_proc_selected);
            index_proc_selected = NULL;
        }

        if (NULL != bcols_in_use) {
            free(bcols_in_use);
            bcols_in_use = NULL;
        }

        if (NULL != list_of_ranks_in_all_subgroups) {
            free(list_of_ranks_in_all_subgroups);
            list_of_ranks_in_all_subgroups = NULL;
        }

        return ret;
}

void mca_coll_ml_allreduce_matrix_init(mca_coll_ml_module_t *ml_module,
                     const mca_bcol_base_component_2_0_0_t *bcol_component)
{
    int op, dt, et;

    for (op = 0; op < OMPI_OP_NUM_OF_TYPES; ++op) {
        for (dt = 0; dt < OMPI_DATATYPE_MAX_PREDEFINED; ++dt) {
            for (et = 0; et < BCOL_NUM_OF_ELEM_TYPES; ++et) {
                ml_module->allreduce_matrix[op][dt][et] =
                           bcol_component->coll_support(op, dt, et);
            }
        }
    }
}

int mca_coll_ml_fulltree_hierarchy_discovery(mca_coll_ml_module_t *ml_module,
        int n_hierarchies)
{
    return mca_coll_ml_tree_hierarchy_discovery(ml_module,
            &ml_module->topo_list[COLL_ML_HR_FULL],
            n_hierarchies, NULL, NULL);
}

int mca_coll_ml_allreduce_hierarchy_discovery(mca_coll_ml_module_t *ml_module,
        int n_hierarchies)
{
    mca_base_component_list_item_t *bcol_cli;
    const mca_bcol_base_component_2_0_0_t *bcol_component;

    sbgp_base_component_keyval_t *sbgp_cli;
    const mca_sbgp_base_component_2_0_0_t *sbgp_component;

    sbgp_cli = (sbgp_base_component_keyval_t *)
              opal_list_get_first(&mca_sbgp_base_components_in_use);

    for (bcol_cli = (mca_base_component_list_item_t *)
                  opal_list_get_first(&mca_bcol_base_components_in_use);
            (opal_list_item_t *) bcol_cli !=
                    opal_list_get_end(&mca_bcol_base_components_in_use);
                        bcol_cli = (mca_base_component_list_item_t *)
                            opal_list_get_next((opal_list_item_t *) bcol_cli),
                        sbgp_cli = (sbgp_base_component_keyval_t *)
                            opal_list_get_next((opal_list_item_t *) sbgp_cli)) {
        bcol_component = (mca_bcol_base_component_2_0_0_t *) bcol_cli->cli_component;
        if (NULL != bcol_component->coll_support_all_types &&
                     !bcol_component->coll_support_all_types(BCOL_ALLREDUCE)) {
            mca_base_component_list_item_t *bcol_cli_next;
            const mca_bcol_base_component_2_0_0_t *bcol_component_next;

            bcol_cli_next = (mca_base_component_list_item_t *)
                            opal_list_get_next((opal_list_item_t *) bcol_cli);

            mca_coll_ml_component.need_allreduce_support = true;
            mca_coll_ml_allreduce_matrix_init(ml_module, bcol_component);

            sbgp_component = (mca_sbgp_base_component_2_0_0_t *)
                                    sbgp_cli->component.cli_component;

            ML_VERBOSE(10, ("Topology build: sbgp %s will be excluded.",
                             sbgp_component->sbgp_version.mca_component_name));

            /* If there isn't additional component supports all types => print warning */
            if (1 == opal_list_get_size(&mca_bcol_base_components_in_use) ||
                  (opal_list_item_t *) bcol_cli_next ==
                              opal_list_get_end(&mca_bcol_base_components_in_use)) {
                ML_ERROR(("\n--------------------------------------------------------------------------------\n"
                          "The BCOL component %s doesn't support \n"
                          "all possible tuples (OPERATION X DATATYPE) for Allreduce \n"
                          "and you didn't provide additional one for alternative topology building, \n"
                          "as a result ML isn't be run correctly and its behavior is undefined. \n"
                          "You should run this bcol with another one supports all possible tuples, \n"
                          "\"--mca bcol_base_string %s,ptpcoll --mca sbgp_base_subgroups_string %s,p2p\" for example.\n",
                          bcol_component->bcol_version.mca_component_name,
                          bcol_component->bcol_version.mca_component_name,
                          sbgp_component->sbgp_version.mca_component_name));
            } else {
                bcol_component_next = (mca_bcol_base_component_2_0_0_t *)
                                               bcol_cli_next->cli_component;

                if (NULL != bcol_component_next->coll_support_all_types &&
                     !bcol_component_next->coll_support_all_types(BCOL_ALLREDUCE)) {
                    ML_ERROR(("\n--------------------------------------------------------------------------------\n"
                          "The BCOL component %s doesn't support \n"
                          "all possible tuples for Allreduce. \n"
                          "While you did provid an additional %s bcol component for alternative topology building, \n"
                          "this component also lacks support for all tuples. \n"
                          "As a result, ML Allreduce's behavior is undefined. \n"
                          "You must provide a component that supports all possible tuples, e.g. \n"
                          "\"--mca bcol_base_string %s,ptpcoll --mca sbgp_base_subgroups_string %s,p2p\n",
                          bcol_component->bcol_version.mca_component_name,
                          bcol_component_next->bcol_version.mca_component_name,
                          bcol_component->bcol_version.mca_component_name,
                          sbgp_component->sbgp_version.mca_component_name));
                }
            }

            return mca_coll_ml_tree_hierarchy_discovery(ml_module,
                    &ml_module->topo_list[COLL_ML_HR_ALLREDUCE],
                    n_hierarchies, sbgp_component->sbgp_version.mca_component_name, NULL);
        }
    }

    return OMPI_SUCCESS;
}

int mca_coll_ml_fulltree_exclude_basesmsocket_hierarchy_discovery(mca_coll_ml_module_t *ml_module,
        int n_hierarchies)
{
    return mca_coll_ml_tree_hierarchy_discovery(ml_module,
            &ml_module->topo_list[COLL_ML_HR_NBS],
            n_hierarchies, "basesmsocket", NULL);
}

int mca_coll_ml_fulltree_ptp_only_hierarchy_discovery(mca_coll_ml_module_t *ml_module,
        int n_hierarchies)
{
    return mca_coll_ml_tree_hierarchy_discovery(ml_module,
            &ml_module->topo_list[COLL_ML_HR_SINGLE_PTP],
            n_hierarchies, NULL, "p2p");
}

int mca_coll_ml_fulltree_iboffload_only_hierarchy_discovery(mca_coll_ml_module_t *ml_module,
        int n_hierarchies)
{
    return mca_coll_ml_tree_hierarchy_discovery(ml_module,
            &ml_module->topo_list[COLL_ML_HR_SINGLE_IBOFFLOAD],
            n_hierarchies, NULL, "ibnet");
}

#define IS_RECHABLE 1
#define IS_NOT_RECHABLE -1

static int mca_coll_ml_fill_in_route_tab(mca_coll_ml_topology_t *topo, ompi_communicator_t *comm)
{
    int i, rc, level, comm_size = 0,
        my_rank = ompi_comm_rank(comm);

    int32_t **route_table = NULL;
    int32_t *all_reachable_ranks = NULL;

    struct ompi_proc_t **ompi_procs = NULL;
    struct ompi_proc_t **sbgp_procs = NULL;

    mca_sbgp_base_module_t *sbgp_group = NULL;
    comm_size = ompi_comm_size(comm);

    all_reachable_ranks = (int32_t *) malloc(comm_size * sizeof(int32_t));
    if (NULL == all_reachable_ranks) {
        ML_VERBOSE(10, ("Cannot allocate memory.\n"));
        rc = OMPI_ERR_OUT_OF_RESOURCE;
        goto ERROR;
    }

    for (i = 0; i < comm_size; ++i) {
        all_reachable_ranks[i] = IS_NOT_RECHABLE;
    }

    route_table = (int32_t **) calloc(topo->n_levels, sizeof(int32_t *));
    if (NULL == route_table) {
        ML_VERBOSE(10, ("Cannot allocate memory.\n"));
        rc = OMPI_ERR_OUT_OF_RESOURCE;
        goto ERROR;
    }

    topo->route_vector = (mca_coll_ml_route_info_t *)
                                calloc(comm_size, sizeof(mca_coll_ml_route_info_t));
    if (NULL == topo->route_vector) {
        ML_VERBOSE(10, ("Cannot allocate memory.\n"));
        rc = OMPI_ERR_OUT_OF_RESOURCE;
        goto ERROR;
    }

    all_reachable_ranks[my_rank] = IS_RECHABLE;
    ompi_procs = comm->c_local_group->grp_proc_pointers;

    for (level = 0; level < topo->n_levels; ++level) {
        sbgp_group = topo->component_pairs[level].subgroup_module;

        route_table[level] = (int32_t *) malloc(comm_size * sizeof(int32_t));
        if (NULL == route_table[level]) {
            ML_VERBOSE(10, ("Cannot allocate memory.\n"));
            rc = OMPI_ERR_OUT_OF_RESOURCE;
            goto ERROR;
        }

        for (i = 0; i < comm_size; ++i) {
            if (IS_NOT_RECHABLE != all_reachable_ranks[i]) {
                all_reachable_ranks[i] = sbgp_group->my_index;
            }
        }

        rc = comm_allreduce_pml(all_reachable_ranks,
                route_table[level],
                comm_size,
                MPI_INT, sbgp_group->my_index,
                MPI_MAX, sbgp_group->group_size,
                sbgp_group->group_list,
                comm);
        if (OMPI_SUCCESS != rc) {
            ML_VERBOSE(10, ("comm_allreduce failed.\n"));
            goto ERROR;
        }

        for (i = 0; i < comm_size; ++i) {
            if (IS_NOT_RECHABLE !=
                         route_table[level][i]) {
                all_reachable_ranks[i] = IS_RECHABLE;
            }
        }
    }

    assert(0 < level);

    /* If there are unreachable ranks =>
       reach them through leader of my upper layer */
    for (i = 0; i < comm_size; ++i) {
        if (IS_NOT_RECHABLE ==
                   route_table[level - 1][i]) {
            route_table[level - 1][i] = 0;
        }
    }

    free(all_reachable_ranks);

    for (i = 0; i < comm_size; ++i) {
        for (level = 0; level < topo->n_levels; ++level) {
            if (IS_NOT_RECHABLE != route_table[level][i]) {
                topo->route_vector[i].level = level;
                topo->route_vector[i].rank = route_table[level][i];
                break;
            }
        }
    }

#if OPAL_ENABLE_DEBUG
#define COLL_ML_ROUTE_BUFF_SIZE (1024*1024)
    {
        int ii, jj;
        char buff[COLL_ML_ROUTE_BUFF_SIZE];
        char *output = buff;

        memset(buff, 0, COLL_ML_ROUTE_BUFF_SIZE);

        sprintf(output, "ranks:   ");

        output = buff + strlen(buff);
        assert(COLL_ML_ROUTE_BUFF_SIZE + buff > output);

        for(ii = 0; ii < comm_size; ++ii) {
            sprintf(output, " %2d",  ii);

            output = buff + strlen(buff);
            assert(COLL_ML_ROUTE_BUFF_SIZE + buff > output);
        }

        for (ii = 0; ii < topo->n_levels; ++ii) {
            sprintf(output, "\nlevel: %d ", ii);

            output = buff + strlen(buff);
            assert(COLL_ML_ROUTE_BUFF_SIZE + buff > output);
            for(jj = 0; jj < comm_size; ++jj) {
                sprintf(output, " %2d", route_table[ii][jj]);

                output = buff + strlen(buff);
                assert(COLL_ML_ROUTE_BUFF_SIZE + buff > output);
            }
        }

        sprintf(output, "\n\nThe vector is:\n============\nranks:       ");

        output = buff + strlen(buff);
        assert(COLL_ML_ROUTE_BUFF_SIZE + buff > output);

        for(ii = 0; ii < comm_size; ++ii) {
            sprintf(output, " %6d",  ii);

            output = buff + strlen(buff);
            assert(COLL_ML_ROUTE_BUFF_SIZE + buff > output);
        }

        sprintf(output, "\nlevel x rank: ");

        output = buff + strlen(buff);
        assert(COLL_ML_ROUTE_BUFF_SIZE + buff > output);

        for(ii = 0; ii < comm_size; ++ii) {
            sprintf(output, " (%d, %d)",
                            topo->route_vector[ii].level,
                            topo->route_vector[ii].rank);

            output = buff + strlen(buff);
            assert(COLL_ML_ROUTE_BUFF_SIZE + buff > output);
        }

        ML_VERBOSE(10, ("\nThe table is:\n============\n%s\n", buff));
    }
#endif

    for (level = 0; level < topo->n_levels; ++level) {
        free(route_table[level]);
    }

    free(route_table);

    return OMPI_SUCCESS;

ERROR:

    ML_VERBOSE(10, ("Exit with error status - %d.\n", rc));
    if (NULL != route_table) {
        for (level = 0; level < topo->n_levels; ++level) {
            if (NULL != route_table[level]) {
                free(route_table[level]);
            }
        }

        free(route_table);
    }

    if (NULL != sbgp_procs) {
        free(sbgp_procs);
    }

    if (NULL != all_reachable_ranks) {
        free(all_reachable_ranks);
    }

    return rc;
}

static void init_coll_func_pointers(mca_coll_ml_module_t *ml_module)
{
    mca_coll_base_module_2_0_0_t *coll_base = &ml_module->super;

    int iboffload_used =
            mca_coll_ml_check_if_bcol_is_used("iboffload", ml_module, COLL_ML_TOPO_MAX);

    /* initialize coll component function pointers */
    coll_base->coll_module_enable = ml_module_enable;
    coll_base->ft_event        = NULL;

    if (mca_coll_ml_component.disable_allgather) {
        coll_base->coll_allgather = NULL;
        coll_base->coll_iallgather = NULL;
    } else {
        coll_base->coll_allgather = NULL;
        coll_base->coll_iallgather = NULL;
    }

    coll_base->coll_allgatherv = NULL;

    if (mca_coll_ml_component.use_knomial_allreduce) {
        if (true == mca_coll_ml_component.need_allreduce_support) {
            coll_base->coll_allreduce = NULL;
        } else {
            coll_base->coll_allreduce = NULL;
        }
    } else {
        coll_base->coll_allreduce = NULL;
    }

    if (mca_coll_ml_component.disable_alltoall) {
        coll_base->coll_alltoall = NULL;
        coll_base->coll_ialltoall = NULL;
    } else {
        coll_base->coll_alltoall = NULL;
        coll_base->coll_ialltoall = NULL;
    }

    coll_base->coll_alltoallv  = NULL;
    coll_base->coll_alltoallw  = NULL;

    coll_base->coll_barrier = mca_coll_ml_barrier_intra;

    /* Use the sequential broadcast */
    if (mca_coll_ml_component.use_sequential_bcast) {
        coll_base->coll_bcast = mca_coll_ml_bcast_sequential_root;
    } else {
        coll_base->coll_bcast = mca_coll_ml_parallel_bcast;
    }

    coll_base->coll_exscan     = NULL;
    coll_base->coll_gather     = NULL;
    /* Current iboffload/ptpcoll version have no support for gather */
    if (iboffload_used  ||
        mca_coll_ml_check_if_bcol_is_used("ptpcoll", ml_module, COLL_ML_TOPO_MAX)) {
        coll_base->coll_gather      = NULL;
    }


    coll_base->coll_gatherv    = NULL;

    coll_base->coll_reduce     = NULL;
    coll_base->coll_reduce_scatter = NULL;
    coll_base->coll_scan       = NULL;
    coll_base->coll_scatter    = NULL;
#if 0
    coll_base->coll_scatter    = mca_coll_ml_scatter_sequential;
#endif
    coll_base->coll_scatterv   = NULL;

    coll_base->coll_iallgatherv = NULL;
    coll_base->coll_iallreduce  = NULL;
    coll_base->coll_ialltoallv  = NULL;
    coll_base->coll_ialltoallw  = NULL;
    coll_base->coll_ibarrier    = mca_coll_ml_ibarrier_intra;

    coll_base->coll_ibcast      = mca_coll_ml_parallel_bcast_nb;
    coll_base->coll_iexscan     = NULL;
    coll_base->coll_igather     = NULL;
    coll_base->coll_igatherv    = NULL;
    coll_base->coll_ireduce     = NULL;
    coll_base->coll_ireduce_scatter = NULL;
    coll_base->coll_iscan       = NULL;
    coll_base->coll_iscatter    = NULL;
    coll_base->coll_iscatterv   = NULL;
}

static int init_lists(mca_coll_ml_module_t *ml_module)
{
    mca_coll_ml_component_t *cs = &mca_coll_ml_component;
    int num_elements = cs->free_list_init_size;
    int max_elements = cs->free_list_max_size;
    int elements_per_alloc = cs->free_list_grow_size;
    size_t length_payload = 0;
    size_t length;
    int ret;

    /* initialize full message descriptors - moving this to the
     *   module, as the fragment has resrouce requirements that
     *   are communicator dependent */
    OBJ_CONSTRUCT(&(ml_module->message_descriptors), ompi_free_list_t);

    /* no data associated with the message descriptor */

    length = sizeof(mca_coll_ml_descriptor_t);
    ret = ompi_free_list_init_ex_new(&(ml_module->message_descriptors), length,
            opal_cache_line_size, OBJ_CLASS(mca_coll_ml_descriptor_t),
            length_payload, 0,
            num_elements, max_elements, elements_per_alloc,
            NULL,
            init_ml_message_desc, ml_module);
    if (OPAL_UNLIKELY(OMPI_SUCCESS != ret)) {
        ML_ERROR(("ompi_free_list_init_ex_new exit with error"));
        return ret;
    }

    /* initialize fragement descriptors - always associate one fragment
     * descriptr with full message descriptor, so that we can minimize
     * small message latency */

    /* create a free list of fragment descriptors */
    OBJ_CONSTRUCT(&(ml_module->fragment_descriptors), ompi_free_list_t);

    /*length_payload=sizeof(something);*/
    length = sizeof(mca_coll_ml_fragment_t);
    ret = ompi_free_list_init_ex_new(&(ml_module->fragment_descriptors), length,
            opal_cache_line_size, OBJ_CLASS(mca_coll_ml_fragment_t),
            length_payload, 0,
            num_elements, max_elements, elements_per_alloc,
            NULL,
            init_ml_fragment_desc, ml_module);
    if (OMPI_SUCCESS != ret) {
        ML_ERROR(("ompi_free_list_init_ex_new exit with error"));
        return ret;
    }

    return OMPI_SUCCESS;
}

static int check_for_max_supported_ml_modules(struct ompi_communicator_t *comm)
{
    int i, ret;
    mca_coll_ml_component_t *cs = &mca_coll_ml_component;
    int *comm_ranks = NULL;

    comm_ranks = (int *)calloc(ompi_comm_size(comm), sizeof(int));
    if (OPAL_UNLIKELY(NULL == comm_ranks)) {
        ML_VERBOSE(10, ("Cannot allocate memory.\n"));
        return OMPI_ERR_OUT_OF_RESOURCE;
    }
    for (i = 0; i < ompi_comm_size(comm); i++) {
        comm_ranks[i] = i;
    }

    ret = comm_allreduce_pml(&cs->max_comm, &cs->max_comm,
            1 , MPI_INT, ompi_comm_rank(comm),
            MPI_MIN, ompi_comm_size(comm), comm_ranks,
            comm);
    if (OMPI_SUCCESS != ret) {
        ML_ERROR(("comm_allreduce - failed to collect max_comm data"));
        return ret;
    }

    if (0 >= cs->max_comm ||
            ompi_comm_size(comm) < cs->min_comm_size) {
        return OMPI_ERROR;
    } else {
        --cs->max_comm;
    }

    free(comm_ranks);

    return OMPI_SUCCESS;
}

#if OPAL_ENABLE_DEBUG
#define DEBUG_ML_COMM_QUERY()                                                                   \
    do {                                                                                        \
        static int verbosity_level = 5;                                                         \
        static int module_num = 0;                                                              \
        ML_VERBOSE(10, ("ML module - %p num %d for comm - %p, "                                 \
                    "comm size - %d, ML component prio - %d.\n",                                \
                    ml_module, ++module_num, comm, ompi_comm_size(comm), *priority));           \
        /* For now I want to always print that we enter ML -                                    \
           at the past there was an issue that we did not enter ML and actually run with tuned. \
           Still I do not want to print it for each module - only for the first. */             \
        ML_VERBOSE(verbosity_level, ("ML module - %p was successfully created", ml_module));    \
        verbosity_level = 10;                                                                   \
    } while(0)

#else
#define DEBUG_ML_COMM_QUERY()
#endif

static int mca_coll_ml_need_multi_topo(int bcol_collective)
{
    mca_base_component_list_item_t *bcol_cli;
    const mca_bcol_base_component_2_0_0_t *bcol_component;

    for (bcol_cli = (mca_base_component_list_item_t *)
                  opal_list_get_first(&mca_bcol_base_components_in_use);
            (opal_list_item_t *) bcol_cli !=
                    opal_list_get_end(&mca_bcol_base_components_in_use);
                        bcol_cli = (mca_base_component_list_item_t *)
                            opal_list_get_next((opal_list_item_t *) bcol_cli)) {
        bcol_component = (mca_bcol_base_component_2_0_0_t *) bcol_cli->cli_component;
        if (NULL != bcol_component->coll_support_all_types &&
                     !bcol_component->coll_support_all_types(bcol_collective)) {
            return true;
        }
    }

    return false;
}

/* We may call this function ONLY AFTER algorithm initialization */
static int setup_bcast_table(mca_coll_ml_module_t *module)
{
    mca_coll_ml_component_t *cm = &mca_coll_ml_component;

    /* setup bcast index table */
    if (cm->use_static_bcast) {
        module->bcast_fn_index_table[0] = ML_BCAST_SMALL_DATA_KNOWN;
        if (cm->enable_fragmentation) {
            module->bcast_fn_index_table[1] = ML_BCAST_SMALL_DATA_KNOWN;
        } else if (!(MCA_BCOL_BASE_ZERO_COPY &
                    module->coll_ml_bcast_functions[ML_BCAST_LARGE_DATA_KNOWN]->topo_info->all_bcols_mode)) {
            ML_ERROR(("ML couldn't be used: because the mca param coll_ml_enable_fragmentation "
                      "was set to zero and there is a bcol doesn't support zero copy method."));
            return OMPI_ERROR;
        } else {
            module->bcast_fn_index_table[1] = ML_BCAST_LARGE_DATA_KNOWN;
        }
    } else {
        module->bcast_fn_index_table[0] = ML_BCAST_SMALL_DATA_UNKNOWN;
        if (cm->enable_fragmentation) {
            module->bcast_fn_index_table[1] = ML_BCAST_SMALL_DATA_UNKNOWN;
        } else if (!(MCA_BCOL_BASE_ZERO_COPY &
                    module->coll_ml_bcast_functions[ML_BCAST_LARGE_DATA_UNKNOWN]->topo_info->all_bcols_mode)) {
            ML_ERROR(("ML couldn't be used: because the mca param coll_ml_enable_fragmentation "
                      "was set to zero and there is a bcol doesn't support zero copy method."));
            return OMPI_ERROR;
        } else {
            /* If the topology support zero level and no fragmentation was requested */
            module->bcast_fn_index_table[1] = ML_BCAST_LARGE_DATA_UNKNOWN;
        }
    }

    return OMPI_SUCCESS;
}

static void ml_check_for_enabled_topologies (int map[][MCA_COLL_MAX_NUM_SUBTYPES], mca_coll_ml_topology_t *topo_list)
{
    int coll_i, st_i;
    for (coll_i = 0; coll_i < MCA_COLL_MAX_NUM_COLLECTIVES; coll_i++) {
        for (st_i = 0; st_i < MCA_COLL_MAX_NUM_SUBTYPES; st_i++) {
            if (map[coll_i][st_i] > -1) {
                /* The topology is used, so set it to enabled */
                assert(map[coll_i][st_i] <= COLL_ML_TOPO_MAX);
                topo_list[map[coll_i][st_i]].status = COLL_ML_TOPO_ENABLED;
            }
        }
    }
}

static void setup_default_topology_map(mca_coll_ml_module_t *ml_module)
{
    int i, j;
    for (i = 0; i < MCA_COLL_MAX_NUM_COLLECTIVES; i++) {
        for (j = 0; j < MCA_COLL_MAX_NUM_SUBTYPES; j++) {
            ml_module->collectives_topology_map[i][j] = -1;
        }
    }

    ml_module->collectives_topology_map[ML_BARRIER][ML_BARRIER_DEFAULT]           = COLL_ML_HR_FULL;

    ml_module->collectives_topology_map[ML_BCAST][ML_BCAST_SMALL_DATA_KNOWN]      = COLL_ML_HR_FULL;
    ml_module->collectives_topology_map[ML_BCAST][ML_BCAST_SMALL_DATA_UNKNOWN]    = COLL_ML_HR_FULL;
    ml_module->collectives_topology_map[ML_BCAST][ML_BCAST_SMALL_DATA_SEQUENTIAL] = COLL_ML_HR_FULL;
    ml_module->collectives_topology_map[ML_BCAST][ML_BCAST_LARGE_DATA_KNOWN]      = COLL_ML_HR_FULL;
    ml_module->collectives_topology_map[ML_BCAST][ML_BCAST_LARGE_DATA_UNKNOWN]    = COLL_ML_HR_FULL;
    ml_module->collectives_topology_map[ML_BCAST][ML_BCAST_LARGE_DATA_UNKNOWN]    = COLL_ML_HR_FULL;

    ml_module->collectives_topology_map[ML_ALLGATHER][ML_SMALL_DATA_ALLGATHER]    = COLL_ML_HR_FULL;
    ml_module->collectives_topology_map[ML_ALLGATHER][ML_LARGE_DATA_ALLGATHER]    = COLL_ML_HR_FULL;

    ml_module->collectives_topology_map[ML_GATHER][ML_SMALL_DATA_GATHER]    = COLL_ML_HR_FULL;
    ml_module->collectives_topology_map[ML_GATHER][ML_LARGE_DATA_GATHER]    = COLL_ML_HR_FULL;

    ml_module->collectives_topology_map[ML_ALLTOALL][ML_SMALL_DATA_ALLTOALL]      = COLL_ML_HR_SINGLE_IBOFFLOAD;
    ml_module->collectives_topology_map[ML_ALLTOALL][ML_LARGE_DATA_ALLTOALL]      = COLL_ML_HR_SINGLE_IBOFFLOAD;

    ml_module->collectives_topology_map[ML_ALLREDUCE][ML_SMALL_DATA_ALLREDUCE]    = COLL_ML_HR_FULL;
    ml_module->collectives_topology_map[ML_ALLREDUCE][ML_LARGE_DATA_ALLREDUCE]    = COLL_ML_HR_FULL;

    if (mca_coll_ml_need_multi_topo(BCOL_ALLREDUCE)) {
        ml_module->collectives_topology_map[ML_ALLREDUCE][ML_SMALL_DATA_EXTRA_TOPO_ALLREDUCE] = COLL_ML_HR_ALLREDUCE;
        ml_module->collectives_topology_map[ML_ALLREDUCE][ML_LARGE_DATA_EXTRA_TOPO_ALLREDUCE] = COLL_ML_HR_ALLREDUCE;
    }

    ml_module->collectives_topology_map[ML_SCATTER][ML_SCATTER_SMALL_DATA_KNOWN]  = COLL_ML_HR_FULL;
    ml_module->collectives_topology_map[ML_SCATTER][ML_SCATTER_N_DATASIZE_BINS]   = COLL_ML_HR_FULL;
    ml_module->collectives_topology_map[ML_SCATTER][ML_SCATTER_SMALL_DATA_UNKNOWN]    = COLL_ML_HR_FULL;
    ml_module->collectives_topology_map[ML_SCATTER][ML_SCATTER_SMALL_DATA_SEQUENTIAL] = COLL_ML_HR_FULL;
}

#define GET_CF(I, J) (&mca_coll_ml_component.coll_config[I][J]);

static void load_cached_config(mca_coll_ml_module_t *ml_module)
{
    int c_idx, m_idx, alg;
    per_collective_configuration_t *cf = NULL;

    for (c_idx = 0; c_idx < ML_NUM_OF_FUNCTIONS; c_idx++) {
        for (m_idx = 0; m_idx < ML_NUM_MSG; m_idx++) {
            cf = GET_CF(c_idx, m_idx);
            /* load topology tunings */
            if (ML_UNDEFINED != cf->topology_id &&
                ML_UNDEFINED != cf->algorithm_id) {
                alg =
                    cf->algorithm_id;
                ml_module->collectives_topology_map[c_idx][alg] =
                    cf->topology_id;
            }
        }
    }
}

/* Pasha: In future I would suggest to convert this configuration to some sophisticated mca parameter or
 even configuration file. On this stage of project I will set it statically and later we will change it
 to run time parameter */
static void setup_topology_coll_map(mca_coll_ml_module_t *ml_module)
{
    /* Load default topology setup */
    setup_default_topology_map(ml_module);

    /* Load configuration file */
    load_cached_config(ml_module);

    ml_check_for_enabled_topologies(ml_module->collectives_topology_map, ml_module->topo_list);
}

/* query to see if the module is available for use on the given
 * communicator, and if so, what it's priority is.  This is where
 * the backing shared-memory file is created.
 */
mca_coll_base_module_t *
mca_coll_ml_comm_query(struct ompi_communicator_t *comm, int *priority)
{
    /* local variables */
    int ret = OMPI_SUCCESS;

    mca_coll_ml_module_t *ml_module = NULL;
    mca_coll_ml_component_t *cs = &mca_coll_ml_component;
    double start, end, tic;
    bool iboffload_was_requested = mca_coll_ml_check_if_bcol_is_requested("iboffload");

    ML_VERBOSE(10, ("ML comm query start.\n"));

    /**
     * No support for inter-communicator yet.
     */
    if (OMPI_COMM_IS_INTER(comm)) {
        *priority = -1;
        return NULL;
    }

    /**
     * If it is inter-communicator and size is less than 2 we have specialized modules
     * to handle the intra collective communications.
     */
    if (OMPI_COMM_IS_INTRA(comm) && ompi_comm_size(comm) < 2) {
        ML_VERBOSE(10, ("It is inter-communicator and size is less than 2.\n"));
        *priority = -1;
        return NULL;
    }

    /**
     * In current implementation we limit number of supported ML modules in cases when
     * iboffload companent was requested
     */
    if (iboffload_was_requested) {
        ret = check_for_max_supported_ml_modules(comm);
        if (OMPI_SUCCESS != ret) {
            /* We have nothing to cleanup yet, so just return NULL */
            ML_VERBOSE(10, ("check_for_max_supported_ml_modules returns ERROR, return NULL"));
            *priority = -1;
            return NULL;
        }
    }

    ML_VERBOSE(10, ("Create ML module start.\n"));

    /* allocate and initialize an ml  module */
    ml_module = OBJ_NEW(mca_coll_ml_module_t);
    if (NULL == ml_module) {
        return NULL;
    }

    /* Get our priority */
    *priority = cs->ml_priority;

    /** Set initial ML values **/
    ml_module->comm = comm;
    /* set the starting sequence number */
    ml_module->collective_sequence_num = cs->base_sequence_number;
    ml_module->no_data_collective_sequence_num = cs->base_sequence_number;
    /* initialize the size of the largest collective communication description */
    ml_module->max_fn_calls = 0;

#ifdef NEW_LEADER_SELECTION
    coll_ml_construct_resource_graphs(ml_module);
#endif

    /* Set topology - function map */
    setup_topology_coll_map(ml_module);

    /**
     * This is the core of the function:
     * setup communicator hierarchy - the ml component is available for
     * caching information about the sbgp modules selected.
     */
    start = ret_us();
    ret = ml_discover_hierarchy(ml_module);
    if (OMPI_SUCCESS != ret) {
        ML_ERROR(("ml_discover_hierarchy exited with error.\n"));
        goto CLEANUP;
    }
    end = ret_us();
    tic = end - start;
    /*fprintf(stderr,"discover hierarchy %1.8f\n",tic);*/

    /* gvm Disabled for debuggin */
    ret = mca_coll_ml_build_filtered_fn_table(ml_module);
    if (OMPI_SUCCESS != ret) {
         ML_ERROR(("mca_coll_ml_build_filtered_fn_table returned an error.\n"));
         goto CLEANUP;
    }

    /* Generate active bcols list */
    generate_active_bcols_list(ml_module);

    /* setup collective schedules - note that a given bcol may have more than
       one module instantiated.  We may want to use the same collective cap
       capabilities over more than one set of procs.  Each module will store
       the relevant information for a given set of procs */
    ML_VERBOSE(10, ("Call for setup schedule.\n"));
    ret = ml_coll_schedule_setup(ml_module);
    if (OMPI_SUCCESS != ret) {
        ML_ERROR(("ml_coll_schedule_setup exit with error"));
        goto CLEANUP;
    }

    /* Setup bcast table */
    ML_VERBOSE(10, ("Setup bcast table\n"));
    ret = setup_bcast_table(ml_module);
    if (OMPI_SUCCESS != ret) {
        ML_ERROR(("setup_bcast_table exit with error"));
        goto CLEANUP;
    }

    ML_VERBOSE(10, ("Setup pointer to collectives calls.\n"));
    init_coll_func_pointers(ml_module);

    ML_VERBOSE(10, ("Setup free lists\n"));
    ret = init_lists(ml_module);
    if (OMPI_SUCCESS != ret) {
        goto CLEANUP;
    }

    DEBUG_ML_COMM_QUERY();

    /* Compute the bruck's buffer constant -- temp buffer requirements */
    {
        int comm_size =ompi_comm_size(comm);
        int count = 1, log_comm_size = 0;

        /* compute log of comm_size */
        while (count < comm_size) {
            count = count << 1;
            log_comm_size++;
        }

        ml_module->brucks_buffer_threshold_const =
                (comm_size / 2 + comm_size % 2) * (log_comm_size) ;


       ml_module->log_comm_size = log_comm_size;
    }

    if (iboffload_was_requested) {
        /* HACK: Calling memory sync barrier first time to make sure
         * that iboffload create qps for service barrier in right order,
         * otherwise we may have deadlock and really nasty data corruptions.
         * If you plan to remove this one - please talk to me first.
         * Pasha.
         !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
         Work around for deadlock caused by connection setup
         for asyc service barrier. Asyc service barrier use own set of
         MQ and QP _BUT_ the exchange operation uses the MQ that is used for
         primary set of collectives operations like Allgahter, Barrier,etc.
         As result exchange wait operation could be pushed to primary MQ and
         cause dead-lock.
         !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
         Create connection for service barrier and memory address exchange
         for ml buffers and asyc service barrier
         */
        ret = mca_coll_ml_memsync_intra(ml_module, 0);
        if (OMPI_SUCCESS != ret) {
            goto CLEANUP;
        }
        opal_progress();
    }

    /* The module is ready */
    ml_module->initialized = true;

    return &(ml_module->super);

CLEANUP:
    /* Vasily: RLG:  Need to cleanup free lists */
    if (NULL != ml_module) {
        OBJ_RELEASE(ml_module);
    }

    return NULL;
}

/*
 * Init module on the communicator
 */
static int
ml_module_enable(mca_coll_base_module_t *module,
                         struct ompi_communicator_t *comm)
{
    /* local variables */
    char output_buffer[2 * MPI_MAX_OBJECT_NAME];

    memset(&output_buffer[0], 0, sizeof(output_buffer));
    snprintf(output_buffer, sizeof(output_buffer), "%s (cid %d)", comm->c_name,
                       comm->c_contextid);

    ML_VERBOSE(10, ("coll:ml:enable: new communicator: %s.\n", output_buffer));

    /* All done */
    return OMPI_SUCCESS;
}

OBJ_CLASS_INSTANCE(mca_coll_ml_module_t,
                   mca_coll_base_module_t,
                   mca_coll_ml_module_construct,
                   mca_coll_ml_module_destruct);

OBJ_CLASS_INSTANCE(mca_coll_ml_collective_operation_progress_t,
        ompi_request_t,
        mca_coll_ml_collective_operation_progress_construct,
        mca_coll_ml_collective_operation_progress_destruct);


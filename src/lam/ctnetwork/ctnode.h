/*
 * Copyright 2002-2003. The Regents of the University of California. This material
 * was produced under U.S. Government contract W-7405-ENG-36 for Los Alamos
 * National Laboratory, which is operated by the University of California for
 * the U.S. Department of Energy. The Government is granted for itself and
 * others acting on its behalf a paid-up, nonexclusive, irrevocable worldwide
 * license in this material to reproduce, prepare derivative works, and
 * perform publicly and display publicly. Beginning five (5) years after
 * October 10,2002 subject to additional five-year worldwide renewals, the
 * Government is granted for itself and others acting on its behalf a paid-up,
 * nonexclusive, irrevocable worldwide license in this material to reproduce,
 * prepare derivative works, distribute copies to the public, perform publicly
 * and display publicly, and to permit others to do so. NEITHER THE UNITED
 * STATES NOR THE UNITED STATES DEPARTMENT OF ENERGY, NOR THE UNIVERSITY OF
 * CALIFORNIA, NOR ANY OF THEIR EMPLOYEES, MAKES ANY WARRANTY, EXPRESS OR
 * IMPLIED, OR ASSUMES ANY LEGAL LIABILITY OR RESPONSIBILITY FOR THE ACCURACY,
 * COMPLETENESS, OR USEFULNESS OF ANY INFORMATION, APPARATUS, PRODUCT, OR
 * PROCESS DISCLOSED, OR REPRESENTS THAT ITS USE WOULD NOT INFRINGE PRIVATELY
 * OWNED RIGHTS.
 
 * Additionally, this program is free software; you can distribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the License,
 * or any later version.  Accordingly, this program is distributed in the hope
 * that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 */
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef CT_NODE_H
#define CT_NODE_H

#include "lam/lfc/object.h"
#include "lam/lfc/hash_table.h"


/*
 *
 *  Abstract topology node class
 *
 */

#define CTNODE(obj)     (lam_ctnode_t *)(obj)

struct lam_ctnode;

typedef struct lam_ctnode_class
{
    lam_class_info_t    super;
    uint32_t            ctl_label_for_link(struct lam_ctnode *, uint32_t);
    char                *ctl_isa_neighbor(struct lam_ctnode *, uint32_t);
} lam_ctnode_class_t;




/*
 *
 *  Available concrete topology classes
 *
 */


extern lam_ctnode_class_t     hypercube_cls;




/*
 *
 *  Abstract topology node interface
 *  every concrete topology should derive from this class.
 *
 */

typedef struct lam_ctnode
{
    lam_object_t    super;
    uint32_t        ctn_label;
    uint32_t        ctn_num_nodes;  /* total # of nodes in network */
    void            *ctn_user_info;
    lam_fast_hash_t ctn_neighbors;
    lam_fast_hash_t ctn_scatter_cache;
    lam_fast_hash_t ctn_bcast_cache;    
} lam_ctnode_t;


void lam_ctn_init(lam_ctnode_t *node);
void lam_ctn_destroy(lam_ctnode_t *node);

/*
 *
 *  Functions for managing neighbors
 *
 */

void *lam_ctn_get_neighbor(lam_ctnode_t *node, uint32_t neighbor_label);
/*
 PRE:   neighbor_label is the label of the node's neighbor
 POST:  returns a pointer to the node's neighbor
 */

void lam_ctn_set_neighbor(lam_ctnode_t *node, uint32_t label, void *neighbor);
/*
 PRE:    label represents the label for a valid neighbor.
 POST:   Adds a link to a neighbor with specified label.
 */


/*
 *
 *  Accessor functions
 *
 */

INLINE uint32_t lam_ctn_get_label(lam_ctnode_t *node) {return node->ctn_label;}
INLINE void lam_ctn_set_label(lam_ctnode_t *node, uint32_t label)
    {node->ctn_label = label;}

INLINE uint32_t lam_ctn_get_num_nodes(lam_ctnode_t *node) {return node->ctn_num_nodes;}


/*
 *
 *  "PURE VIRTUAL" functions that must be implemented
 *  by the concrete subclass.
 *
 */

int lam_ctn_isa_neighbor(lam_ctnode_t *node, uint32_t label);
/*
 POST:   returns 1 if a node with specified label is a label for
 a neighbor node.  This does not imply that the get_neighbor() function
 would return non-NULL; it only verifies that the label is a valid label
 for a neighbor.
 */


uint32_t lam_ctn_label_for_link(lam_ctnode_t *node, uint32_t link);
/*
 PRE: The graph edges connecting node to its neighbors are oriented
        so that the links (edges) are numbered starting from 1.
 POST: Returns the label of neighbor connected to node via given link.
 */



/*
 *
 *  "PURE VIRTUAL" routing functions that must be implemented
 *  by the concrete subclass.
 *
 */


char *lam_ctn_initial_control_data(lam_ctnode_t *node, uint32_t *ctrl_size);
/*
 POST: Returns pointer to byte array for control data for routing
        messages.  The length of the control array is stored in
        ctrl_size.  Caller must free array.
 */



/*
 *
 *  Hypercube interface
 *
 */

typedef struct lam_hcube
{
    lam_ctnode_t    super;
    unsigned int    hc_hsize;           /* hc_hsize = log2(# nodes in network) */
} lam_hcube_t;

extern lam_class_info_t     hcube_cls;

#endif  /* CT_NODE_H */


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

#ifndef CT_MESSAGE_H
#define CT_MESSAGE_H

#include "lam/base/object.h"


/*
 *
 *      Available Classes
 *
 */

extern lam_class_info_t     lam_ctmsg_cls;

/*
 *
 *      CT Message interface
 *
 */

/*
 * Message control info for routing msgs.
 */

enum
{
    LAM_CT_BCAST = 1,
    LAM_CT_ALLGATHER,
    LAM_CT_SCATTER,
    LAM_CT_PT2PT
};

typedef struct lam_ct_ctrl
{
    uint8_t         ctc_is_user_msg;    /* 1 -> msg is for user app. */
    uint8_t         ctc_routing_type;   /* broadcast, scatter, pt2pt, etc. */
    uint16_t        ctc_len;
    uint8_t         *ctc_info;
} lam_ct_ctrl_t;



typedef struct lam_ctmsg
{
    lam_object_t    super;
    lam_ct_ctrl_t   ctm_ctrl;
    uint32_t        ctm_len;
    uint8_t         *ctm_data;
    int             ctm_should_free;
} lam_ctmsg_t;

#endif  /* CT_MESSAGE_H */



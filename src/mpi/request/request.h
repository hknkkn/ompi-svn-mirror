/*
 * $HEADER$
 */

#ifndef LAM_REQUEST_H
#define LAM_REQUEST_H

#include "mpi.h"
#include "lam/lfc/list.h"


extern lam_class_info_t lam_request_cls;

typedef enum {
    LAM_REQUEST_PML,
    LAM_REQUEST_IO,
    LAM_REQUEST_GEN,
    LAM_REQUEST_MAX
} lam_request_type_t;

struct lam_request_t {
    lam_list_item_t super;
    lam_request_type_t req_type;
};
typedef struct lam_request_t lam_request_t;

void lam_request_init(lam_request_t*);
void lam_request_destroy(lam_request_t*);

#endif


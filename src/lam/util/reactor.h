/*
 * $HEADER$
 */

#ifndef LAM_REACTOR_H
#define LAM_REACTOR_H

#include "lam/types.h"
#include "lam/lfc/list.h"
#include "lam/lfc/hash_table.h"
#include "lam/threads/mutex.h"

extern const int LAM_NOTIFY_ALL;
extern const int LAM_NOTIFY_RECV;
extern const int LAM_NOTIFY_SEND;
extern const int LAM_NOTIFY_EXCEPT;

extern lam_class_info_t lam_reactor_cls;


/*
 * Utilizes select() to provide callbacks when an event
 * (e.g. readable,writeable,exception) occurs on a designated
 * descriptor.  Objects interested in receiving callbacks must
 * implement the lam_reactor_listener_t interface.
 */

struct lam_reactor_listener_t;
typedef void (*rl_recv_handler_fn_t)(struct lam_reactor_listener_t *r, int sd);
typedef void (*rl_send_handler_fn_t)(struct lam_reactor_listener_t *r, int sd);
typedef void (*rl_except_handler_fn_t)(struct lam_reactor_listener_t *r, int sd);

struct lam_reactor_listener_t {
    rl_recv_handler_fn_t rl_recv_handler;
    rl_send_handler_fn_t rl_send_handler;
    rl_except_handler_fn_t rl_except_handler;
    void *rl_user_data;
};
typedef struct lam_reactor_listener_t lam_reactor_listener_t;


struct lam_reactor_descriptor_t {
    lam_list_item_t         rd_base;
    int                     rd;
    volatile int            rd_flags;
    lam_reactor_listener_t *rd_recv;
    lam_reactor_listener_t *rd_send;
    lam_reactor_listener_t *rd_except;
};
typedef struct lam_reactor_descriptor_t lam_reactor_descriptor_t;


void lam_reactor_descriptor_init(lam_reactor_descriptor_t*);
void lam_reactor_descriptor_destroy(lam_reactor_descriptor_t*);


struct lam_reactor_t {
    lam_object_t       r_base;
    lam_mutex_t        r_mutex;
    lam_list_t         r_active;
    lam_list_t         r_free;
    lam_list_t         r_pending;
    lam_fast_hash_t    r_hash;
    int                r_max;
    bool               r_run;
    int                r_changes;
    lam_fd_set_t       r_send_set;
    lam_fd_set_t       r_recv_set;
    lam_fd_set_t       r_except_set;
};
typedef struct lam_reactor_t lam_reactor_t;

 
void lam_reactor_init(lam_reactor_t*);
void lam_reactor_destroy(lam_reactor_t*);

bool lam_reactor_insert(lam_reactor_t*, int sd, lam_reactor_listener_t*, int flags);
bool lam_reactor_remove(lam_reactor_t*, int sd, lam_reactor_listener_t*, int flags);
void lam_reactor_poll(lam_reactor_t*);
void lam_reactor_run(lam_reactor_t*);
void lam_reactor_dispatch(lam_reactor_t* r, int cnt, lam_fd_set_t* rset, lam_fd_set_t* sset, lam_fd_set_t* eset);

#endif /* LAM_REACTOR_H */


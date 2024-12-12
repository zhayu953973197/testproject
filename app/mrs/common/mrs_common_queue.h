/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS common queue.
 */

#ifndef __MRS_COMMON_QUEUE_H__
#define __MRS_COMMON_QUEUE_H__

#include <soc_types.h>

#ifdef __cplusplus
extern "C" {
#endif


#define mrs_lock_init(x)   ((x) = 0)
#define mrs_lock(x)        ((x) = 1)
#define mrs_unlock(x)      ((x) = 0)
#define mrs_try_lock(x)    (((x) == 0) ? ((x) = 1) : 0)
#define mrs_lock_status(x) (x)

typedef struct mrs_queue_node {
    struct mrs_queue_node *next;
} mrs_queue_node;

typedef struct mrs_srv_queue {
    mrs_queue_node *head;
    mrs_queue_node *tail;
} mrs_srv_queue;

td_void mrs_queue_init(mrs_srv_queue *queue);
td_void mrs_queue_enqueue(mrs_srv_queue *queue, mrs_queue_node *node);
mrs_queue_node *mrs_queue_dequeue(mrs_srv_queue *queue);
mrs_queue_node *mrs_queue_top(const mrs_srv_queue *queue);
td_u32 mrs_queue_length(const mrs_srv_queue *queue);
td_bool mrs_queue_is_empty(const mrs_srv_queue *queue);
td_void mrs_queue_empty(mrs_srv_queue *queue, td_void (*free_node)(td_void *));
td_void mrs_queue_traverse(const mrs_srv_queue *queue, td_bool(*visit)(td_void *, td_void *),
                           td_void *extra);
mrs_queue_node *mrs_queue_replace(mrs_srv_queue *queue, EXT_CONST mrs_queue_node *old_node, mrs_queue_node *new_node);

#ifdef __cplusplus
}
#endif
#endif /* __MRS_COMMON_QUEUE_H__ */

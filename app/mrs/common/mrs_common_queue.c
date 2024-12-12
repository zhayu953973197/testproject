/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS common queue.
 */

#include "mrs_common_queue.h"

td_void mrs_queue_init(mrs_srv_queue *queue)
{
    queue->head = TD_NULL;
    queue->tail = TD_NULL;
}

td_void mrs_queue_enqueue(mrs_srv_queue *queue, mrs_queue_node *node)
{
    node->next = TD_NULL;

    if (queue->head == TD_NULL) {
        queue->head = node;
    } else {
        queue->tail->next = node;
    }

    queue->tail = node;
}

mrs_queue_node *mrs_queue_dequeue(mrs_srv_queue *queue)
{
    mrs_queue_node *node = TD_NULL;

    if (queue->head == TD_NULL) {
        return TD_NULL;
    }

    node = queue->head;

    if (queue->head == queue->tail) {
        queue->head = TD_NULL;
        queue->tail = TD_NULL;
    } else {
        queue->head = queue->head->next;
    }

    return node;
}

mrs_queue_node *mrs_queue_top(const mrs_srv_queue *queue)
{
    return queue->head;
}

td_u32 mrs_queue_length(const mrs_srv_queue *queue)
{
    td_u32 length = 0;
    mrs_queue_node *node = queue->head;

    while (node != TD_NULL) {
        length++;
        node = node->next;
    }

    return length;
}

td_bool mrs_queue_is_empty(const mrs_srv_queue *queue)
{
    return (td_bool)(queue->head == TD_NULL);
}

td_void mrs_queue_empty(mrs_srv_queue *queue, td_void (*free_node)(td_void *))
{
    mrs_queue_node *node = TD_NULL;

    while (queue->head != TD_NULL) {
        node = mrs_queue_dequeue(queue);
        if (free_node != TD_NULL) {
            free_node((td_void *)node);
        }
    }
}

td_void mrs_queue_traverse(const mrs_srv_queue *queue, td_bool (*visit)(td_void *, td_void *), td_void *extra)
{
    mrs_queue_node *node = queue->head;

    if (visit == TD_NULL) {
        return;
    }

    while (node != TD_NULL) {
        if (visit((td_void *)node, extra) == TD_TRUE) {
            break;
        }

        node = node->next;
    }
}

mrs_queue_node *mrs_queue_replace(mrs_srv_queue *queue, EXT_CONST mrs_queue_node *old_node, mrs_queue_node *new_node)
{
    mrs_queue_node *node = TD_NULL;
    mrs_queue_node temp = { TD_NULL };

    if ((queue == TD_NULL) || (old_node == TD_NULL) || (new_node == TD_NULL)) {
        return TD_NULL;
    }

    temp.next = queue->head;
    node = &temp;

    while (node != TD_NULL) {
        if (node->next == old_node) {
            node->next = new_node;
            new_node->next = old_node->next;
            break;
        }

        node = node->next;
    }

    if (old_node == queue->head) {
        queue->head = new_node;
    }

    if (old_node == queue->tail) {
        queue->tail = new_node;
    }

    if (node == TD_NULL) {
        return TD_NULL;
    }

    return (mrs_queue_node *)old_node;
}

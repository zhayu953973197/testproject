/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS STA queue handle.
 */

#include "mrs_sta_queue.h"
#include "mrs_sta_srv.h"
#include "mrs_config.h"
#include "mrs_common_tools.h"
#include "mrs_common_uart.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)
typedef struct {
    mrs_mr_queue_item *item;
    mrs_sta_mr_queue *queue;
    td_u8 result;
    td_u8 rsv[3]; /* reserved 3 bytes */
} mr_queue_param;

/* replace queue elements */
td_bool mrs_sta_queue_replace(td_pvoid p, td_pvoid q);

td_u32 mrs_sta_queue_init(mrs_sta_mr_queue *queue)
{
    if (queue == TD_NULL) {
        return EXT_ERR_FAILURE;
    }

    mrs_lock_init(queue->lock);
    mrs_timer_stop(MRS_TIMER_ID_STA_QUEUE_LOCK);

    mrs_queue_init(&queue->mr_queue);
    queue->retry = 0;

    return EXT_ERR_SUCCESS;
}

td_bool mrs_sta_try_enqueue(EXT_CONST mrs_sta_mr_queue *queue, EXT_CONST mrs_mr_queue_item *item)
{
    mrs_srv_queue *q = TD_NULL;
    mr_queue_param param;
    td_u8 q_len;

    if ((queue == TD_NULL) || (item == TD_NULL)) {
        return TD_FALSE;
    }

    if (mrs_sta_plc_retry_filter(item) == TD_TRUE) {
        /* forbid repeating PLC frame to join the queue */
        return TD_FALSE;
    }

    q = (mrs_srv_queue *)&queue->mr_queue;
    param.item = (mrs_mr_queue_item *)item;
    param.queue = (mrs_sta_mr_queue *)queue;
    param.result = TD_FALSE;
    mrs_queue_traverse(q, mrs_sta_queue_replace, (td_void *)&param);
    if (param.result == TD_TRUE) {
        return TD_TRUE;
    }

    q_len = (td_u8)mrs_queue_length(q);
    if (q_len >= MRS_CFG_STA_QUEUE_MAX) {
        return TD_FALSE;
    }

    mrs_queue_enqueue(q, (mrs_queue_node *)&item->link);
    if (q_len == 0) {
        mrs_sta_queue_notify();
    }

    return TD_TRUE;
}

td_bool mrs_sta_try_dequeue(mrs_sta_mr_queue *queue)
{
    mrs_mr_queue_item *item = TD_NULL;

    if (queue == TD_NULL) {
        return TD_FALSE;
    }

    item = (mrs_mr_queue_item *)mrs_queue_dequeue(&queue->mr_queue);
    mrs_free(item);

    queue->retry = 0;
    mrs_unlock(queue->lock);
    mrs_timer_stop(MRS_TIMER_ID_STA_QUEUE_LOCK);
    mrs_sta_queue_notify();
    return TD_TRUE;
}

td_bool mrs_sta_queue_replace(td_pvoid p, td_pvoid q)
{
    mrs_mr_queue_item *item = (mrs_mr_queue_item *)p;
    mr_queue_param *param = (mr_queue_param *)q;

    if ((item == TD_NULL) || (param == TD_NULL)) {
        return TD_TRUE;
    }

    param->result = TD_FALSE;
    if ((param->item->id == item->id) && (param->item->seq == item->seq)) {
        if ((item == (mrs_mr_queue_item *)mrs_queue_top(&param->queue->mr_queue)) &&
            (mrs_lock_status(param->queue->lock))) {
            param->result = TD_FALSE;
        } else {
            mrs_queue_replace(&param->queue->mr_queue, &item->link, &param->item->link);
            mrs_free(item);
            param->result = TD_TRUE;
        }
    }

    return param->result;
}

td_void mrs_sta_queue_handle(mrs_sta_mr_queue *queue)
{
    mrs_mr_queue_item *item = TD_NULL;
    td_u32 ret;

    if (mrs_try_lock(queue->lock) == 0) {
        return;
    }

    mrs_timer_start(MRS_TIMER_ID_STA_QUEUE_LOCK, MRS_STA_MR_QUEUE_LOCK_TIME, EXT_TIMER_TYPE_ONCE);

    item = (mrs_mr_queue_item *)mrs_queue_top(&queue->mr_queue);
    if (item == TD_NULL) {
        mrs_unlock(queue->lock);
        mrs_timer_stop(MRS_TIMER_ID_STA_QUEUE_LOCK);
        return;
    }

    if (queue->retry > item->try_max) {
        mrs_sta_try_dequeue(queue);
        return;
    }

    queue->retry++;

    /* send before send formal data */
    ret = mrs_uart_tx((td_pbyte) "\xFE\xFE\xFE\xFE", (td_u16)strlen("\xFE\xFE\xFE\xFE"));
    ret |= mrs_uart_tx(item->data, item->data_len);
    if (ret != EXT_ERR_SUCCESS) {
        mrs_sta_try_dequeue(queue);
        return;
    }

    mrs_timer_start(MRS_TIMER_ID_STA_TTL, (td_u16)item->timeout * 100, EXT_TIMER_TYPE_ONCE); /* 100ms */
}

td_void mrs_sta_queue_unlock(mrs_sta_mr_queue *queue)
{
    if (queue == TD_NULL) {
        return;
    }

    mrs_unlock(queue->lock);
    mrs_timer_stop(MRS_TIMER_ID_STA_QUEUE_LOCK);
}

#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_STA) */

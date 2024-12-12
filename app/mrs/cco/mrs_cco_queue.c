/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS CCO queue.
 */

#include "mrs_cco_queue.h"
#include "mrs_msg.h"
#include "mrs_common_tools.h"
#include "mrs_common_uart.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)

mrs_cco_queue g_cco_queue;

td_void mrs_cco_queue_init(td_void)
{
    mrs_queue_init(&g_cco_queue.queue);
}

mrs_cco_queue *mrs_get_cco_queue(td_void)
{
    return &g_cco_queue;
}

td_u8 mrs_cco_queue_get_fsm(td_void)
{
    mrs_cco_queue *q = mrs_get_cco_queue();
    return q->fsm;
}

td_void mrs_cco_queue_set_fsm(td_u8 fsm)
{
    mrs_cco_queue *q = mrs_get_cco_queue();
    q->fsm = fsm;
}

td_void mrs_cco_queue_enqueue(mrs_cco_queue_item *item)
{
    mrs_cco_queue *q = mrs_get_cco_queue();

    mrs_queue_enqueue(&q->queue, &item->link);
    q->num++;
}

td_void mrs_cco_queue_set_item_invalid(td_u8 afn, td_u8 fn)
{
    mrs_cco_queue *q = mrs_get_cco_queue();
    mrs_cco_queue_item *item = (mrs_cco_queue_item *)mrs_queue_top(&q->queue);
    mrs_queue_node *it;

    if (item == TD_NULL) {
        mrs_unlock(q->lock);
        return;
    }

    if ((item->afn == afn) && (item->fn == fn)) {
        mrs_timer_stop(MRS_TIMER_ID_CCO_TO_HOST);
        mrs_cco_queue_end(q);
    }

    for (it = q->queue.head; it != TD_NULL; it = it->next) {
        item = (mrs_cco_queue_item *)it;
        if ((item->afn == afn) && (item->fn == fn)) {
            item->valid = TD_FALSE;
        }
    }
}

td_void mrs_cco_queue_end(mrs_cco_queue *q)
{
    mrs_cco_queue_item *item = TD_NULL;

    mrs_unlock(q->lock);
    /* reset status of queue */
    mrs_cco_queue_set_fsm(FSM_CCO_QUEUE_IDLE);

    item = (mrs_cco_queue_item *)mrs_queue_dequeue(&q->queue);
    mrs_free(item);

    q->num--;
    q->retry = 0;

    mrs_cco_queue_active();
}

td_void mrs_msg_on_queue_notify(EXT_CONST mrs_queue_msg *msg)
{
    mrs_cco_queue *q = mrs_get_cco_queue();
    mrs_cco_queue_item *item = TD_NULL;

    ext_unref_param(msg);

    if (mrs_try_lock(q->lock) == 0) {
        return;
    }

    item = (mrs_cco_queue_item *)mrs_queue_top(&q->queue);
    if (item == TD_NULL) {
        mrs_unlock(q->lock);
        return;
    }

    if (item->valid != TD_TRUE) {
        /* remove invalid element */
        mrs_cco_queue_end(q);
        return;
    }

    q->retry = 0;
    mrs_uart_tx(item->data, item->data_len);
    /* update status, wait for response */
    mrs_cco_queue_set_fsm(FSM_CCO_QUEUE_WAIT_ACK);
    mrs_timer_start(MRS_TIMER_ID_CCO_TO_HOST, item->timeout, EXT_TIMER_TYPE_ONCE);
}

td_void mrs_cco_queue_on_timer(td_void)
{
    mrs_cco_queue *q = mrs_get_cco_queue();
    mrs_cco_queue_item *item = TD_NULL;
    mrs_cco_queue_item *temp = TD_NULL;

    item = (mrs_cco_queue_item *)mrs_queue_top(&q->queue);
    if (item == TD_NULL) {
        mrs_unlock(q->lock);
        return;
    }

    if (q->retry < item->retry_max) {
        if (item->single_timeout != TD_NULL) {
            if (item->single_timeout(item->param) != EXT_ERR_CONTINUE) {
                mrs_cco_queue_end(q);
                return;
            }
        }

        q->retry++;
        mrs_uart_tx(item->data, item->data_len);
        mrs_cco_queue_set_fsm(FSM_CCO_QUEUE_WAIT_ACK);
        mrs_timer_start(MRS_TIMER_ID_CCO_TO_HOST, item->timeout, EXT_TIMER_TYPE_ONCE);
        return;
    }

    if (item->total_timeout != TD_NULL) {
        item->total_timeout(item->param);
    }

    temp = (mrs_cco_queue_item *)mrs_queue_top(&q->queue);
    if (temp == item) {
        mrs_cco_queue_end(q);
    }
}

#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_CCO) */


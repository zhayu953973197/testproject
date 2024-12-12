/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS CCO queue.
 */

#ifndef __MRS_CCO_QUEUE_H__
#define __MRS_CCO_QUEUE_H__

#include "mrs_common_queue.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
#ifdef __cplusplus
extern "C" {
#endif

enum {
    FSM_CCO_QUEUE_IDLE,
    FSM_CCO_QUEUE_WAIT_ACK,
};

typedef struct {
    mrs_srv_queue queue;
    td_u8 num;
    td_u8 lock;
    td_u8 retry;
    td_u8 fsm;
} mrs_cco_queue;

typedef struct {
    mrs_queue_node link;
    td_bool valid;
    td_u8 afn;
    td_u8 fn;
    td_u8 retry_max;
    td_u16 data_len;
    td_u16 timeout;
    td_pvoid param;
    td_void (*rx_handle)(td_pvoid);
    td_u32 (*single_timeout)(td_pvoid);
    td_u32 (*total_timeout)(td_pvoid);
    td_u8 data[0];
} mrs_cco_queue_item;

#define mrs_cco_queue_active() mrs_msg_queue_send(MRS_MSG_ID_QUEUE_NOTIFY, 0, 0, 0)

/* initialization for CCO interactive queue */
td_void mrs_cco_queue_init(td_void);
/* get queue */
mrs_cco_queue *mrs_get_cco_queue(td_void);
/* get bytes of queue status */
td_u8 mrs_cco_queue_get_fsm(td_void);
/* set bytes of queue status */
td_void mrs_cco_queue_set_fsm(td_u8 fsm);
/* join queue */
td_void mrs_cco_queue_enqueue(mrs_cco_queue_item *item);
/* mark queue items with invalid flag */
td_void mrs_cco_queue_set_item_invalid(td_u8 afn, td_u8 fn);
/* clear queue */
td_void mrs_cco_queue_end(mrs_cco_queue *q);
/* periodically check interactive queue */
td_void mrs_cco_queue_on_timer(td_void);

#ifdef __cplusplus
}
#endif

#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_CCO) */
#endif /* __MRS_COMMON_UART_H__ */


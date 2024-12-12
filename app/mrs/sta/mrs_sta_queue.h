/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS STA queue handle.
 */

#ifndef __MRS_STA_QUEUE_H__
#define __MRS_STA_QUEUE_H__

#include "mrs_common_queue.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)
#ifdef __cplusplus
extern "C" {
#endif


#define mrs_sta_queue_notify() mrs_msg_queue_send(MRS_MSG_ID_TIMER, 0, MRS_TIMER_ID_STA_QUEUE, 0)

#define MRS_MR_QUEUE_MAX_RETRY 2

#define MRS_STA_ITEM_ID_DETECT    0xFFFF
#define MRS_STA_ITME_ID_EVENT_QRY 0xFFFE

typedef struct mrs_mr_queue_item {
    mrs_queue_node link;
    td_u16 id;
    td_u16 seq;
    td_u8 protocol;
    td_u8 timeout;
    td_u8 no_rsp_flg : 1;
    td_u8 nak_flag : 1;
    td_u8 rsv0 : 2;
    td_u8 try_max : 4;
    td_u8 status;
    td_void (*rx_handle)(EXT_CONST struct mrs_mr_queue_item *item, td_pbyte data, td_u16 data_len);
    td_void (*timeout_handle)(td_void);
    td_u8 mac_addr[EXT_METER_ADDR_LEN];
    td_u16 data_len;
    td_u8 data[0];
} mrs_mr_queue_item;

typedef struct {
    mrs_srv_queue mr_queue;
    td_u8 retry;
    td_u8 lock;
    td_u16 rsv;
} mrs_sta_mr_queue;

/* initialization for message recieving queue */
td_u32 mrs_sta_queue_init(mrs_sta_mr_queue *queue);
/* join the queue */
td_bool mrs_sta_try_enqueue(EXT_CONST mrs_sta_mr_queue *queue, EXT_CONST mrs_mr_queue_item *item);
/* remove the element from the queue */
td_bool mrs_sta_try_dequeue(mrs_sta_mr_queue *queue);
/* send element of queue */
td_void mrs_sta_queue_handle(mrs_sta_mr_queue *queue);
/* unlock the queue */
td_void mrs_sta_queue_unlock(mrs_sta_mr_queue *queue);

#ifdef __cplusplus
}
#endif

#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_STA) */
#endif /* __MRS_STA_QUEUE_H__ */


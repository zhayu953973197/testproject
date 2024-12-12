/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS CCO EVENT report.
 */

#ifndef __MRS_CCO_EVT_H__
#define __MRS_CCO_EVT_H__

#include "mrs_common_plc.h"
#include "mrs_common_queue.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
#ifdef __cplusplus
extern "C" {
#endif


/* enable for CCO power down detection */
#define MRS_CCO_DETECT_POWER_FAILURE_ENABLE (1)

typedef struct {
    mrs_queue_node link;
    td_u8 meter[EXT_METER_ADDR_LEN];
    td_u16 seq;
    td_u8 fn;
    td_u8 rsv;
    td_u16 data_len;
    td_u8 data[0];
} mrs_cco_evt_item;

typedef struct {
    ext_list link;
    td_u8 meter[EXT_METER_ADDR_LEN];
    td_u16 seq;
    td_u32 time;
} mrs_cco_evt_reported_item;

typedef struct {
    ext_list evt_reported_list; /* < Reported events list */
    mrs_srv_queue evt_q;       /* < Event queue */
    td_u8 evt_q_num;           /* < Valid event in the queue */
    td_u8 evt_q_lock;          /* < Locker of event queue */
    td_u8 report_list_num;     /* < Valid event number of reported events list */
    td_bool acd_flg;           /* < Un-report event flag */
} mrs_cco_evt_ctx;

/* Get event global control context */
mrs_cco_evt_ctx *mrs_cco_get_evt_ctx(td_void);
/* Initial event global control context */
td_void mrs_cco_event_init(td_void);
#define mrs_cco_evt_report_active() mrs_msg_queue_send(MRS_MSG_ID_EVT_NOTIFY, 0, 0, 0)

#ifdef __cplusplus
}
#endif

#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_CCO) */
#endif /* __MRS_CCO_EVT_H__ */

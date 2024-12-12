/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS STA EVENT report.
 */

#ifndef __MRS_STA_EVT_H__
#define __MRS_STA_EVT_H__

#include "mrs_common_plc.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)
#ifdef __cplusplus
extern "C" {
#endif

typedef td_void (*mrs_sta_evt_callback)(td_void);

typedef struct {
    td_u8 hard_evt_out;
    td_u8 evt_out;
    td_u8 timeout;
    td_u8 timeout_max;
    td_u8 retry;
    td_u8 max_retry;
    td_u16 seq;
    td_u16 sta_queue_seq;
    td_u8  pad[2]; /* 2:reserved bytes */
    mrs_plc_frame_data *report_frame;

    mrs_sta_evt_callback query_notify;
    mrs_sta_evt_callback plc_ack_notify;
    mrs_sta_evt_callback trigger_notify;
} mrs_sta_evt_ctx;

/* Event initial */
td_u32 mrs_sta_event_init(td_void);

/* Handler of event report timeout */
td_void mrs_sta_event_report_plc_timeout_handle(td_void);
mrs_sta_evt_ctx *mrs_sta_get_evt_ctx(td_void);

/* Register query EVENT_OUT notify */
td_void mrs_sta_event_register_query_notify(mrs_sta_evt_callback notify);

/* Register PLC ACK notify */
td_void mrs_sta_event_register_plc_ack_notify(mrs_sta_evt_callback notify);

/* Register EVENT_OUT trigger notify */
td_void mrs_sta_event_register_trigger_notify(mrs_sta_evt_callback notify);

#ifdef __cplusplus
}
#endif

#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_STA) */
#endif /* __MRS_STA_EVT_H__ */


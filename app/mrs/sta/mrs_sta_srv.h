/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS sta service.
 */

#ifndef MRS_STA_SERVICE_H
#define MRS_STA_SERVICE_H

#include "mrs_sta_queue.h"
#include "mrs_config.h"
#include "mrs_proto_645.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)
#ifdef __cplusplus
extern "C" {
#endif

#define MRS_STA_STATUS_INIT 0
#define MRS_STA_STATUS_MR   1

#define mrs_is_product_relay() (uapi_get_product_type() == EXT_FTM_PRODUCT_TYPE_M108)

typedef td_u32 (*mrs_sta_chl_notify_fct)(td_bool chl_status, uintptr_t param, td_bool *one_shot);

/* elements in frame cache. cache used for avoiding redundant frame */
typedef struct {
    td_u16 id;
    td_u16 seq;
    td_u16 data_len;
    td_u8 valid;
    td_u8 protocol;
    td_u32 timestamp;
    td_u8 data[MRS_645_FRAME_LEN_MAX];
} mrs_sta_frame_buf_item;

/* frame cache. cache used for avoiding redundant frame */
typedef struct {
    td_u8 current;
    td_u8 rsv[3]; /* reserved 3B */
    mrs_sta_frame_buf_item item[MRS_CFG_STA_FRAME_BUF_CNT];
} mrs_sta_frame_buf_ctx;

/* STA control unit */
typedef struct {
    td_u8 meter[MRS_METER_ADDR_LEN];
    td_u8 mac[EXT_PLC_MAC_ADDR_LEN];

    td_u32 baudrate;

    td_u8 protocol;
    td_u8 status;
    td_u16 plc_seq;

    td_u8 chl_status : 1;
    td_u8 sync_mac : 1;
    td_u8 rsv0 : 6;
    td_u8 rsv1[3]; /* reserved 3B */

    mrs_sta_chl_notify_fct chl_notify_handle;
    uintptr_t chl_notify_param;

    mrs_sta_mr_queue mr_queue;
    mrs_sta_frame_buf_ctx frame_buf;
    mrs_on_timer_fct on_timer[MRS_TIMER_ID_MAX];
} mrs_sta_srv_ctx;

/* get control unit */
mrs_sta_srv_ctx *mrs_sta_get_srv_ctx(td_void);
/* get message queue */
mrs_sta_mr_queue *mrs_sta_get_queue(td_void);
/* STA service initialization */
td_u32 mrs_sta_srv_init(td_void);
/* set STA status */
td_void mrs_sta_set_status(td_u8 status);
/* get STA status */
td_u8 mrs_sta_get_status(td_void);

td_u8 mrs_sta_get_meter_protocol(td_void);
td_void mrs_sta_set_meter_protocol(td_u8 proto);

/* register notify function */
td_u32 mrs_sta_register_chl_notify(mrs_sta_chl_notify_fct callback, uintptr_t param);
/* clear notify function */
td_u32 mrs_sta_clear_chl_notify(mrs_sta_chl_notify_fct callback);
/* call notify function */
td_u32 mrs_sta_chl_notify_handle(td_bool chl_status);

td_void mrs_sta_set_sync_mac_flag(td_bool flag);
td_bool mrs_sta_get_sync_mac_flag(td_void);

/* add elements to frame cache */
td_u32 mrs_sta_frame_buf_add(const mrs_mr_queue_item *item, const td_pbyte data, td_u16 data_len);
/* find the same frame in frame cache */
td_u32 mrs_sta_frame_buf_find(const mrs_mr_queue_item *item, td_pbyte *data, td_u16 *data_len);
/* filter frame */
td_bool mrs_sta_plc_retry_filter(EXT_CONST mrs_mr_queue_item *item);
td_void mrs_sta_register_on_timer_func(mrs_timer_id timer_id, mrs_on_timer_fct on_timer);
#define mrs_srv_init() mrs_sta_srv_init()

#ifdef __cplusplus
}
#endif

#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_STA) */
#endif /* __MRS_STA_SERVICE_H__ */

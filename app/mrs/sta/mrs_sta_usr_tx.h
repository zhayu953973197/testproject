/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS sta uart sending list.
 */

#ifndef __MRS_STA_USR_TX_H__
#define __MRS_STA_USR_TX_H__

#include "mrs_config.h"
#include "mrs_msg.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)
#ifdef __cplusplus
extern "C" {
#endif


#define MRS_USR_TX_MAX_RETRY 0

/* Low to High */
enum {
    TX_ITEM_PRI_0 = 0,
    TX_ITEM_PRI_1,
    TX_ITEM_PRI_2,
    TX_ITEM_PRI_INVALID
};

typedef struct {
    td_u16 id;
    td_u16 seq;
    td_u8 protocol;
    td_u8 timeout;
    td_u8 addr[MRS_METER_ADDR_LEN];
} mrs_sta_usr_frame_head;

typedef struct mrs_usr_tx_item {
    EXT_LIST link;
    mrs_sta_usr_frame_head frame_head;
    td_u8 no_resp;
    td_u8 priority;
    td_u8 retry;
    td_u8 usr_id;
    td_u16 rsv;
    td_u16 data_len;
    td_u8 data[0];
} mrs_usr_tx_item;

td_u32 mrs_sta_usr_tx_queue_init(td_void);
td_bool mrs_sta_usr_tx(mrs_usr_tx_item *item);
td_void mrs_sta_usr_tx_handle(td_void);
td_void mrs_sta_usr_tx_unlock(td_void);
mrs_usr_tx_item *mrs_sta_usr_get_tx_item(td_void);
td_void mrs_usr_tx_dequeue(td_void);

/* PLC */
td_u32 mrs_sta_usr_xr_plc_tx(mrs_usr_tx_item *item, td_pbyte data, td_u16 data_len);
td_u32 mrs_sta_usr_pr_plc_tx(mrs_usr_tx_item *item, td_pbyte data, td_u16 data_len);
td_u32 mrs_sta_usr_ciu_plc_tx(mrs_usr_tx_item *item, td_pbyte data, td_u16 data_len);
td_bool mrs_sta_usr_plc_filter(mrs_sta_usr_frame_head *head);

#ifdef __cplusplus
}
#endif

#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_STA) */
#endif /* __MRS_STA_USR_TX_H__ */


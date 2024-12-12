/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS STA plc handle.
 */

#ifndef __MRS_STA_PLC_H__
#define __MRS_STA_PLC_H__

#include "mrs_common_plc.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)
#ifdef __cplusplus
extern "C" {
#endif

typedef td_void (*mrs_sta_plc_rx_callback)(EXT_CONST mrs_plc_frame_data *plc);

td_void mrs_sta_register_plc_rx_xr_notify(mrs_sta_plc_rx_callback notify);
td_void mrs_sta_register_plc_rx_pr_notify(mrs_sta_plc_rx_callback notify);
td_void mrs_sta_register_plc_rx_bc_notify(mrs_sta_plc_rx_callback notify);
td_void mrs_sta_register_plc_rx_ciu_notify(mrs_sta_plc_rx_callback notify);

/* processing for recieving  transparent PLC frame */
td_void mrs_sta_xr_plc_frame_rx(EXT_CONST mrs_plc_frame_data *plc);

/* processing for recieving parallel PLC frame */
td_void mrs_sta_pr_plc_frame_rx(EXT_CONST mrs_plc_frame_data *plc);

/* processing for recieving braodcast PLC frame */
td_void mrs_broadcast_plc_frame_rx(EXT_CONST mrs_plc_frame_data *plc);

/* PLC channel status */
td_bool mrs_sta_plc_chl_status(td_void);

/* set STA address */
td_void mrs_sta_set_mac_addr(const td_u8 mac[EXT_METER_ADDR_LEN]);

/* Received event on PLC */
td_void mrs_sta_evt_plc_frame_rx(const mrs_plc_frame_data *plc);

td_void mrs_sta_tf_plc_frame_rx(EXT_CONST mrs_plc_frame_data *plc);

/* CIU frame */
td_void mrs_sta_ciu_plc_frame_rx(EXT_CONST mrs_plc_frame_data *plc);

/* query station info */
td_void mrs_sta_qry_plc_frame_rx(const mrs_plc_frame_data *plc);

/* STA equip test frame */
td_void mrs_sta_dut_plc_frame_rx(EXT_CONST mrs_plc_frame_data *plc);

#define mrs_xr_plc_frame_rx(plc)  mrs_sta_xr_plc_frame_rx(plc)
#define mrs_pr_plc_frame_rx(plc)  mrs_sta_pr_plc_frame_rx(plc)
#define mrs_bc_plc_frame_rx(plc)  mrs_broadcast_plc_frame_rx(plc)
#define mrs_evt_plc_frame_rx(plc) mrs_sta_evt_plc_frame_rx(plc)
#define mrs_tf_plc_frame_rx(plc)  mrs_sta_tf_plc_frame_rx(plc)
#define mrs_ciu_plc_frame_rx(plc) mrs_sta_ciu_plc_frame_rx(plc)
#define mrs_qry_plc_frame_rx(plc) mrs_sta_qry_plc_frame_rx(plc)
#define mrs_cco_dut_plc_frame_rx(plc)

#ifdef __cplusplus
}
#endif

#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_STA) */
#endif /* __MRS_STA_PLC_H__ */
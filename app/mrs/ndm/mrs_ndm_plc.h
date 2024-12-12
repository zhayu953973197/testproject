/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS STA plc handle.
 */

#ifndef __MRS_NDM_PLC_H__
#define __MRS_NDM_PLC_H__

#include "mrs_common_plc.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_NDM)
#ifdef __cplusplus
extern "C" {
#endif

td_void mrs_ndm_send_cco_dut_plc_frame(const td_pbyte data, const td_u16 data_len,
                                       const td_pbyte meter, const td_u8 meter_len);

td_void mrs_ndm_send_sta_dut_plc_frame(const td_pbyte data, const td_u16 data_len,
                                       const td_pbyte meter, const td_u8 meter_len, td_bool is_no_mac);

td_void mrs_ndm_send_ciu_plc_frame(const td_pbyte data, const td_u16 data_len,
                                   const td_pbyte meter, const td_u8 meter_len, td_bool is_no_mac);

td_void mrs_ndm_ciu_plc_frame_rx(mrs_plc_frame_data *plc);
td_void mrs_ndm_cco_dut_plc_frame_rx(mrs_plc_frame_data *plc);
td_void mrs_ndm_sta_dut_plc_frame_rx(mrs_plc_frame_data *plc);
td_void mrs_ndm_dut_init(td_void);

#define mrs_xr_plc_frame_rx(plc)
#define mrs_pr_plc_frame_rx(plc)
#define mrs_evt_plc_frame_rx(plc)
#define mrs_bc_plc_frame_rx(plc)
#define mrs_tf_plc_frame_rx(plc)
#define mrs_ciu_plc_frame_rx(plc) mrs_ndm_ciu_plc_frame_rx(plc)
#define mrs_qry_plc_frame_rx(plc)
#define mrs_cco_dut_plc_frame_rx(plc) mrs_ndm_cco_dut_plc_frame_rx(plc)
#define mrs_sta_dut_plc_frame_rx(plc) mrs_ndm_sta_dut_plc_frame_rx(plc)

#ifdef __cplusplus
}
#endif
#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_NDM) */
#endif

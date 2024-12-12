/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS DFX.
 */

#ifndef __MRS_DFX_H__
#define __MRS_DFX_H__

#include "mrs_common_plc.h"

#ifdef __cplusplus
extern "C" {
#endif


td_u32 mrs_diag_dfx(td_u16 id, td_pvoid cmd, td_u16 cmd_size, td_u8 option);

td_void mrs_dfx_init(td_void);

td_void mrs_dfx_uart_chl_rx(const td_u8 * const data, td_u16 data_len, td_bool msg_proc);
td_void mrs_dfx_uart_chl_err_rx(td_u16 data_len, td_u32 err_cause, td_bool msg_proc);
td_void mrs_dfx_uart_chl_rx_frame(td_u16 data_len, td_u32 err_cause, td_bool msg_proc);
td_void mrs_dfx_uart_chl_tx(const td_u8 *data, td_u16 data_len);
td_void mrs_dfx_uart_chl_tx_ok_cnt(td_u16 data_len);
td_void mrs_dfx_uart_chl_tx_err_cnt(td_u16 data_len, td_u32 err_cause);

td_void mrs_dfx_plc_chl_rx(const ext_dms_chl_rx *rx);
td_void mrs_dfx_plc_chl_err_rx(const ext_dms_chl_rx *rx, td_u32 err_cause);
td_void mrs_dfx_plc_rx_proc(const mrs_plc_frame_data *rx);
td_void mrs_dfx_plc_rx_err_proc(const mrs_plc_frame_data *rx, td_u32 err_cause);
td_void mrs_dfx_plc_chl_tx(const mrs_plc_frame_data *tx, td_bool msg_proc);
td_void mrs_dfx_plc_chl_err_tx(const mrs_plc_frame_data *tx, td_bool msg_proc, td_u32 err_cause);

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
td_u32 mrs_dfx_tf(td_u16 id, td_pvoid cmd, td_u16 cmd_size, td_u8 option);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __MRS_DFX_H__ */

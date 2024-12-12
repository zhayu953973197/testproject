/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS NDM 645 handle.
 */

#ifndef __MRS_NDM_645_H__
#define __MRS_NDM_645_H__

#include "mrs_common_uart.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_NDM)
#ifdef __cplusplus
extern "C" {
#endif

#define MRS_DUT_645_CTRL_FN_CCO  (0x06)
#define MRS_DUT_645_CTRL_FN_STA  (0x07)
#define MRS_DUT_645_CTRL_FN_NDM  (0x09)
#define MRS_DUT_645_CTRL_FN_CIU  (0x1F)
#define MRS_DUT_645_CTRL_FN_MASK (0x1F)

td_void mrs_ndm_cco_dut_uart_frame(td_pbyte data, td_u16 data_len, const td_pbyte addr, td_u8 addr_len);

#define mrs_ndm_sta_dut_uart_frame(data, data_len) mrs_uart_tx((data), (data_len))
#define mrs_ndm_ciu_uart_frame(data, data_len)     mrs_uart_tx((data), (data_len))

#ifdef __cplusplus
}
#endif
#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_NDM) */
#endif /* __MRS_NDM_645_H__ */

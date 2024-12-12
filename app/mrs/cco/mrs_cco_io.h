/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS STA I/O.
 */

#ifndef __MRS_CCO_IO_H__
#define __MRS_CCO_IO_H__

#include "mrs_common_tools.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
#ifdef __cplusplus
extern "C" {
#endif


#define MRS_PLC_LED_RX EXT_LED_IDX_0
#define MRS_PLC_LED_TX EXT_LED_IDX_1

/* CCO IO initialization */
td_u32 mrs_cco_io_init(td_void);

#define mrs_io_init()        mrs_cco_io_init()
#define mrs_plc_rx_led_off() uapi_led_off(MRS_PLC_LED_RX)
#define mrs_plc_tx_led_off() uapi_led_off(MRS_PLC_LED_TX)

td_void mrs_plc_tx_phase_a_off(td_void);
td_void mrs_plc_tx_phase_b_off(td_void);
td_void mrs_plc_tx_phase_c_off(td_void);

td_void mrs_cco_phase_init(td_void);

#ifdef __cplusplus
}
#endif

#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_CCO) */
#endif /* __MRS_CCO_IO_H__ */

/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS STA I/O.
 */

#ifndef __MRS_STA_IO_H__
#define __MRS_STA_IO_H__

#include <soc_types.h>
#include <soc_mdm_io.h>

#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)

#ifdef __cplusplus
extern "C" {
#endif

typedef td_u32 (*mrs_io_evtout_callback)(td_u8 param0, td_bool param1);
typedef td_u32 (*mrs_io_reset_callback)(td_void);

td_u32 mrs_sta_io_init(td_void);
td_void mrs_sta_io_evtout_timeout_handle(td_void);
td_void mrs_sta_io_register_evtout_callback(mrs_io_evtout_callback callback, td_u8 param);

/* LED language */
td_void mrs_sta_led_join_network(td_void);
td_void mrs_sta_led_offline(td_void);

td_void mrs_sta_plc_rx_led_off(td_void);
td_void mrs_sta_plc_tx_led_off(td_void);

#define mrs_io_init()        mrs_sta_io_init()
#define mrs_plc_rx_led_off() mrs_sta_plc_rx_led_off()
#define mrs_plc_tx_led_off() mrs_sta_plc_tx_led_off()
td_void uapi_set_power_failure_module_gpio_init(ext_gpio_idx_e id, td_u8 io_val, ext_gpio_direction_e dir);
td_u32 uapi_get_power_failure_is_plug_module(td_void);

#ifdef __cplusplus
}
#endif

#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_STA) */
#endif /* __MRS_STA_IO_H__ */


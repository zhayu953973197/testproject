/*
 * Copyright (c) CompanyNameMagicTag 2018-2020. All rights reserved.
 * Description: APP UART handle.
 */

#ifndef APP_UART_H
#define APP_UART_H

#include "soc_mdm_uart.h"

#ifdef __cplusplus
extern "C" {
#endif

#define APP_UART_SLEEP_DURTION_MS 1000

td_u32 app_uart_init(td_void);
td_u32 app_uart_tx(EXT_CONST td_u8 *data, td_u16 data_len);
/* Set the serial port number and serial port parameters before calling app_uart_init. */
td_u32 app_uart_set_param(ext_uart_port uart_port, ext_uart_init_type uart_cfg);
/* The serial port configuration takes effect. If the serial port is used independently,
 * it must be called after app_uart_set_param is called.
 */
td_u32 app_uart_init_cfg(td_void);

#ifdef __cplusplus
}
#endif
#endif

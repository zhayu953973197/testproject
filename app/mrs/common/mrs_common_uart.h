/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS uart entry.
 */

#ifndef __MRS_COMMON_UART_H__
#define __MRS_COMMON_UART_H__

#include <soc_types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef td_void (*mrs_uart_recv)(td_u8 ch);
typedef td_u16 (*mrs_uart_send)(td_pbyte buffer, td_u16 length);

typedef struct {
    mrs_uart_recv recv;
    mrs_uart_send send;
} mrs_uart_callback;

td_u32 mrs_uart_init(td_void);

td_void mrs_uart_task_body(uintptr_t param);

td_u32 mrs_uart_tx(td_pbyte data, td_u16 length);

td_void mrs_uart_register_rx_callback(mrs_uart_recv callback);
td_void mrs_uart_register_tx_callback(mrs_uart_send callback);
td_u16 mrs_uart_send_data(td_pbyte data, td_u16 length);

#ifdef __cplusplus
}
#endif
#endif /* __MRS_COMMON_UART_H__ */

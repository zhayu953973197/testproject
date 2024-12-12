/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS main entry.
 */

#ifndef __MRS_COMMON_MAIN_H__
#define __MRS_COMMON_MAIN_H__

#include "mrs_config.h"
#include "mrs_common_uart.h"

#ifdef __cplusplus
extern "C" {
#endif

#define APP_TL_QUEUE_SIZE 10

typedef struct {
    td_u32 queue_id;
    td_u32 sem_id[MRS_SEM_ID_MAX];
    ext_timer_handle_s timer_handle[MRS_TIMER_ID_MAX];

    mrs_uart_callback uart_callback;
} mrs_main_srv_context;

mrs_main_srv_context *mrs_get_main_srv_ctx(td_void);
td_void app_phase_init(td_void);

#ifdef __cplusplus
}
#endif

#endif /* __MRS_COMMON_MAIN_H__ */

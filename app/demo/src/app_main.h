/*
 * Copyright (c) CompanyNameMagicTag 2018-2020. All rights reserved.
 * Description: APP main entry.
 */

#ifndef APP_MAIN_H
#define APP_MAIN_H

#include "soc_mdm_timer.h"

#ifdef __cplusplus
extern "C" {
#endif

#define APP_UART_BUF_SIZE        512
#define APP_FRAME_DATA_MAX       466
#define APP_QUEUE_SIZE           16
#define APP_UART_TASK_STACK_SIZE 1024
#define APP_UART_TASK_PRIORITY   25
#define APP_SRV_TASK_STACK_SIZE  1024
#define APP_SRV_TASK_PRIORITY    25
#define APP_STARTUP_TIME         0xffffffff
#define APP_SLEEP_DURTION_MS     1000
#define APP_SEND_PLC_PRIO        3
#define APP_LED_STEP             16
#define APP_LED_STEP1            4
#define APP_LED_STEP2            8
#define APP_LED_STEP3            12
#define APP_TL_QUEUE_SIZE        10

typedef enum {
    APP_MSG_ID_TIMER = 1,
    APP_MSG_ID_DIAG,
    APP_MSG_ID_MAX
} app_msg_id;

typedef enum {
    APP_TIMER_ID_UART = 0,
    APP_TIMER_ID_INIT = 1,
    APP_TIMER_ID_WHITELIST = 1,
    APP_TIMER_ID_WRITE_WHITELIST_TO_FLASH,
    APP_TIMER_ID_MAX
} app_timer_id;

#define APP_TIMER_INIT 500
#define APP_TIME_UART  200

typedef enum {
    APP_SEM_ID_UART,
    APP_SEM_ID_MAX
} app_sem_id;

typedef struct {
    td_u16 rx_len;
    td_u16 data_th;
    td_u8 buffer[APP_UART_BUF_SIZE];
} app_data_buf;

typedef struct {
    td_u32 task_id;
    td_u32 queue_id;
    td_u32 sem_id[APP_SEM_ID_MAX];
    ext_timer_handle_s timer_handle[APP_TIMER_ID_MAX];
} app_res_cfg;

typedef struct {
    app_res_cfg res;
    app_data_buf buffer;
} app_srv_context;

td_u32 app_srv_init(td_void);
app_srv_context *app_get_srv_context(td_void);
td_void app_uart_rx(td_u8 ch);
#ifdef __cplusplus
}
#endif
#endif

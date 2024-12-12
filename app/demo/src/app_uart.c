/*
 * Copyright (c) CompanyNameMagicTag 2018-2020. All rights reserved.
 * Description: APP UART handle.
 */

#include "app_main.h"
#include "stdio.h"
#include "soc_mdm_task.h"
#include "app_uart.h"

EXT_PRV ext_uart_port g_app_uart_port = EXT_UART_0;
EXT_PRV ext_uart_init_type g_app_uart_cfg = { 115200, 8, 1, 2 };

static td_void app_uart_task_body(td_u32 param);

td_u32 app_uart_init_cfg(td_void)
{
    td_u32 ret;

    printf("app_uart_init().\n");

    uapi_uart_close(g_app_uart_port);

    ret = uapi_uart_open(g_app_uart_port);
    ret |= uapi_uart_ioctl(g_app_uart_port, UART_CFG_SET_ATTR, (uintptr_t)&g_app_uart_cfg);
    if (ret != EXT_ERR_SUCCESS) {
        printf("open uart%d failed.\n", g_app_uart_port);
        return EXT_ERR_FAILURE;
    }

    return EXT_ERR_SUCCESS;
}

/* initialize application layer business port */
td_u32 app_uart_init(td_void)
{
    td_u32 ret;
    td_u32 uart_task_id = (td_u32)-1;

    ret = app_uart_init_cfg();
    if (ret != EXT_ERR_SUCCESS) {
        return ret;
    }

    ret = uapi_task_create(&uart_task_id, "COM_APP", app_uart_task_body, 0,
                           APP_UART_TASK_STACK_SIZE, APP_UART_TASK_PRIORITY);
    if (ret != EXT_ERR_SUCCESS) {
        printf("create uart%d task failed.\n", g_app_uart_port);
        return EXT_ERR_FAILURE;
    }

    printf("uart%d init success.\n", g_app_uart_port);

    return EXT_ERR_SUCCESS;
}

static td_void app_uart_task_body(td_u32 param)
{
    td_u8 ch = 0;
    td_s32 len;

    ext_unref_param(param);

    printf("ENTER app_uart_task_body().\n");

    for (;;) {
        len = uapi_uart_read(g_app_uart_port, &ch, 1);
        if (len == 1) {
            app_uart_rx(ch);
            continue;
        }

        uapi_sleep(APP_UART_SLEEP_DURTION_MS);
    }
}

td_u32 app_uart_tx(EXT_CONST td_u8 *data, td_u16 data_len)
{
    if (uapi_uart_write(g_app_uart_port, data, data_len) != (td_s32)data_len) {
        return EXT_ERR_FAILURE;
    }

    return EXT_ERR_SUCCESS;
}

td_u32 app_uart_set_param(ext_uart_port uart_port, ext_uart_init_type uart_cfg)
{
    printf("set app uartport: %d\n", uart_port);
    g_app_uart_port = uart_port;
    g_app_uart_cfg = uart_cfg;

    return EXT_ERR_SUCCESS;
}


/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS uart entry.
 */

#include "mrs_common_uart.h"
#include "mrs_config.h"
#include "mrs_common_tools.h"
#include "mrs_common_main.h"
#include "mrs_dfx.h"
#include "soc_mac_nv.h"

ext_uart_port g_mrs_uart_port = MRS_SRV_UART_PORT;

static td_void mrs_uart_rx_handle(td_u8 ch);

td_u32 mrs_uart_init(td_void)
{
    td_u32 ret;
    ext_uart_init_type init = {
        MRS_SRV_UART_BAUDRATE, MRS_SRV_UART_DATABITS,
        MRS_SRV_UART_STOPBITS, MRS_SRV_UART_PARITY
    };

#ifdef PRODUCT_CFG_PRODUCT_TYPE_NDM
    diag_cmd_config_ndm_connect_para_stru ndm_param = { 0 };
    ret = uapi_nv_read(ID_DIAG_CMD_CONFIG_NDM_CONNECT_PARA,
        &ndm_param, sizeof(diag_cmd_config_ndm_connect_para_stru));
    if (ret == EXT_ERR_SUCCESS) {
        switch (ndm_param.work_mode) {
            case EXT_NDM_CIU_MODE_SWITCH_COM:
            case EXT_NDM_SEARCH_NET_MODE_SWITCH_COM:
                g_mrs_uart_port = EXT_UART_1; /* business port. CNComents:业务串口 */
                break;
            case EXT_NDM_UNKNOWN_WORK_MODE:
            case EXT_NDM_CIU_MODE:
            case EXT_NDM_SEARCH_NET_MODE: /* 2 is search mode */
                g_mrs_uart_port = EXT_UART_0; /* debug port. CNComents:调试串口 */
                break;
            default:
                break;
        }
    }
#endif
    (td_void) uapi_uart_close(g_mrs_uart_port);
    ret = uapi_uart_open(g_mrs_uart_port);
    if (ret != EXT_ERR_SUCCESS) {
        return ret;
    }

    ret = uapi_uart_ioctl(g_mrs_uart_port, UART_CFG_SET_ATTR, (uintptr_t)&init);
    if (ret != EXT_ERR_SUCCESS) {
        return ret;
    }

    mrs_uart_register_tx_callback(mrs_uart_send_data);

    return ret;
}

td_void mrs_uart_task_body(uintptr_t param)
{
    td_s32 len;
    td_u8 ch = 0;

    ext_unref_param(param);

    for (;;) {
        len = uapi_uart_read(g_mrs_uart_port, &ch, 1);
        if (len == 1) {
            mrs_uart_rx_handle(ch);
            continue;
        }

        uapi_sleep(1000); /* sleep time 1000ms */
    }
}

td_void mrs_uart_register_rx_callback(mrs_uart_recv callback)
{
    mrs_main_srv_context *srv = mrs_get_main_srv_ctx();

    mrs_printf("mrs_uart_register_rx_fct: %x\n", (uintptr_t)callback);
    if (srv != TD_NULL) {
        srv->uart_callback.recv = callback;
    }
}

td_void mrs_uart_register_tx_callback(mrs_uart_send callback)
{
    mrs_main_srv_context *srv = mrs_get_main_srv_ctx();
    if (srv != TD_NULL) {
        srv->uart_callback.send = callback;
    }
}

static td_void mrs_uart_rx_handle(td_u8 ch)
{
    mrs_main_srv_context *srv = mrs_get_main_srv_ctx();

    if (srv == TD_NULL) {
        return;
    }

    mrs_dfx_uart_chl_rx(&ch, 1, TD_FALSE);

    if (srv->uart_callback.recv != TD_NULL) {
        srv->uart_callback.recv(ch);
    }
}

td_u16 mrs_uart_send_data(td_pbyte data, td_u16 length)
{
    return (td_u16)uapi_uart_write(g_mrs_uart_port, data, length);
}

td_u32 mrs_uart_tx(td_pbyte data, td_u16 length)
{
    mrs_main_srv_context *srv = mrs_get_main_srv_ctx();

    mrs_logbuf("[  Tx]: ", data, length);
    mrs_dfx_uart_chl_tx(data, length);

    if (srv == TD_NULL) {
        mrs_dfx_uart_chl_tx_err_cnt(length, EXT_ERR_INVALID_PARAMETER);
        return EXT_ERR_NO_INITILIZATION;
    }

    if (srv->uart_callback.send == TD_NULL) {
        mrs_dfx_uart_chl_tx_err_cnt(length, EXT_ERR_NOT_SUPPORT);
        return EXT_ERR_NO_INITILIZATION;
    }

    if (length != srv->uart_callback.send(data, length)) {
        mrs_dfx_uart_chl_tx_err_cnt(length, EXT_ERR_FAILURE);
        return EXT_ERR_FAILURE;
    }

    mrs_dfx_uart_chl_tx_ok_cnt(length);
    return EXT_ERR_SUCCESS;
}

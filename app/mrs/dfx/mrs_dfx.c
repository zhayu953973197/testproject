/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS DFX.
 */

#include "mrs_dfx.h"
#include <dfx_app.h>
#include "mrs_cco_tf.h"

typedef struct {
    uart_chl_inf uart_chl[2]; /* 2groups */
    plc_chl_inf plc_chl[2];   /* 2groups */
} mrs_dfx_ctx;

td_u32 mrs_stat_uart_rx(td_void);
td_u32 mrs_stat_plc_rx(td_void);

static mrs_dfx_ctx g_mrs_dfx;

td_void mrs_dfx_init(td_void)
{
    memset_s(&g_mrs_dfx, sizeof(g_mrs_dfx), 0, sizeof(g_mrs_dfx));

#if (defined(PRODUCT_CFG_PRODUCT_TYPE_CCO) || defined(PRODUCT_CFG_PRODUCT_TYPE_STA))
    uapi_sys_register_reset_notify(mrs_stat_uart_rx, EXT_RST_CON_USR0);
    uapi_sys_register_reset_notify(mrs_stat_plc_rx, EXT_RST_CON_USR1);
#endif
}

td_u32 mrs_stat_uart_rx(td_void)
{
    return g_mrs_dfx.uart_chl[1].rx_ok_num;
}

td_u32 mrs_stat_plc_rx(td_void)
{
    return g_mrs_dfx.plc_chl[1].rx_total_num;
}

td_void mrs_dfx_uart_chl_rx(const td_u8 * const data, td_u16 data_len, td_bool msg_proc)
{
    uart_chl_inf *inf = (msg_proc == TD_FALSE) ? &g_mrs_dfx.uart_chl[0] : &g_mrs_dfx.uart_chl[1];
    td_u16 idx;
    td_u16 tail;
    td_u16 max_len;

    if ((data == TD_NULL) || (data_len == 0)) {
        return;
    }

    inf->rx_total_len += data_len;
    inf->rx_last_time = uapi_get_milli_seconds();

    tail = inf->rx_buf_tail;
    max_len = (td_u16)sizeof(inf->rx_buf);
    tail %= max_len;

    for (idx = 0; idx < data_len; idx++) {
        inf->rx_buf[tail++] = data[idx];
        tail %= max_len;
    }

    inf->rx_buf_tail = tail;
}

td_void mrs_dfx_uart_chl_err_rx(td_u16 data_len, td_u32 err_cause, td_bool msg_proc)
{
    uart_chl_inf *inf = (msg_proc == TD_FALSE) ? &g_mrs_dfx.uart_chl[0] : &g_mrs_dfx.uart_chl[1];

    inf->rx_err_len += data_len;
    inf->rx_err_cause = (td_u16)err_cause;
}

td_void mrs_dfx_uart_chl_rx_frame(td_u16 data_len, td_u32 err_cause, td_bool msg_proc)
{
    uart_chl_inf *inf = (msg_proc == TD_FALSE) ? &g_mrs_dfx.uart_chl[0] : &g_mrs_dfx.uart_chl[1];

    if (err_cause == EXT_ERR_SUCCESS) {
        inf->rx_ok_len += data_len;
        inf->rx_ok_num++;
    } else {
        inf->rx_err_len += data_len;
        inf->rx_err_num++;
        inf->rx_err_cause = (td_u16)err_cause;
    }
}

td_void mrs_dfx_uart_chl_tx(const td_u8 *data, td_u16 data_len)
{
    td_u8 i;

    for (i = 0; i < 2; i++) { /* 2groups */
        uart_chl_inf *inf = &g_mrs_dfx.uart_chl[i];

        inf->tx_total_len += data_len;
        inf->tx_last_time = uapi_get_milli_seconds();
        inf->tx_total_num++;
        inf->tx_last_len = data_len;
        memset_s(inf->tx_buf, sizeof(inf->tx_buf), 0, sizeof(inf->tx_buf));
        if (memcpy_s(inf->tx_buf, sizeof(inf->tx_buf), data, uapi_min(sizeof(inf->tx_buf), data_len)) != EOK) {
            return;
        }
    }
}

td_void mrs_dfx_uart_chl_tx_ok_cnt(td_u16 data_len)
{
    td_u8 i;

    for (i = 0; i < 2; i++) { /* 2groups */
        uart_chl_inf *inf = &g_mrs_dfx.uart_chl[i];

        inf->tx_ok_len += data_len;
        inf->tx_ok_num++;
    }
}

td_void mrs_dfx_uart_chl_tx_err_cnt(td_u16 data_len, td_u32 err_cause)
{
    td_u8 i;

    for (i = 0; i < 2; i++) { /* 2groups */
        uart_chl_inf *inf = &g_mrs_dfx.uart_chl[i];

        inf->tx_err_len += data_len;
        inf->tx_err_num++;
        inf->tx_err_cause = (td_u16)err_cause;
    }
}

td_void mrs_dfx_plc_chl_rx(const ext_dms_chl_rx *rx)
{
    plc_chl_inf *inf = &g_mrs_dfx.plc_chl[0];

    if (rx == TD_NULL) {
        return;
    }

    inf->rx_total_num++;
    inf->rx_time = uapi_get_milli_seconds();
    inf->rx_id = rx->id;
    memcpy_s(inf->src_mac, sizeof(inf->src_mac), rx->src_mac, sizeof(rx->src_mac));
    inf->rx_len = rx->payload_len;

    memset_s(inf->rx_buf, sizeof(inf->rx_buf), 0, sizeof(inf->rx_buf));
    if (memcpy_s(inf->rx_buf, sizeof(inf->rx_buf),
            rx->payload, uapi_min(sizeof(inf->rx_buf), rx->payload_len)) != EOK) {
        return;
    }
}

td_void mrs_dfx_plc_chl_err_rx(const ext_dms_chl_rx *rx, td_u32 err_cause)
{
    plc_chl_inf *inf = &g_mrs_dfx.plc_chl[0];

    if (rx == TD_NULL) {
        return;
    }

    inf->rx_err_num++;
    inf->rx_err_time = uapi_get_milli_seconds();
    inf->rx_err_id = rx->id;
    memcpy_s(inf->err_src_mac, sizeof(inf->err_src_mac), rx->src_mac, sizeof(rx->src_mac));
    inf->rx_err_cause = (td_u16)err_cause;
    inf->rx_err_len = rx->payload_len;

    memset_s(inf->rx_err_buf, sizeof(inf->rx_err_buf), 0, sizeof(inf->rx_buf));
    if (memcpy_s(inf->rx_err_buf, sizeof(inf->rx_err_buf),
                 rx->payload, uapi_min(sizeof(inf->rx_buf), rx->payload_len)) != EOK) {
        return;
    }
}

td_void mrs_dfx_plc_rx_proc(const mrs_plc_frame_data *rx)
{
    plc_chl_inf *inf = &g_mrs_dfx.plc_chl[1];

    if (rx == TD_NULL) {
        return;
    }

    inf->rx_total_num++;
    inf->rx_time = uapi_get_milli_seconds();
    inf->rx_id = rx->id;
    memcpy_s(inf->src_mac, sizeof(inf->src_mac), rx->addr, sizeof(rx->addr));
    inf->rx_len = rx->payload_len;

    memset_s(inf->rx_buf, sizeof(inf->rx_buf), 0, sizeof(inf->rx_buf));
    if (memcpy_s(inf->rx_buf, sizeof(inf->rx_buf),
            rx->payload, uapi_min(sizeof(inf->rx_buf), rx->payload_len)) != EOK) {
        return;
    }
}

td_void mrs_dfx_plc_rx_err_proc(const mrs_plc_frame_data *rx, td_u32 err_cause)
{
    plc_chl_inf *inf = &g_mrs_dfx.plc_chl[1];

    if (rx == TD_NULL) {
        return;
    }

    inf->rx_err_num++;
    inf->rx_err_time = uapi_get_milli_seconds();
    inf->rx_err_id = rx->id;
    memcpy_s(inf->err_src_mac, sizeof(inf->err_src_mac), rx->addr, sizeof(rx->addr));
    inf->rx_err_cause = (td_u16)err_cause;
    inf->rx_err_len = rx->payload_len;

    memset_s(inf->rx_err_buf, sizeof(inf->rx_err_buf), 0, sizeof(inf->rx_buf));
    if (memcpy_s(inf->rx_err_buf, sizeof(inf->rx_err_buf),
                 rx->payload, uapi_min(sizeof(inf->rx_err_buf), rx->payload_len)) != EOK) {
        return;
    }
}

td_void mrs_dfx_plc_chl_tx(const mrs_plc_frame_data *tx, td_bool msg_proc)
{
    plc_chl_inf *inf = (msg_proc == TD_FALSE) ? &g_mrs_dfx.plc_chl[0] : &g_mrs_dfx.plc_chl[1];

    if (tx == TD_NULL) {
        return;
    }

    inf->tx_total_num++;
    inf->tx_time = uapi_get_milli_seconds();
    inf->tx_id = tx->id;
    memcpy_s(inf->dst_mac, sizeof(inf->dst_mac), tx->addr, sizeof(tx->addr));

    inf->tx_len = tx->payload_len;
    memset_s(inf->tx_buf, sizeof(inf->tx_buf), 0, sizeof(inf->tx_buf));
    if (memcpy_s(inf->tx_buf, sizeof(inf->tx_buf),
            tx->payload, uapi_min(sizeof(inf->tx_buf), tx->payload_len)) != EOK) {
        return;
    }
}

td_void mrs_dfx_plc_chl_err_tx(const mrs_plc_frame_data *tx, td_bool msg_proc, td_u32 err_cause)
{
    plc_chl_inf *inf = (msg_proc == TD_FALSE) ? &g_mrs_dfx.plc_chl[0] : &g_mrs_dfx.plc_chl[1];

    if (tx == TD_NULL) {
        return;
    }

    inf->tx_fail_num++;
    inf->tx_fail_time = uapi_get_milli_seconds();
    inf->tx_id = tx->id;
    inf->tx_err_cause = (td_u16)err_cause;
    memcpy_s(inf->fail_dst_mac, sizeof(inf->fail_dst_mac), tx->addr, sizeof(tx->addr));

    inf->tx_fail_len = tx->payload_len;
    memset_s(inf->tx_fail_buf, sizeof(inf->tx_fail_buf), 0, sizeof(inf->tx_buf));
    if (memcpy_s(inf->tx_fail_buf, sizeof(inf->tx_fail_buf),
                 tx->payload, uapi_min(sizeof(inf->tx_fail_buf), tx->payload_len)) != EOK) {
        return;
    }
}

td_u32 mrs_diag_dfx(td_u16 id, td_pvoid cmd, td_u16 cmd_size, td_u8 option)
{
    td_pbyte payload = TD_NULL;
    td_u16 payload_len = 0;

    ext_unref_param(cmd);
    ext_unref_param(cmd_size);

    switch (id) {
        case ID_DIAG_CMD_UART_MSG_SEND_INF:
            payload = (td_pbyte)&g_mrs_dfx.uart_chl[0];
            payload_len = sizeof(g_mrs_dfx.uart_chl[0]);
            break;

        case ID_DIAG_CMD_UART_MSG_PROC_INF:
            payload = (td_pbyte)&g_mrs_dfx.uart_chl[1];
            payload_len = sizeof(g_mrs_dfx.uart_chl[1]);
            break;

        case ID_DIAG_CMD_PLC_MSG_SEND_INF:
            payload = (td_pbyte)&g_mrs_dfx.plc_chl[0];
            payload_len = sizeof(g_mrs_dfx.plc_chl[0]);
            break;

        case ID_DIAG_CMD_PLC_MSG_PROC_INF:
            payload = (td_pbyte)&g_mrs_dfx.plc_chl[1];
            payload_len = sizeof(g_mrs_dfx.plc_chl[1]);
            break;

        default:
            return EXT_ERR_INVALID_PARAMETER;
    }

    return uapi_diag_report_packet(id, option, payload, payload_len, TD_FALSE);
}

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
td_u32 mrs_dfx_tf(td_u16 id, td_pvoid cmd, td_u16 cmd_size, td_u8 option)
{
    td_u32 ret = EXT_ERR_NOT_SUPPORT;

    switch (id) {
        case ID_DIAG_CMD_TF_CTRL:
            ret = mrs_dfx_tf_ctrl((diag_cmd_tf_ctrl_req *)cmd, cmd_size, option);
            break;
        case ID_DIAG_CMD_TF_RESULT_QUERY:
            ret = mrs_dfx_tf_result_query((diag_cmd_tf_result_req *)cmd, cmd_size, option);
            break;
        default:
            break;
    }

    return ret;
}
#endif


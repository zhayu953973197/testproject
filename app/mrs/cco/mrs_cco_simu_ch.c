/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS CCO simulation channel.
 */

#include "mrs_cco_simu_ch.h"
#include "mrs_proto.h"
#include "mrs_cco_srv.h"
#include "mrs_common_tools.h"
#include "mrs_common_uart.h"
#include "dfx_app.h"
#include "mrs_dfx.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
typedef struct {
    td_u8 rsv0;
    td_u8 simu_ch_enable; /* 0: simulator channel closed; 1: simulator channel opened; */
    td_u8 rsv1[2];        /* reserved 2B */
} mrs_simu_ch_ctrl;

typedef struct {
    td_u32 data_size;
    td_u8 data[0];
} mrs_simu_ch_data;

td_u16 mrs_cco_simu_ch_tx(td_pbyte buffer, td_u16 length);

td_u32 mrs_cco_simu_ch_ctrl(td_u16 id, td_pvoid cmd, td_u16 cmd_size, td_u8 option)
{
    mrs_simu_ch_ctrl *ctrl = TD_NULL;
    td_bool new_status;
    td_bool old_status;

    ext_unref_param(id);
    ext_unref_param(cmd_size);
    ext_unref_param(option);

    if (cmd == TD_NULL) {
        return EXT_ERR_INVALID_PARAMETER;
    }

    ctrl = (mrs_simu_ch_ctrl *)cmd;
    new_status = (ctrl->simu_ch_enable == TD_FALSE) ? TD_FALSE : TD_TRUE;
    old_status = mrs_cco_simu_status();
    if (old_status == new_status) {
        return EXT_ERR_SUCCESS;
    }

    mrs_cco_simu_ctrl(new_status);
    mrs_proto_simu_ctrl(new_status);

    if (new_status == TD_TRUE) {
        /* disable --> enable */
        mrs_uart_register_rx_callback(TD_NULL);
        mrs_uart_register_tx_callback(mrs_cco_simu_ch_tx);
    } else {
        /* enable --> disable */
        mrs_uart_register_rx_callback(mrs_proto_rx);
        mrs_uart_register_tx_callback(mrs_uart_send_data);
    }

    return EXT_ERR_SUCCESS;
}
td_u32 mrs_cco_simu_ch_rx(td_u16 id, td_pvoid cmd, td_u16 cmd_size, td_u8 option)
{
    mrs_simu_ch_data *req = (mrs_simu_ch_data *)cmd;
    td_u32 i;

    ext_unref_param(id);
    ext_unref_param(cmd_size);
    ext_unref_param(option);

    if (mrs_cco_simu_status() == TD_FALSE) {
        return EXT_ERR_FAILURE;
    }

    for (i = 0; i < req->data_size; i++) {
        mrs_proto_rx(req->data[i]);
    }

    mrs_dfx_uart_chl_rx(req->data, (td_u16)req->data_size, TD_FALSE);

    return EXT_ERR_SUCCESS;
}

td_u16 mrs_cco_simu_ch_tx(td_pbyte buffer, td_u16 length)
{
    mrs_simu_ch_data *ind = TD_NULL;
    td_u32 ret;

    if ((buffer == TD_NULL) || (length == 0)) {
        return 0;
    }

    ind = (mrs_simu_ch_data *)mrs_malloc(sizeof(mrs_simu_ch_data) + length);
    if (ind == TD_NULL) {
        return 0;
    }

    ind->data_size = (td_u32)length;
    if (ind->data_size > 0) {
        if (memcpy_s(ind->data, ind->data_size, buffer, length) != EOK) {
            mrs_free(ind);
            return 0;
        }
    }

    ret = uapi_diag_report_packet(ID_DIAG_CMD_CSIM_PACKET, EXT_DIAG_CMD_INSTANCE_LOCAL, (td_pbyte)ind,
        (td_u16)(sizeof(mrs_simu_ch_data) + length), TD_FALSE);
    mrs_free(ind);

    if (ret == EXT_ERR_SUCCESS) {
        return length;
    }

    return 0;
}

#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_CCO) */


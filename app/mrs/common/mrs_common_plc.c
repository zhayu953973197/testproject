/*
 * Copyright (c) CompanyNameMagicTag 2012-2020. All rights reserved.
 * Description: MRS plc handle.
 */

#include "mrs_common_plc.h"
#include "mrs_common_tools.h"

#include "mrs_cco_plc.h"

#include "mrs_sta_plc.h"

#include "mrs_ndm_plc.h"

#include "mrs_dfx.h"

td_u32 mrs_plc_chl_status(td_u16 port, td_u8 status);
td_u32 mrs_plc_chl_rx(td_u16 port, EXT_CONST ext_dms_chl_rx *chl_rx);

td_u32 mrs_plc_chl_init(td_void)
{
    ext_dms_chl_port_obj plc_chl_obj = { mrs_plc_chl_status, mrs_plc_chl_rx, TD_NULL };

    return uapi_dms_chl_setup(&plc_chl_obj, EXT_DMS_CHL_MAC_PORT_APP);
}

td_u32 mrs_plc_chl_status(td_u16 port, td_u8 status)
{
    return mrs_msg_queue_send(MRS_MSG_ID_PLC_CHL, port, status, 0);
}

td_u32 mrs_plc_chl_rx(td_u16 port, EXT_CONST ext_dms_chl_rx *chl_rx)
{
    td_u16 payload_len = sizeof(mrs_plc_frame_data) + chl_rx->payload_len;
    td_pbyte payload = (td_pbyte)mrs_malloc(payload_len);
    mrs_plc_frame_data *plc_frame = TD_NULL;
    td_u32 ret;

    ext_unref_param(port);

    mrs_dfx_plc_chl_rx((ext_dms_chl_rx *)chl_rx);

    if (payload == TD_NULL) {
        return EXT_ERR_MALLOC_FAILUE;
    }

    (td_void) memset_s(payload, payload_len, 0, payload_len);
    plc_frame = (mrs_plc_frame_data *)payload;
    plc_frame->id = chl_rx->id;
    (td_void) memcpy_s(plc_frame->addr, sizeof(plc_frame->addr), chl_rx->src_mac, sizeof(chl_rx->src_mac));
    plc_frame->payload_len = chl_rx->payload_len;
    plc_frame->payload = payload + sizeof(mrs_plc_frame_data);
    plc_frame->form_serial_number = chl_rx->form_serial_number;

    if (plc_frame->payload_len > 0) {
        if (memcpy_s(plc_frame->payload, plc_frame->payload_len, chl_rx->payload, chl_rx->payload_len) != EOK) {
            mrs_free(payload);
            return EXT_ERR_FAILURE;
        }
    }

    if (is_plc_bc_mac(chl_rx->dest_mac)) {
        plc_frame->bc_flag = TD_TRUE;
    }

    mrs_printf("[PLC-RX]: %02X:%02X:%02X:%02X:%02X:%02X (%04X) ",
               chl_rx->src_mac[0], chl_rx->src_mac[1], /* 0&1bytes */
               chl_rx->src_mac[2], chl_rx->src_mac[3], /* 2&3bytes */
               chl_rx->src_mac[4], chl_rx->src_mac[5], /* 4&5bytes */
               chl_rx->id);
    mrs_logbuf("  ", chl_rx->payload, chl_rx->payload_len);

    ret = mrs_msg_queue_send(MRS_MSG_ID_PLC_RX, plc_frame->id, (uintptr_t)payload, payload_len);
    if (ret != EXT_ERR_SUCCESS) {
        mrs_dfx_plc_chl_err_rx(chl_rx, ret);
        mrs_free(payload);
    }

    return ret;
}

td_void mrs_msg_on_plc_rx(EXT_CONST mrs_queue_msg *msg)
{
    mrs_plc_frame_data *plc = (mrs_plc_frame_data *)msg->param1;
    td_u16 length = (td_u16)msg->param2;
    td_u16 id = msg->param0;

    mrs_dfx_plc_rx_proc(plc);

    if (length != plc->payload_len + sizeof(mrs_plc_frame_data)) {
        mrs_free(plc);
        mrs_dfx_plc_rx_err_proc(plc, EXT_ERR_BAD_DATA);
        return;
    }

    switch (id) {
        case PLC_CMD_ID_XR:
            mrs_xr_plc_frame_rx(plc);
            break;

        case PLC_CMD_ID_PR:
            mrs_pr_plc_frame_rx(plc);
            break;

        case PLC_CMD_ID_EVT:
            mrs_evt_plc_frame_rx(plc);
            break;

        case PLC_CMD_ID_BC:
            mrs_bc_plc_frame_rx(plc);
            break;

        case PLC_CMD_ID_METER_LIST:
        case PLC_CMD_ID_START_SEARCH:
        case PLC_CMD_ID_STOP_SEARCH:
            mrs_tf_plc_frame_rx(plc);
            break;

        case PLC_CMD_ID_CIU_TRANSMIT:
            mrs_ciu_plc_frame_rx(plc);
            break;

        case PLC_CMD_ID_QRY_STA_INF:
            mrs_qry_plc_frame_rx(plc);
            break;

        case PLC_CMD_ID_DUT_CCO_TEST:
            mrs_cco_dut_plc_frame_rx(plc);
            break;

        case PLC_CMD_ID_DUT_STA_TEST:
            mrs_sta_dut_plc_frame_rx(plc);
            break;

        default:
            mrs_dfx_plc_rx_err_proc(plc, EXT_ERR_NOT_FOUND);
            break;
    }

    mrs_free(plc);
}

td_u32 mrs_plc_frame_send(const mrs_plc_frame_data *frame)
{
    td_u32 ret;
    td_u32 err;
    mrs_plc_frame_data *new_frame = TD_NULL;
    td_u16 payload_len = sizeof(mrs_plc_frame_data) + frame->payload_len;
    td_pbyte payload = mrs_malloc(payload_len);
    if (payload == TD_NULL) {
        return EXT_ERR_MALLOC_FAILUE;
    }

    err = (td_u32)memcpy_s(payload, payload_len, frame, sizeof(mrs_plc_frame_data));
    err |= (td_u32)memcpy_s(payload + sizeof(mrs_plc_frame_data), payload_len - sizeof(mrs_plc_frame_data),
                            frame->payload, frame->payload_len);
    if (err != EOK) {
        mrs_free(payload);
        return EXT_ERR_FAILURE;
    }

    new_frame = (mrs_plc_frame_data *)payload;
    new_frame->payload = payload + sizeof(mrs_plc_frame_data);

    mrs_dfx_plc_chl_tx(new_frame, TD_FALSE);

    ret = mrs_msg_queue_send(MRS_MSG_ID_PLC_TX, 0, (uintptr_t)new_frame, 0);
    if (ret != EXT_ERR_SUCCESS) {
        mrs_dfx_plc_chl_err_tx(new_frame, (td_bool)ret, TD_FALSE);
        mrs_free(payload);
    }

    return ret;
}

td_void mrs_msg_on_plc_tx(EXT_CONST mrs_queue_msg *msg)
{
    mrs_plc_frame_data *frame = (mrs_plc_frame_data *)msg->param1;
    ext_dms_chl_tx_data *tx_data = TD_NULL;
    ext_dms_chl_tx_ctrl *tx_ctrl = TD_NULL;
    td_pbyte payload;
    td_u16 length;

    mrs_printf("[PLC-TX]: %02X:%02X:%02X:%02X:%02X:%02X (%04X) ",
               frame->addr[0], frame->addr[1], frame->addr[2], frame->addr[3], /* 0&1bytes, 2&3bytes */
               frame->addr[4], frame->addr[5], frame->id);                     /* 4&5bytes */

    mrs_logbuf("  ", frame->payload, frame->payload_len);

    mrs_dfx_plc_chl_tx(frame, TD_TRUE);

    tx_ctrl = (ext_dms_chl_tx_ctrl *)mrs_malloc(sizeof(ext_dms_chl_tx_ctrl));
    if (tx_ctrl == TD_NULL) {
        mrs_dfx_plc_chl_err_tx(frame, EXT_ERR_MALLOC_FAILUE, TD_TRUE);
        mrs_free(frame);
        return;
    }

    length = sizeof(ext_dms_chl_tx_data) + frame->payload_len;
    payload = (td_pbyte)mrs_malloc(length);
    if (payload == TD_NULL) {
        mrs_dfx_plc_chl_err_tx(frame, EXT_ERR_MALLOC_FAILUE, TD_TRUE);
        mrs_free(tx_ctrl);
        mrs_free(frame);
        return;
    }

    (td_void) memset_s(payload, length, 0, length);
    (td_void) memset_s(tx_ctrl, sizeof(ext_dms_chl_tx_ctrl), 0, sizeof(ext_dms_chl_tx_ctrl));

    do {
        tx_data = (ext_dms_chl_tx_data *)payload;
        tx_data->data_size = frame->payload_len;
        if (tx_data->data_size > 0) {
            if (memcpy_s(tx_data->data, tx_data->data_size, frame->payload, frame->payload_len) != EOK) {
                break;
            }
        }

        tx_ctrl->id = frame->id;
        tx_ctrl->ctrl = EXT_DMS_CHL_TX_CTRL_NA;
        tx_ctrl->type = EXT_DMS_CHL_TX_TYPE_MR;
        tx_ctrl->prio = 3; /* priority 3 */
        if (memcpy_s(tx_ctrl->dest_mac, sizeof(tx_ctrl->dest_mac), frame->addr, sizeof(frame->addr)) != EOK) {
            break;
        }

        uapi_dms_chl_tx(EXT_DMS_CHL_MAC_PORT_APP, tx_data, tx_ctrl);
    } while (TD_FALSE);
    mrs_free(tx_data);
    mrs_free(tx_ctrl);
    mrs_free(frame);
}

td_u32 mrs_convert_meter_to_mac(const td_pbyte meter, const td_u8 meter_len, td_pbyte mac, td_u8 mac_len)
{
    /* need invert address */
    if (memcpy_s(mac, mac_len, meter, meter_len) != EXT_ERR_SUCCESS) {
        return EXT_ERR_FAILURE;
    }
    mrs_hex_invert(mac, mac_len);
    return EXT_ERR_SUCCESS;
}


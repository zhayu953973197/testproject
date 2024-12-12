/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS CCO plc handle.
 */

#include "mrs_cco_plc.h"
#include "mrs_common_tools.h"
#include "mrs_cco_mr_manage.h"
#include "mrs_cco_srv.h"
#include "mrs_cco_upg.h"
#include "mrs_proto_645.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
typedef td_u32 (*mrs_dut_handler)(mrs_dut_cco_frame *dut_frm, mrs_plc_frame_data *reply_plc);

/* processing for recieving PLC read meter frame */
td_void mrs_cco_mr_plc_frame_rx(td_u8 *mac, td_u16 seq, td_u8 type, td_u8 protocol, td_pbyte data, td_u16 length);
/* processing for equip test mode PLC frame */
td_u32 mrs_cco_dut_set_power_gain(mrs_dut_cco_frame *dut_frm, mrs_plc_frame_data *reply_plc);
td_u32 mrs_cco_dut_get_verion(mrs_dut_cco_frame *dut_frm, mrs_plc_frame_data *reply_plc);
mrs_dut_cco_frame *mrs_cco_dut_create_reply_frame(const mrs_dut_cco_frame *dut_frm, const td_u8 *data, td_u16 len);

td_u16 g_dut_cco_seq = 0xFFFF;

td_void mrs_cco_mr_plc_frame_rx(td_u8 *mac, td_u16 seq, td_u8 type, td_u8 protocol, td_pbyte data, td_u16 length)
{
    td_pbyte out_buf = TD_NULL;
    td_u16 out_len = 0;
    mr_task_node *node = TD_NULL;
    mrs_mr_item *item = mrs_cco_mr_find_pool_item(mac, seq, type);

    if (item == TD_NULL) {
        return;
    }

    node = item->node;
    /* Update protocol */
    node->protocol = protocol;
    node->create_uart_frame(&out_buf, &out_len, node, data, length);
    if (out_buf != TD_NULL) {
        node->send_uart_frame(out_buf, out_len);
    }

    mrs_free(out_buf);
    mrs_cco_mr_remove_pool_item(node);
    mrs_cco_mr_list_remove(node);
}

td_void mrs_cco_xr_plc_frame_rx(mrs_plc_frame_data *plc)
{
    mrs_data_transmit_xr *xr = (mrs_data_transmit_xr *)plc->payload;

    /* filter invalid frame */
    if (xr->dir != MRS_PLC_UP_FLG) {
        return;
    }

    if ((xr->data_len == 0) || (xr->stru_size < sizeof(mrs_data_transmit_xr)) ||
        (xr->stru_size + xr->data_len != plc->payload_len) || (xr->data_len > PLC_DATA_LEN_MAX)) {
        return;
    }

    mrs_cco_mr_plc_frame_rx(plc->addr, xr->seq, MRS_CCO_MR_TYPE_XR, xr->protocol,
                            plc->payload + xr->stru_size, xr->data_len);
}

td_void mrs_cco_pr_plc_frame_rx(mrs_plc_frame_data *plc)
{
    mrs_data_transmit_pr_up *pr = (mrs_data_transmit_pr_up *)plc->payload;

    /* filter invalid frame */
    if ((pr->data_len == 0) || (pr->stru_size < sizeof(mrs_data_transmit_pr_up)) ||
        (pr->stru_size + pr->data_len != plc->payload_len) || (pr->data_len > PLC_DATA_LEN_MAX)) {
        return;
    }

    mrs_cco_mr_plc_frame_rx(plc->addr, pr->seq, MRS_CCO_MR_TYPE_PR, pr->protocol,
                            plc->payload + pr->stru_size, pr->data_len);
}

/* PLC channel available */
td_void mrs_msg_on_plc_chl(EXT_CONST mrs_queue_msg *msg)
{
    mrs_cco_srv_ctx *cco = mrs_cco_get_srv_ctx();
    td_u8 status = (td_u8)msg->param1;

    if (status == 0) {
        cco->chl_status = TD_FALSE;
    } else {
        cco->chl_status = TD_TRUE;
    }
}

/* query station info */
td_void mrs_cco_qry_plc_frame_rx(mrs_plc_frame_data *plc)
{
    mrs_query_sta_inf *inf = (mrs_query_sta_inf *)plc->payload;
    td_u32 pos = inf->stru_size;
    td_u8 *data = TD_NULL;
    td_u8 cnt = inf->element_cnt;
    td_u8 data_len;
    td_u8 i;

    if ((inf->stru_size < sizeof(mrs_query_sta_inf)) || (cnt == 0)
        || (plc->payload_len > PLC_DATA_LEN_MAX)) {
        return;
    }

    for (i = 0; i < cnt; i++) {
        switch (plc->payload[pos]) {
            case MRS_QUERY_SW_VERSION:
                data = plc->payload + pos + 2; /* data shifts pos +2B */
                data_len = plc->payload[pos + 1];
                mrs_cco_upg_get_sw_ver_rx(plc->addr, data, data_len);
                break;

            default:
                data = TD_NULL;
                data_len = 0;
                break;
        }

        pos += 2; /* data shifts pos +2B */
        pos += data_len;
    }
}

/* CCO equip test frame */
td_void mrs_cco_dut_plc_frame_rx(mrs_plc_frame_data *plc)
{
    mrs_dut_cco_frame *dut_frm = (mrs_dut_cco_frame *)plc->payload;
    td_u32 ret, err;
    mrs_plc_frame_data plc_frame = { 0 };
    td_u8 addr[EXT_PLC_MAC_ADDR_LEN] = { 0 };
    td_u8 meter[MRS_METER_ADDR_LEN] = { 0 };

    mrs_dut_handler dut_handlers[3] = { /* array of 3 functions */
        TD_NULL,
        mrs_cco_dut_set_power_gain, /* MRS_SET_POWER_GAIN */
        mrs_cco_dut_get_verion,     /* MRS_QRY_STA_VERSION */
    };

    /* filter invalid frame */
    if (dut_frm->seq == g_dut_cco_seq) {
        /* Duplicate frames */
        return;
    }

    g_dut_cco_seq = dut_frm->seq;

    uapi_get_my_mac(addr, sizeof(addr));
    mrs_convert_mac_to_meter(addr, sizeof(addr), meter, sizeof(meter));
    if (memcmp(meter, dut_frm->src_addr, sizeof(meter)) != 0) {
        return;
    }

    if ((dut_frm->stru_size < sizeof(mrs_dut_cco_frame)) ||
        (dut_frm->stru_size + dut_frm->data_len != plc->payload_len) ||
        (dut_frm->data_len > PLC_DATA_LEN_MAX)) {
        return;
    }

    if ((dut_frm->dir != MRS_PLC_DN_FLG) || (dut_frm->cmd_id > MRS_QRY_STA_VERSION)
        || (dut_frm->vendor != MRS_DUT_VENDOR)) {
        return;
    }

    if (uapi_crc32(0, plc->payload + 8, (td_u32)(dut_frm->data_len + dut_frm->stru_size - 8)) != /* shifts 8 */
        dut_frm->crc) {
        return;
    }

    if (dut_handlers[dut_frm->cmd_id] != TD_NULL) {
        err = (td_u32)memset_s(&plc_frame, sizeof(mrs_plc_frame_data), 0, sizeof(mrs_plc_frame_data));
        err |= (td_u32)memset_s(plc_frame.addr, sizeof(plc_frame.addr), 0xFF, sizeof(plc_frame.addr));
        if (err != EOK) {
            return;
        }

        plc_frame.id = PLC_CMD_ID_DUT_CCO_TEST;

        ret = dut_handlers[dut_frm->cmd_id](dut_frm, &plc_frame);
        if (ret != EXT_ERR_SUCCESS) {
            mrs_free(plc_frame.payload);
            return;
        }

        mrs_plc_frame_send(&plc_frame);

        mrs_free(plc_frame.payload);
    }
}

td_u32 mrs_cco_dut_set_power_gain(mrs_dut_cco_frame *dut_frm, mrs_plc_frame_data *reply_plc)
{
    td_u32 ret;
    td_u16 data_len = dut_frm->data_len;
    td_u8 *data = dut_frm->data;
    td_u8 power_step;
    td_u8 afe_pga_gain;
    td_u8 ch;

    if ((data_len != 2) || (data == TD_NULL)) { /* length: 2B */
        return EXT_ERR_BAD_DATA;
    }

    power_step = data[0];
    afe_pga_gain = data[1];

    if (uapi_get_seconds() > MRS_SET_POWER_GAIN_TIME) {
        return EXT_ERR_CFG_NOT_ALLOW;
    }

    ret = uapi_set_power_step((td_s8)power_step);
    ret |= uapi_set_afe_pga_gain(afe_pga_gain);
    if (ret == EXT_ERR_SUCCESS) {
        ch = 0;
    } else {
        ch = 1;
    }

    reply_plc->payload = (td_u8 *)mrs_cco_dut_create_reply_frame(dut_frm, &ch, sizeof(ch));
    if (reply_plc->payload == TD_NULL) {
        return EXT_ERR_FAILURE;
    }

    reply_plc->payload_len = sizeof(mrs_dut_cco_frame) + sizeof(ch);

    return EXT_ERR_SUCCESS;
}

td_u32 mrs_cco_dut_get_verion(mrs_dut_cco_frame *dut_frm, mrs_plc_frame_data *reply_plc)
{
    td_u16 ver_len;
    td_u8 version[MRS_SW_VERSION_MAX] = { 0 };
    if (dut_frm->data_len != 0) {
        return EXT_ERR_BAD_DATA;
    }

    ver_len = (td_u16)strlen(MRS_SW_VERSION_STR);
    if ((ver_len > MRS_SW_VERSION_MAX) || (ver_len == 0)) {
        return EXT_ERR_FAILURE;
    }

    if (memcpy_s(version, MRS_SW_VERSION_MAX, MRS_SW_VERSION_STR, ver_len) != EOK) {}

    reply_plc->payload = (td_u8 *)mrs_cco_dut_create_reply_frame(dut_frm, version, (td_u16)MRS_SW_VERSION_MAX);
    reply_plc->payload_len = sizeof(mrs_dut_cco_frame) + MRS_SW_VERSION_MAX;

    return EXT_ERR_SUCCESS;
}

mrs_dut_cco_frame *mrs_cco_dut_create_reply_frame(const mrs_dut_cco_frame *dut_frm, const td_u8 *data, td_u16 len)
{
    mrs_dut_cco_frame *dut_reply = TD_NULL;
    td_u32 err;
    td_u16 reply_len = sizeof(mrs_dut_cco_frame) + len;
    td_u8 addr[EXT_PLC_MAC_ADDR_LEN] = { 0 };

    dut_reply = (mrs_dut_cco_frame *)mrs_malloc(reply_len);
    if (dut_reply == TD_NULL) {
        return TD_NULL;
    }

    err = (td_u32)memcpy_s(dut_reply, reply_len, dut_frm, sizeof(mrs_dut_cco_frame));
    err |= (td_u32)memcpy_s(dut_reply->data, len, data, len);
    if (err != EOK) {
        mrs_free(dut_reply);
        return TD_NULL;
    }

    uapi_get_my_mac(addr, sizeof(addr));
    mrs_convert_meter_to_mac(addr, sizeof(addr), dut_reply->src_addr, EXT_PLC_MAC_ADDR_LEN);

    dut_reply->dir = MRS_PLC_UP_FLG;
    dut_reply->data_len = len;

    dut_reply->crc =
        uapi_crc32(0, (td_u8 *)dut_reply + 8, (td_u32)(dut_reply->data_len + dut_reply->stru_size - 8)); /* shifts 8 */

    return dut_reply;
}

#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_CCO) */


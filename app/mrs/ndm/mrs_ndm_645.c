/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS NDM 645 handle.
 */

#include "mrs_ndm_645.h"
#include "mrs_ndm_plc.h"
#include "mrs_msg.h"
#include "mrs_proto_645.h"
#include "mrs_common_tools.h"
#include "soc_mac_nv.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_NDM)
#define MRS_DI_CCO_DUT_TRANS 0xE010

#define MRS_DI_NDM_SET_WORK_MDOE_TRANS 0xE001
#define MRS_DI_NDM_REQ_WORK_MDOE_TRANS 0xE002


td_void mrs_ndm_dut_cco_uart_rx(td_pbyte data, td_u16 data_len, td_pbyte addr, td_u8 addr_len);
td_bool mrs_ndm_dut_plc_frame_check(const mrs_plc_frame_data *plc);
td_void mrs_ndm_test_cmd_uart_rx(td_pbyte data, td_u16 data_len, td_pbyte addr, td_u8 addr_len);

/* processing for 645 frame recieving from serial ports */
td_void mrs_msg_on_app_frame_rx_645(EXT_CONST mrs_queue_msg *msg)
{
    td_pbyte data = (td_pbyte)msg->param1;
    td_u16 data_len = msg->param0;
    td_u8 fn;
    td_u8 is_no_mac_state;

    mrs_proto_645_frame_inf frame = { { 0 }, 0, 0, TD_NULL };

    if (mrs_proto_645_decode_frame(data, data_len, &frame) != EXT_ERR_SUCCESS) {
        mrs_free(data);
        return;
    }

    fn = frame.ctrl & MRS_DUT_645_CTRL_FN_MASK;
    switch (fn) {
        case MRS_DUT_645_CTRL_FN_CCO: /* CCO equip frame */
            mrs_ndm_dut_cco_uart_rx(frame.data, frame.data_len, frame.addr, (td_u8)sizeof(frame.addr));
            break;

        case MRS_DUT_645_CTRL_FN_CIU:
            is_no_mac_state = TD_TRUE;
            uapi_p2p_set_ciu_no_mac_state_p1901(is_no_mac_state);
            mrs_ndm_send_ciu_plc_frame(data, data_len, frame.addr, sizeof(frame.addr), is_no_mac_state);
            break;
        case MRS_DUT_645_CTRL_FN_NDM:
            mrs_ndm_test_cmd_uart_rx(frame.data, frame.data_len, frame.addr, (td_u8)sizeof(frame.addr));
            break;
        case MRS_DUT_645_CTRL_FN_STA: /* STA equip frame */
        default:
            is_no_mac_state = TD_TRUE;
            uapi_p2p_set_ciu_no_mac_state_p1901(is_no_mac_state);
            mrs_ndm_send_sta_dut_plc_frame(data, data_len, frame.addr, (const td_u8)sizeof(frame.addr),
                                           is_no_mac_state);
            break;
    }

    mrs_free(data);
}
td_void mrs_ndm_set_work_mode_uart_frame(td_u8 result)
{
    mrs_proto_645_frame_inf inf = { { 0 }, 0, 0, TD_NULL };
    td_u32 ret;
    td_u8 *frame = TD_NULL;
    td_u16 frame_len;

    inf.data_len = (td_u8)(3); /* 3: di (2B) + work_mode (1B) */
    inf.data = (td_u8 *)mrs_malloc(inf.data_len);
    if (inf.data == TD_NULL) {
        return;
    }

    inf.data[0] = 0x01;
    inf.data[1] = 0xE0; /* 0xE0 */
    inf.data[2] = result; /* index 2: 0x01 */

    inf.ctrl = MRS_DUT_645_CTRL_FN_NDM | 0x80; /* 0x80 */

    ret = mrs_proto_645_create_frame(&inf, &frame, &frame_len);
    mrs_free(inf.data);
    if (ret == EXT_ERR_SUCCESS) {
        mrs_uart_tx(frame, frame_len);
    }

    mrs_free(frame);
}
td_void mrs_ndm_req_work_mode_uart_frame(td_void)
{
    mrs_proto_645_frame_inf inf = { { 0 }, 0, 0, TD_NULL };
    td_u32 ret;
    td_u8 *frame = TD_NULL;
    td_u16 frame_len;

    inf.data_len = (td_u8)(5); /* 5: di (2B) + tei(2B) + work_mode (1B) */
    inf.data = (td_u8 *)mrs_malloc(inf.data_len);
    if (inf.data == TD_NULL) {
        return;
    }
    diag_cmd_config_ndm_connect_para_stru nv_para = {0};

    ret = uapi_nv_read(ID_DIAG_CMD_CONFIG_NDM_CONNECT_PARA, (td_pvoid)&nv_para, sizeof(nv_para));
    if (ret != EXT_ERR_SUCCESS) {
        return;
    }

    inf.data[0] = 0x02; /* 0x02 */
    inf.data[1] = 0xE0; /* 0xE0 */
    inf.data[2] = nv_para.ndm_tei & EXT_U8_MAX;              /* index 2: ndm_tei */
    inf.data[3] = (nv_para.ndm_tei >> EXT_U8_BITS) & EXT_U8_MAX;       /* index 3: ndm_tei */
    inf.data[4] = nv_para.work_mode;                   /* index 4: work_mode */

    inf.ctrl = MRS_DUT_645_CTRL_FN_NDM | 0x80; /* 0x80 */

    ret = mrs_proto_645_create_frame(&inf, &frame, &frame_len);
    mrs_free(inf.data);
    if (ret == EXT_ERR_SUCCESS) {
        mrs_uart_tx(frame, frame_len);
    }

    mrs_free(frame);
}

td_u32 mrs_ndm_set_ndm_info(td_u8 work_mode, td_u16 tei)
{
    diag_cmd_config_ndm_connect_para_stru nv_para = {0};

    if (!(work_mode <= EXT_NDM_SEARCH_NET_MODE_SWITCH_COM && work_mode > EXT_NDM_UNKNOWN_WORK_MODE)) {
        return EXT_ERR_FAILURE;
    }

    if (!((tei) <= EXT_NDM_MAX_TEI && (tei) >= EXT_NDM_MIN_TEI)) {
        return EXT_ERR_FAILURE;
    }

    td_u32 ret = uapi_nv_read(ID_DIAG_CMD_CONFIG_NDM_CONNECT_PARA, (td_pvoid)&nv_para, sizeof(nv_para));
    if (ret != EXT_ERR_SUCCESS) {
        return EXT_ERR_FAILURE;
    }

    nv_para.work_mode = work_mode;
    nv_para.ndm_tei = tei;
    if (uapi_nv_write(ID_DIAG_CMD_CONFIG_NDM_CONNECT_PARA, (td_pvoid)&nv_para, sizeof(nv_para)) != EXT_ERR_SUCCESS) {
        return EXT_ERR_FAILURE;
    }

    return EXT_ERR_SUCCESS;
}

td_void mrs_ndm_test_cmd_uart_rx(td_pbyte data, td_u16 data_len, td_pbyte addr, td_u8 addr_len)
{
    td_u16 di;
    td_u8 work_mode;
    td_u16 tei;
    ext_unref_param(addr);
    ext_unref_param(addr_len);

    if (data_len < 2) { /* 2:di (2B) */
        return;
    }
    mrs_proto_645_data_decode(data, data_len);

    di = uapi_make_u16(data[0], data[1]);
    if (di == MRS_DI_NDM_SET_WORK_MDOE_TRANS) {
        if (data_len < 5) { /* 5:di (2B) + tei(2B) + work_mode (1B) */
            return;
        }
        tei = uapi_make_u16(data[2], data[3]); /* index 2,3: tei */
        work_mode = data[4];    /* index 4: work_mode */
        td_u32 ret = mrs_ndm_set_ndm_info(work_mode, tei);
        mrs_ndm_set_work_mode_uart_frame((td_u8)ret);
    } else if (di == MRS_DI_NDM_REQ_WORK_MDOE_TRANS) {
        mrs_ndm_req_work_mode_uart_frame();
    }
}

td_void mrs_ndm_dut_cco_uart_rx(td_pbyte data, td_u16 data_len, td_pbyte addr, td_u8 addr_len)
{
    td_u16 di;
    td_u16 cmd;
    td_u16 length;

    if (data_len < 6) { /* 6:di (2B) + cmd id (2B) + length (2B) + data (length) */
        return;
    }

    mrs_proto_645_data_decode(data, data_len);

    di = uapi_make_u16(data[0], data[1]);
    cmd = uapi_make_u16(data[2], data[3]);    /* index 2&3: cmd */
    length = uapi_make_u16(data[4], data[5]); /* index 4&5: length */
    if ((di != MRS_DI_CCO_DUT_TRANS) || (cmd != PLC_CMD_ID_DUT_CCO_TEST) ||
        (length < sizeof(mrs_dut_cco_frame)) || (length + 6 != data_len)) { /* 6:di (2B),cmd id (2B) ,length (2B) */
        return;
    }

    mrs_ndm_send_cco_dut_plc_frame(data + EXT_METER_ADDR_LEN, data_len - EXT_METER_ADDR_LEN, addr, addr_len);
}

td_void mrs_ndm_cco_dut_uart_frame(td_pbyte data, td_u16 data_len, const td_pbyte addr, td_u8 addr_len)
{
    mrs_proto_645_frame_inf inf = { { 0 }, 0, 0, TD_NULL };
    errno_t err;
    td_u32 ret;
    td_u8 *frame = TD_NULL;
    td_u16 frame_len;

    inf.data_len = (td_u8)(data_len + 6); /* 6: di (2B) + cmd id (2B) + length (2B) + data (length) */
    inf.data = (td_u8 *)mrs_malloc(inf.data_len);
    if (inf.data == TD_NULL) {
        return;
    }

    inf.data[0] = 0x10;
    inf.data[1] = 0xE0;
    inf.data[2] = 0x01;                            /* index 2: 0x01 */
    inf.data[3] = 0xF0;                            /* index 3: 0xF0 */
    inf.data[4] = (td_u8)(data_len & EXT_U8_MAX); /* index 4: data length low 8bits */
    inf.data[5] = (td_u8)((data_len >> EXT_U8_BITS) & EXT_U8_MAX); /* index 5: data length high 8bits */

    err = memcpy_s(inf.data + 6, inf.data_len - 6, data, data_len); /* 6:di (2B) + cmd id (2B) + length (2B) */
    if (err != EOK) {
        mrs_free(inf.data);
        return;
    }

    err = memcpy_s(inf.addr, sizeof(inf.addr), addr, addr_len);
    if (err != EOK) {
        mrs_free(inf.data);
        return;
    }

    mrs_hex_invert(inf.addr, sizeof(inf.addr));
    inf.ctrl = MRS_DUT_645_CTRL_FN_CCO | 0x80;

    ret = mrs_proto_645_create_frame(&inf, &frame, &frame_len);
    mrs_free(inf.data);
    if (ret == EXT_ERR_SUCCESS) {
        mrs_uart_tx(frame, frame_len);
    }

    mrs_free(frame);
}

td_bool mrs_proto_is_rcv_p2p_pkt_by_dest_mac(EXT_CONST td_u8 *dest_mac, td_u8 mac_len)
{
    if (dest_mac == TD_NULL || mac_len != EXT_PLC_MAC_ADDR_LEN) {
        return TD_FALSE;
    }

    return mrs_is_wild_mac(dest_mac);
}

#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_NDM) */

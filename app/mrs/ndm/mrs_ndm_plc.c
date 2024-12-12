/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS STA plc handle.
 */

#include "mrs_ndm_plc.h"
#include "mrs_ndm_645.h"
#include "mrs_common_tools.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_NDM)
#ifdef __cplusplus
extern "C" {
#endif

td_u16 g_ndm_plc_seq = 0;
td_u8 g_dest_cco_addr[EXT_PLC_MAC_ADDR_LEN] = { 0 };

td_bool mrs_ndm_dut_plc_frame_check(const mrs_plc_frame_data *plc);
td_bool mrs_ndm_cco_dut_plc_frame_check(const mrs_plc_frame_data *plc);

td_void mrs_ndm_dut_init(td_void)
{
    g_ndm_plc_seq = (td_u16)uapi_get_random_num32(1, 65535); /* radom number: min 1,max 65535 */
}

td_void mrs_ndm_send_cco_dut_plc_frame(const td_pbyte data, const td_u16 data_len,
                                       const td_pbyte meter, const td_u8 meter_len)
{
    errno_t err;
    mrs_plc_frame_data plc;

    err = memset_s(plc.addr, sizeof(plc.addr), 0xFF, sizeof(plc.addr));
    if (err != EOK) {
        return;
    }

    plc.id = PLC_CMD_ID_DUT_CCO_TEST;
    plc.bc_flag = 0;
    plc.option = 0;
    plc.payload_len = data_len;
    plc.payload = data;

    /* Get seq from PLC frame */
    g_ndm_plc_seq = uapi_make_u16(data[2], data[3]); /* index 2&3: sequence */
    err = memcpy_s(g_dest_cco_addr, sizeof(g_dest_cco_addr), meter, meter_len);
    if (err != EOK) {
        return;
    }

    mrs_plc_frame_send(&plc);
}

td_void mrs_ndm_send_sta_dut_plc_frame(const td_pbyte data, const td_u16 data_len,
                                       const td_pbyte meter, const td_u8 meter_len, td_bool is_no_mac)
{
    mrs_dut_test_frame *frame = TD_NULL;
    ext_p2p_mac_frame_ctrl ctrl;
    errno_t err;
    td_u16 payload_len = sizeof(mrs_dut_test_frame) + data_len;

    err = memset_s(&ctrl, sizeof(ctrl), 0, sizeof(ctrl));
    if (err != EOK) {
        return;
    }

    ctrl.data = (td_pbyte)mrs_malloc(payload_len);
    if (ctrl.data == TD_NULL) {
        return;
    }

    if (memset_s(ctrl.data, payload_len, 0, payload_len) != EOK) {
        mrs_free(ctrl.data);
        return;
    }

    mrs_convert_meter_to_mac(meter, meter_len, ctrl.dest_mac, sizeof(ctrl.dest_mac));
    if (is_no_mac == TD_TRUE) {
        /*
         * When CIU uses MAC-free communication, configure ctrl.dest_mac and ctrl.src_mac as the MAC address
         * of the target site CIU sets ctrl.dest_mac to its own MAC address after successful handshake with
         * the target site (except wildcard MAC addresses).
         */
        err = memcpy_s(ctrl.src_mac, sizeof(ctrl.src_mac), ctrl.dest_mac, sizeof(ctrl.dest_mac));
        if (err != EOK) {
            mrs_free(ctrl.data);
            return;
        }
    } else {
        /*
         * When the CIU uses fixed MAC communication, set ctrl.dest_mac as the MAC address of the target site,
         * and configure ctrl.src_mac as its own MAC address. In addition, the identification and processing
         * flow of fixed MAC communication for CIU should be added at the application layer of STA.
         */
    }

    ctrl.lid = 3; /* priority 3 */
    ctrl.port_num = EXT_DMS_CHL_MAC_PORT_APP;
    ctrl.ctrl = EXT_DMS_CHL_TX_CTRL_NA;
    ctrl.id = PLC_CMD_ID_DUT_STA_TEST;
    ctrl.data_size = payload_len;

    frame = (mrs_dut_test_frame *)ctrl.data;
    frame->stru_ver = MRS_PLC_PROTO_VERSION;
    frame->stru_size = sizeof(mrs_dut_test_frame);
    frame->dir = MRS_PLC_DN_FLG;
    frame->seq = ++g_ndm_plc_seq;
    frame->vendor = MRS_DUT_VENDOR;
    frame->data_len = data_len;
    err = memcpy_s(frame->data, frame->data_len, data, data_len);
    if (err != EOK) {
        mrs_free(ctrl.data);
        return;
    }

    frame->crc = uapi_crc32(0, ctrl.data + ext_offset_of_member(mrs_dut_test_frame, vendor),
        payload_len - ext_offset_of_member(mrs_dut_test_frame, vendor));

    uapi_p2p_chl_tx(&ctrl, sizeof(ctrl));
    mrs_free(ctrl.data);
}

td_void mrs_ndm_send_ciu_plc_frame(const td_pbyte data, const td_u16 data_len,
                                   const td_pbyte meter, const td_u8 meter_len,
                                   td_bool is_no_mac)
{
    mrs_ciu_transmit_inf *frame = TD_NULL;
    ext_p2p_mac_frame_ctrl ctrl;
    errno_t err;
    td_u16 payload_len = sizeof(mrs_ciu_transmit_inf) + data_len;

    err = memset_s(&ctrl, sizeof(ctrl), 0, sizeof(ctrl));
    if (err != EOK) {
        return;
    }

    ctrl.data = (td_pbyte)mrs_malloc(payload_len);
    if (ctrl.data == TD_NULL) {
        return;
    }

    err = memset_s(ctrl.data, payload_len, 0, payload_len);
    if (err != EOK) {
        mrs_free(ctrl.data);
        return;
    }

    mrs_convert_meter_to_mac(meter, meter_len, ctrl.dest_mac, sizeof(ctrl.dest_mac));
    if (is_no_mac == TD_TRUE) {
        /*
         * When CIU uses MAC-free communication, configure ctrl.dest_mac and ctrl.src_mac as the MAC address
         * of the target site CIU sets ctrl.dest_mac to its own MAC address after successful handshake with
         * the target site (except wildcard MAC addresses).
         */
        err = memcpy_s(ctrl.src_mac, sizeof(ctrl.src_mac), ctrl.dest_mac, sizeof(ctrl.dest_mac));
        if (err != EOK) {
            mrs_free(ctrl.data);
            return;
        }
    } else {
        /*
         * When the CIU uses fixed MAC communication, set ctrl.dest_mac as the MAC address of the target site,
         * and configure ctrl.src_mac as its own MAC address. In addition, the identification and processing
         * flow of fixed MAC communication for CIU should be added at the application layer of STA.
         */
    }

    ctrl.lid = 3; /* priority 3 */
    ctrl.port_num = EXT_DMS_CHL_MAC_PORT_APP;
    ctrl.ctrl = EXT_DMS_CHL_TX_CTRL_NA;
    ctrl.id = PLC_CMD_ID_CIU_TRANSMIT;
    ctrl.data_size = payload_len;

    frame = (mrs_ciu_transmit_inf *)ctrl.data;
    frame->stru_ver = MRS_PLC_PROTO_VERSION;
    frame->stru_size = sizeof(mrs_ciu_transmit_inf);
    frame->dir = MRS_DIR_CIU_TO_STA;
    frame->prm = MRS_PLC_PRM_HOST;
    frame->seq = ++g_ndm_plc_seq;
    frame->data_len = data_len;

    err = memcpy_s(frame->data, frame->data_len, data, data_len);
    if (err != EOK) {
        mrs_free(ctrl.data);
        return;
    }

    uapi_p2p_chl_tx(&ctrl, sizeof(ctrl));
    mrs_free(ctrl.data);
}

td_void mrs_ndm_ciu_plc_frame_rx(mrs_plc_frame_data *plc)
{
    mrs_ciu_transmit_inf *frame = (mrs_ciu_transmit_inf *)plc->payload;

    /* Check frame direction. */
    if (frame->dir != MRS_DIR_STA_TO_CIU && frame->dir != MRS_DIR_CIU_TO_STA) {
        return;
    }

    /* Check frame length */
    if ((plc->payload_len != frame->stru_size + frame->data_len) || (frame->data_len > PLC_DATA_LEN_MAX)) {
        return;
    }

    /* Reply frame, check seq */
    if ((frame->prm == MRS_PLC_PRM_SLAVE) && ((td_u8)g_ndm_plc_seq != frame->seq)) {
        return;
    }

    uapi_app_received_p2p_pkt(plc->addr, sizeof(plc->addr));

    /* Send frame to uart */
    mrs_ndm_ciu_uart_frame(plc->payload + frame->stru_size, (td_u16)frame->data_len);
    g_ndm_plc_seq++;
}

td_bool mrs_ndm_dut_plc_frame_check(const mrs_plc_frame_data *plc)
{
    mrs_dut_test_frame *frame = (mrs_dut_test_frame *)plc->payload;
    td_u32 crc;

    /* Filter duplicate frames */
    if (frame->seq != g_ndm_plc_seq) {
        return TD_FALSE;
    }

    /* Check frame direction. */
    if (frame->dir != MRS_PLC_UP_FLG) {
        return TD_FALSE;
    }

    /* Check frame length */
    if ((plc->payload_len != frame->stru_size + frame->data_len) || (frame->data_len > PLC_DATA_LEN_MAX)) {
        return TD_FALSE;
    }

    /* Check vendor */
    if (frame->vendor != MRS_DUT_VENDOR) {
        return TD_FALSE;
    }

    /* Check crc */
    crc = uapi_crc32(0, plc->payload + ext_offset_of_member(mrs_dut_test_frame, vendor),
        plc->payload_len - ext_offset_of_member(mrs_dut_test_frame, vendor));
    if (crc != frame->crc) {
        return TD_FALSE;
    }

    return TD_TRUE;
}

td_bool mrs_ndm_cco_dut_plc_frame_check(const mrs_plc_frame_data *plc)
{
    mrs_dut_cco_frame *frame = (mrs_dut_cco_frame *)plc->payload;
    td_u32 crc;

    /* Filters */
    if (memcmp(frame->src_addr, g_dest_cco_addr, sizeof(g_dest_cco_addr)) != 0) {
        return TD_FALSE;
    }

    /* Filter duplicate frames */
    if (frame->seq != g_ndm_plc_seq) {
        return TD_FALSE;
    }

    /* Check frame direction. */
    if (frame->dir != MRS_PLC_UP_FLG) {
        return TD_FALSE;
    }

    /* Check frame length */
    if (plc->payload_len != frame->stru_size + frame->data_len) {
        return TD_FALSE;
    }

    /* Check vendor */
    if (frame->vendor != MRS_DUT_VENDOR) {
        return TD_FALSE;
    }

    /* Check crc */
    crc = uapi_crc32(0, plc->payload + ext_offset_of_member(mrs_dut_cco_frame, vendor),
        plc->payload_len - ext_offset_of_member(mrs_dut_cco_frame, vendor));
    if (crc != frame->crc) {
        return TD_FALSE;
    }

    return TD_TRUE;
}

td_void mrs_ndm_cco_dut_plc_frame_rx(mrs_plc_frame_data *plc)
{
    /* Check frame. */
    if (mrs_ndm_cco_dut_plc_frame_check(plc) != TD_TRUE) {
        return;
    }

    mrs_ndm_cco_dut_uart_frame(plc->payload, plc->payload_len, plc->addr, sizeof(plc->addr));
    g_ndm_plc_seq++;
}

td_void mrs_ndm_sta_dut_plc_frame_rx(mrs_plc_frame_data *plc)
{
    mrs_dut_test_frame *frame = (mrs_dut_test_frame *)plc->payload;

    /* Check frame. */
    if (mrs_ndm_dut_plc_frame_check(plc) != TD_TRUE) {
        return;
    }

    uapi_app_received_p2p_pkt(plc->addr, sizeof(plc->addr));

    mrs_ndm_sta_dut_uart_frame(plc->payload + frame->stru_size, (td_u16)frame->data_len);
    g_ndm_plc_seq++;
}

#ifdef __cplusplus
}
#endif
#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_NDM) */

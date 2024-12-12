/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS sta tf handle.
 */

#include "mrs_sta_tf.h"
#include "mrs_common_tools.h"
#include "mrs_sta_srv.h"
#include "soc_sal.h"
#include "dfx_sys.h"
#include "app_nv.h"
#include "soc_mdm_tf.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)

#define MRS_STA_TF_CHECK_CYCLE    60
#define MRS_STA_TF_TIMEOUT_LEN    1800
#define MRS_STA_TF_LEAVE_LEN_TH   900
#define MRS_STA_TF_START_INTERVAL 600

typedef struct {
    td_u32 start_time;
    td_u32 last_rcv_start_cmd_time;

    td_u8 tf_status;
    td_u8 is_show_dfx_log;
    td_u8 belong_addr[EXT_METER_ADDR_LEN];

    td_u32 tf_stop_time;
} mrs_sta_tf_ctx;

mrs_sta_tf_ctx g_mrs_sta_tf_ctx = { 0, 0, 0, 1, { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, 0 };
mrs_sta_tf_inf g_mrs_tf_conf;

td_u32 mrs_sta_tf_report_result(EXT_CONST mrs_plc_frame_data *tf_req);

mrs_sta_tf_ctx *mrs_sta_get_tf_ctx(td_void)
{
    return &g_mrs_sta_tf_ctx;
}

mrs_sta_tf_inf *mrs_sta_get_tf_conf(td_void)
{
    return &g_mrs_tf_conf;
}

td_u32 mrs_sta_tf_refresh_nv_para(td_void)
{
    mrs_sta_tf_inf *tf_conf = mrs_sta_get_tf_conf();
    mrs_sta_tf_inf nv_data;
    td_u32 ret1;
    td_u32 ret2;

    if (tf_conf == TD_NULL) {
        return EXT_ERR_NO_INITILIZATION;
    }

    ret1 = uapi_nv_read(ID_NV_APP_MRS_STA_TF, (td_pvoid)&nv_data, sizeof(nv_data));
    ret2 = (td_u32)memcpy_s(tf_conf, sizeof(mrs_sta_tf_inf), &nv_data, sizeof(mrs_sta_tf_inf));
    if ((ret1 != EXT_ERR_SUCCESS) || (ret2 != EXT_ERR_SUCCESS)) {
        tf_conf->tf_check_period = MRS_STA_TF_CHECK_CYCLE;
        tf_conf->tf_stop_while_not_receive = MRS_STA_TF_TIMEOUT_LEN;
        tf_conf->tf_unlock_while_leave_network = MRS_STA_TF_LEAVE_LEN_TH;
        tf_conf->tf_start_interval = MRS_STA_TF_START_INTERVAL;
        uapi_nv_write(ID_NV_APP_MRS_STA_TF, (td_pvoid)tf_conf, sizeof(mrs_sta_tf_inf));
    }
    return EXT_ERR_SUCCESS;
}

td_void mrs_sta_tf_meter_change(td_void)
{
    mrs_sta_tf_ctx *tf_ctx = mrs_sta_get_tf_ctx();
    (td_void) memset_s(tf_ctx, sizeof(mrs_sta_tf_ctx), 0, sizeof(mrs_sta_tf_ctx));
    (td_void) memset_s(tf_ctx->belong_addr, EXT_PLC_MAC_ADDR_LEN, 0xFF, EXT_PLC_MAC_ADDR_LEN);
    uapi_clear_belong_network();
    uapi_unlock_network();
}

td_void mrs_sta_tf_dfx_report_message(EXT_IN td_u16 id, EXT_IN td_pvoid packet, EXT_IN td_u16 packet_size)
{
    if (g_mrs_sta_tf_ctx.is_show_dfx_log == TD_FALSE) {
        return;
    }
    ext_diag_layer_msg layer_msg;
    layer_msg.id = id;
    layer_msg.src_mod_id = EXT_MOD_ID_APP_COMMON;
    layer_msg.dest_mod_id = EXT_MOD_ID_APP_COMMON;
    layer_msg.data = packet;
    layer_msg.data_size = packet_size;
    (td_void) diag_report_sys_msg(layer_msg, EXT_MSG_SYS_L3);
}

td_void mrs_sta_tf_handle_start(td_void)
{
    mrs_sta_tf_ctx *tf_ctx = mrs_sta_get_tf_ctx();
    mrs_sta_tf_inf *mrs_conf = mrs_sta_get_tf_conf();

    if ((tf_ctx->tf_stop_time > 0) &&
        (uapi_get_seconds() < (tf_ctx->tf_stop_time + mrs_conf->tf_start_interval)) &&
        (uapi_get_tf_mode() == TF_OFF)) {
        return;
    }

    if (uapi_get_tf_mode() != TF_ON_WITH_STA_JOIN_BY_TF_RESULT) {
        mrs_sta_tf_refresh_nv_para();
        (td_void) memset_s(tf_ctx->belong_addr, EXT_PLC_MAC_ADDR_LEN, 0xFF, EXT_PLC_MAC_ADDR_LEN);
        mrs_timer_start(MRS_TIMER_ID_TF_CYCLE_CHECK, uapi_sec_2_ms(mrs_conf->tf_check_period), EXT_TIMER_TYPE_PERIOD);
        tf_ctx->start_time = uapi_get_seconds();
    }

    uapi_set_tf_mode(TF_ON_WITH_STA_JOIN_BY_TF_RESULT);
    tf_ctx->last_rcv_start_cmd_time = uapi_get_seconds();
    tf_ctx->tf_status = MRS_TF_STATUE_IDENTIFYING;
}

td_void mrs_sta_tf_handle_stop(EXT_CONST mrs_plc_frame_data *plc)
{
    mrs_sta_tf_ctx *tf_ctx = mrs_sta_get_tf_ctx();
    if (uapi_get_tf_mode() == TF_ON_WITH_STA_JOIN_BY_TF_RESULT) {
        uapi_set_tf_mode(TF_OFF);

        if ((!mrs_is_invalid_mac_addr(plc->addr)) && (!mrs_is_broadcast_mac_addr(plc->addr))) {
            (td_void) memcpy_s(tf_ctx->belong_addr, EXT_PLC_MAC_ADDR_LEN, plc->addr, EXT_PLC_MAC_ADDR_LEN);
        }

        tf_ctx->tf_stop_time = uapi_get_seconds();
        if (plc->id == PLC_CMD_ID_STOP_SEARCH) {
            tf_ctx->tf_stop_time = 0;
        }

        mrs_timer_stop(MRS_TIMER_ID_TF_CYCLE_CHECK);
        tf_ctx->tf_status = MRS_TF_STATUS_COMPLETED;

        uapi_unlock_network();
    }
}

td_void mrs_sta_set_meterlist_header(mrs_plc_meterlist_up *meter_list_resp,
    EXT_CONST mrs_plc_meterlist_dl *meter_list_req)
{
    mrs_sta_tf_ctx *tf_ctx = mrs_sta_get_tf_ctx();

    mrs_sta_srv_ctx *srv_ctx = mrs_sta_get_srv_ctx();
    td_u8 mac[EXT_PLC_MAC_ADDR_LEN] = { 0 };
    td_u8 type;
    uapi_get_local_mac_addr(mac, sizeof(mac));

    meter_list_resp->interface_ver = MRS_PLC_PROTO_VERSION;
    meter_list_resp->stru_len = sizeof(mrs_plc_meterlist_up);

    if (tf_ctx->tf_status == MRS_TF_STATUS_COMPLETED) {
        meter_list_resp->status = 0;
    } else {
        meter_list_resp->status = 1;
    }
    type = mrs_is_product_relay() ? MRS_DEVICE_TYPE_PLC_RELAY : MRS_DEVICE_TYPE_PLC_METER;
    if (type == MRS_DEVICE_TYPE_PLC_RELAY) {
        meter_list_resp->meter_num = 0;
        (td_void) memcpy_s(meter_list_resp->asset, MRS_METER_ADDR_LEN, mac, MRS_METER_ADDR_LEN);
        mrs_hex_invert(meter_list_resp->asset, MRS_METER_ADDR_LEN);
    } else {
        meter_list_resp->meter_num = mrs_invalid_meter(srv_ctx->meter) ? 0 : 1;
        (td_void) memcpy_s(meter_list_resp->asset, MRS_METER_ADDR_LEN, srv_ctx->meter, MRS_METER_ADDR_LEN);
    }
    meter_list_resp->type = type;

    (td_void) memcpy_s(meter_list_resp->id, EXT_PLC_MAC_ADDR_LEN, mac, EXT_PLC_MAC_ADDR_LEN);
    mrs_hex_invert(meter_list_resp->id, EXT_PLC_MAC_ADDR_LEN);

    meter_list_resp->seq = meter_list_req->seq;

    if (mrs_is_broadcast_mac_addr(tf_ctx->belong_addr)) {
        meter_list_resp->result = MRS_TF_RESULT_NONE;
    } else if (memcmp(meter_list_req->src, tf_ctx->belong_addr, EXT_PLC_MAC_ADDR_LEN) == 0) {
        meter_list_resp->result = MRS_TF_RESULT_BELONG;
    } else {
        meter_list_resp->result = MRS_TF_RESULT_NOT_BELONG;
    }
    (td_void) memcpy_s(meter_list_resp->src, EXT_PLC_MAC_ADDR_LEN, srv_ctx->mac, EXT_PLC_MAC_ADDR_LEN);
    (td_void) memcpy_s(meter_list_resp->dst, EXT_PLC_MAC_ADDR_LEN, meter_list_req->src, EXT_PLC_MAC_ADDR_LEN);
}

td_u32 mrs_sta_tf_report_result(EXT_CONST mrs_plc_frame_data *plc_req)
{
    mrs_sta_srv_ctx *srv_ctx = mrs_sta_get_srv_ctx();
    mrs_plc_frame_data plc_resp;
    mrs_plc_meterlist_dl *meter_list_req = (mrs_plc_meterlist_dl *)plc_req->payload;
    mrs_plc_meterlist_up *meter_list_resp = TD_NULL;
    mrs_cmd_meterlist_item *item = TD_NULL;
    td_u16 tf_frame_length = sizeof(mrs_plc_meterlist_up) + sizeof(mrs_cmd_meterlist_item);
    td_u32 ret;
    td_u8 type = mrs_is_product_relay() ? MRS_DEVICE_TYPE_PLC_RELAY : MRS_DEVICE_TYPE_PLC_METER;
    td_u8 protocol = mrs_sta_get_meter_protocol();

    meter_list_resp = mrs_malloc(tf_frame_length);
    if (meter_list_resp == TD_NULL) {
        return EXT_ERR_MALLOC_FAILURE;
    }
    (td_void) memset_s(&plc_resp, sizeof(mrs_plc_frame_data), 0, sizeof(mrs_plc_frame_data));
    (td_void) memset_s(meter_list_resp, tf_frame_length, 0, tf_frame_length);
    item = (mrs_cmd_meterlist_item *)meter_list_resp->meter_list;

    /* This command is used to query the header information of the upstream data registered by the slave node. */
    mrs_sta_set_meterlist_header(meter_list_resp, meter_list_req);

    /* Filling Meter Data */
    (td_void) memcpy_s(item->addr, MRS_METER_ADDR_LEN, srv_ctx->meter, MRS_METER_ADDR_LEN);
    item->type = type;
    item->protocol = protocol;

    (td_void) memcpy_s(plc_resp.addr, EXT_PLC_MAC_ADDR_LEN, plc_req->addr, EXT_PLC_MAC_ADDR_LEN);
    plc_resp.id = plc_req->id;
    plc_resp.payload_len = tf_frame_length;
    plc_resp.payload = (td_u8 *)meter_list_resp;

    ret = mrs_plc_frame_send(&plc_resp);
    mrs_sta_tf_dfx_report_message(EXT_DSID_APP_MRS_TF_STA_PLC_SEND, (td_pvoid)plc_resp.payload,
                                  plc_resp.payload_len);

    mrs_free(meter_list_resp);
    return ret;
}

td_void mrs_sta_tf_handle_quary_result(EXT_CONST mrs_plc_frame_data *plc_req)
{
    mrs_sta_tf_ctx *tf_ctx = mrs_sta_get_tf_ctx();
    mrs_plc_meterlist_dl *tf_query_req = (mrs_plc_meterlist_dl *)plc_req->payload;

    if (tf_query_req->enable == MRS_TF_DISABLE) {
        mrs_sta_tf_handle_stop(plc_req);
    }

    if (tf_query_req->force_resp == 1 || tf_ctx->tf_status == MRS_TF_STATUS_COMPLETED) {
        mrs_sta_tf_report_result(plc_req);
    }
}

td_void mrs_sta_tf_plc_frame_rx(EXT_CONST mrs_plc_frame_data *plc)
{
    if (plc == TD_NULL || plc->payload == TD_NULL || mrs_sta_plc_chl_status() != TD_TRUE) {
        return;
    }
    mrs_sta_tf_dfx_report_message(EXT_DSID_APP_MRS_TF_STA_PLC_RECV, (td_pvoid *)plc->payload, plc->payload_len);

    if (plc->id == PLC_CMD_ID_START_SEARCH) { /* Start the transformer district identification. */
        mrs_plc_start_search_meter *tf_start_cmd = (mrs_plc_start_search_meter *)plc->payload;
        if (tf_start_cmd->option != MRS_SM_START_SEARCH) {
            return;
        }
        mrs_sta_tf_handle_start();
    } else if (plc->id == PLC_CMD_ID_METER_LIST) { /* Querying the transformer district identification result */
        mrs_plc_meterlist_dl *tf_query_cmd = (mrs_plc_meterlist_dl *)plc->payload;
        if (tf_query_cmd->option != MRS_SM_SEARCH_RESULT) {
            return;
        }
        mrs_sta_tf_handle_quary_result(plc);
    } else if (plc->id == PLC_CMD_ID_STOP_SEARCH) { /* Stop the transformer district identification. */
        mrs_sta_tf_handle_stop(plc);
    }
}

td_void mrs_sta_tf_cycle_check(td_void)
{
    mrs_sta_tf_ctx *tf_ctx = mrs_sta_get_tf_ctx();
    mrs_sta_tf_inf *tf_conf = mrs_sta_get_tf_conf();
    ext_mdm_lock_info lock_info = { 0 };

    if (mrs_time_sub_duration(uapi_get_seconds(),
                              tf_ctx->last_rcv_start_cmd_time) > tf_conf->tf_stop_while_not_receive) {
        tf_ctx->tf_status = MRS_TF_STATUS_COMPLETED;
        uapi_set_tf_mode(TF_OFF);
        uapi_unlock_network();
        mrs_timer_stop(MRS_TIMER_ID_TF_CYCLE_CHECK);
    }

    uapi_get_lock_info(&lock_info);
    if (lock_info.lock_status != EXT_UNLOCK_NETWORK_STATUS &&
        lock_info.leave_time_len > tf_conf->tf_unlock_while_leave_network) {
        uapi_unlock_network();
    }
}

#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_STA) */

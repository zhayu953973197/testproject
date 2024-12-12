/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS sta service.
 */

#include "mrs_sta_srv.h"
#include "mrs_sta_startup.h"
#include "mrs_sta_evt.h"
#include "mrs_sta_simu_meter.h"
#include "mrs_sta_io.h"
#include "mrs_common_tools.h"
#include "app_nv.h"
#include "mrs_sta_tf.h"
#include "mrs_sta_power_failure.h"
#include "mrs_common_capture.h"
#if defined(MRS_USR_DEMO)
#include "mrs_sta_usr.h"
#endif

#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)

static mrs_sta_srv_ctx *g_sta_srv_ctx = TD_NULL;

td_void mrs_sta_init_timer_func(td_void);
td_void mrs_sta_init_nv_detect(td_void);
td_void mrs_sta_on_timer_queue_ttl(td_void);
td_void mrs_sta_on_timer_queue_notify(td_void);
td_void mrs_sta_on_timer_queue_lock(td_void);
td_u32 mrs_sta_nv_change_detect(td_u8 nv_id);

mrs_sta_srv_ctx *mrs_sta_get_srv_ctx(td_void)
{
    return g_sta_srv_ctx;
}

mrs_sta_mr_queue *mrs_sta_get_queue(td_void)
{
    if (g_sta_srv_ctx != TD_NULL) {
        return &g_sta_srv_ctx->mr_queue;
    }

    return TD_NULL;
}

td_u32 mrs_sta_srv_init(td_void)
{
    mrs_sta_srv_ctx *sta = TD_NULL;
    mrs_simu_meter_inf simu = { 0, 0, 0, 0 };
    td_bool simu_enable = TD_FALSE;
    errno_t err;

    if (g_sta_srv_ctx != TD_NULL) {
        return EXT_ERR_INITILIZATION_ALREADY;
    }

    g_sta_srv_ctx = (mrs_sta_srv_ctx *)mrs_malloc(sizeof(mrs_sta_srv_ctx));
    if (g_sta_srv_ctx == TD_NULL) {
        return EXT_ERR_MALLOC_FAILUE;
    }

    err = memset_s(g_sta_srv_ctx, sizeof(mrs_sta_srv_ctx), 0, sizeof(mrs_sta_srv_ctx));
    if (err != EOK) {
        return EXT_ERR_FAILURE;
    }

    sta = g_sta_srv_ctx;

    uapi_get_local_mac_addr(sta->mac, sizeof(sta->mac));
    if (mrs_is_product_relay()) {
        /* relay */
        mrs_sta_set_mac_addr(sta->mac);
        return EXT_ERR_SUCCESS;
    }

    mrs_sta_event_init();
    mrs_sta_init_timer_func();
    mrs_sta_init_nv_detect();

#if defined(MRS_USR_DEMO)
    uapi_set_assoc_by_app_mac(TD_TRUE);
    mrs_sta_usr_srv_init();
#else
    mrs_proto_645_init();
#endif

    mrs_sta_queue_init(&sta->mr_queue);
    sta->baudrate = MRS_STA_DEFAULT_BAUDRATE;

    if (uapi_nv_read(ID_NV_APP_SIMU_METER, &simu, sizeof(simu)) == EXT_ERR_SUCCESS) {
        if (simu.simu_meter == 1) {
            simu_enable = TD_TRUE;
        }
    }

    mrs_simu_meter_enable(simu_enable);
    mrs_io_init();
    mrs_timer_start(MRS_TIMER_ID_STA_INIT, MRS_STA_INIT_TIME, EXT_TIMER_TYPE_ONCE);

    return EXT_ERR_SUCCESS;
}

td_void mrs_sta_init_timer_func(td_void)
{
    mrs_sta_register_on_timer_func(MRS_TIMER_ID_UART, mrs_proto_645_on_timer);
    mrs_sta_register_on_timer_func(MRS_TIMER_ID_STA_QUEUE_LOCK, mrs_sta_on_timer_queue_lock);
    mrs_sta_register_on_timer_func(MRS_TIMER_ID_STA_QUEUE, mrs_sta_on_timer_queue_notify);
    mrs_sta_register_on_timer_func(MRS_TIMER_ID_STA_TTL, mrs_sta_on_timer_queue_ttl);
    mrs_sta_register_on_timer_func(MRS_TIMER_ID_STA_INIT, mrs_sta_startup_detect);
    mrs_sta_register_on_timer_func(MRS_TIMER_ID_STA_IO_EVTOUT, mrs_sta_io_evtout_timeout_handle);
    mrs_sta_register_on_timer_func(MRS_TIMER_ID_STA_EVT_REPORT, mrs_sta_event_report_plc_timeout_handle);
    mrs_sta_register_on_timer_func(MRS_TIMER_ID_RST_IO, mrs_sta_reset_handle);
    mrs_sta_register_on_timer_func(MRS_TIMER_ID_PLC_LED_RX, mrs_sta_plc_rx_led_off);
    mrs_sta_register_on_timer_func(MRS_TIMER_ID_PLC_LED_TX, mrs_sta_plc_tx_led_off);
    mrs_sta_register_on_timer_func(MRS_TIMER_ID_TF_CYCLE_CHECK, mrs_sta_tf_cycle_check);
    mrs_sta_register_on_timer_func(MRS_TIMER_ID_STA_POWER_FAILURE, mrs_sta_create_power_failure_frame_proc);
    mrs_sta_register_on_timer_func(MRS_TIMER_ID_STA_PW_CHECK, mrs_pwf_check_main);
    mrs_sta_register_on_timer_func(MRS_TIMER_ID_STA_POWER_FAILURE_ACK, mrs_sta_handle_wait_power_failure_ack);
    mrs_sta_register_on_timer_func(MRS_TIMET_ID_PULL_RST_DELAY_TIMER, mrs_power_failure_pull_rst_handle);
    mrs_sta_register_on_timer_func(MRS_TIMER_ID_STA_ENTER_TESTMODE, mrs_enter_test_mode);
    mrs_sta_register_on_timer_func(MRS_TIMER_ID_STA_PHY_TRANS_TESTMODE, mrs_exit_phy_tans_test_mode);
    mrs_sta_register_on_timer_func(MRS_TIMER_ID_STA_PHY_LOOP_TESTMODE, mrs_exit_phy_loop_test_mode);
    mrs_sta_register_on_timer_func(MRS_TIMER_ID_STA_MAC_MSDU_TESTMODE, mrs_exit_mac_msdu_test_mode);
}

td_void mrs_sta_init_nv_detect(td_void)
{
    uapi_nv_register_change_nofity_proc(ID_NV_APP_SIMU_METER, ID_NV_APP_SIMU_METER, mrs_sta_nv_change_detect);
}

td_u32 mrs_sta_nv_change_detect(td_u8 nv_id)
{
    return mrs_msg_queue_send(MRS_MSG_ID_NV_CHANGE_NOTIFY, 0, nv_id, 0);
}

td_void mrs_msg_on_nv_change_notify(EXT_CONST mrs_queue_msg *msg)
{
    switch (msg->param1) {
        case ID_NV_APP_SIMU_METER:
            mrs_simu_meter_nv_detect();
            break;

        default:
            break;
    }
}

td_void mrs_sta_register_on_timer_func(mrs_timer_id timer_id, mrs_on_timer_fct on_timer)
{
    mrs_sta_srv_ctx *sta = mrs_sta_get_srv_ctx();

    if (timer_id >= MRS_TIMER_ID_MAX) {
        return;
    }

    sta->on_timer[timer_id] = on_timer;
}

td_void mrs_sta_on_timer_queue_ttl(td_void)
{
    mrs_sta_srv_ctx *sta = mrs_sta_get_srv_ctx();
    mrs_sta_queue_unlock(&sta->mr_queue);
    mrs_sta_queue_handle(&sta->mr_queue);
}

td_void mrs_sta_on_timer_queue_notify(td_void)
{
    mrs_sta_srv_ctx *sta = mrs_sta_get_srv_ctx();
    mrs_sta_queue_handle(&sta->mr_queue);
}

td_void mrs_sta_on_timer_queue_lock(td_void)
{
    uapi_usr_reboot(EXT_SYS_REBOOT_CAUSE_USR1);
}

td_void mrs_msg_on_timer(EXT_CONST mrs_queue_msg *msg)
{
    mrs_sta_srv_ctx *sta = mrs_sta_get_srv_ctx();
    td_u32 id = (td_u32)msg->param1;

    if (id >= MRS_TIMER_ID_MAX) {
        return;
    }

    if (sta->on_timer[id]) {
        sta->on_timer[id]();
    }
}

td_void mrs_sta_set_status(td_u8 status)
{
    mrs_sta_srv_ctx *sta = mrs_sta_get_srv_ctx();
    sta->status = status;
}

td_u8 mrs_sta_get_status(td_void)
{
    mrs_sta_srv_ctx *sta = mrs_sta_get_srv_ctx();
    return sta->status;
}

td_void mrs_sta_set_sync_mac_flag(td_bool flag)
{
    mrs_sta_srv_ctx *sta = mrs_sta_get_srv_ctx();
    sta->sync_mac = flag;
}

td_bool mrs_sta_get_sync_mac_flag(td_void)
{
    mrs_sta_srv_ctx *sta = mrs_sta_get_srv_ctx();
    return (td_bool)sta->sync_mac;
}

td_u32 mrs_sta_register_chl_notify(mrs_sta_chl_notify_fct callback, uintptr_t param)
{
    mrs_sta_srv_ctx *sta = mrs_sta_get_srv_ctx();

    if (sta->chl_notify_handle != TD_NULL) {
        return EXT_ERR_FULL;
    }

    sta->chl_notify_handle = callback;
    sta->chl_notify_param = param;
    return EXT_ERR_SUCCESS;
}

td_u32 mrs_sta_clear_chl_notify(mrs_sta_chl_notify_fct callback)
{
    mrs_sta_srv_ctx *sta = mrs_sta_get_srv_ctx();
    if (sta->chl_notify_handle != callback) {
        return EXT_ERR_NOT_FOUND;
    }

    sta->chl_notify_handle = TD_NULL;
    sta->chl_notify_param = 0;
    return EXT_ERR_SUCCESS;
}

td_u32 mrs_sta_chl_notify_handle(td_bool chl_status)
{
    mrs_sta_srv_ctx *sta = mrs_sta_get_srv_ctx();
    if (sta->chl_notify_handle != TD_NULL) {
        td_bool one_shot = TD_FALSE;
        sta->chl_notify_handle(chl_status, sta->chl_notify_param, &one_shot);
        if (one_shot == TD_TRUE) {
            sta->chl_notify_handle = TD_NULL;
            sta->chl_notify_param = 0;
        }
    }

    return EXT_ERR_SUCCESS;
}

td_u32 mrs_sta_frame_buf_add(const mrs_mr_queue_item *item, const td_pbyte data, td_u16 data_len)
{
    mrs_sta_srv_ctx *sta = mrs_sta_get_srv_ctx();
    mrs_sta_frame_buf_ctx *ctx = &sta->frame_buf;
    mrs_sta_frame_buf_item *node = ctx->item + ctx->current;

    if ((data == TD_NULL) || (data_len > MRS_645_FRAME_LEN_MAX)) {
        return EXT_ERR_FAILURE;
    }

    node->id = item->id;
    node->seq = item->seq;
    node->valid = TD_TRUE;
    node->data_len = data_len;
    node->protocol = item->protocol;
    node->timestamp = uapi_get_seconds();
    if (memcpy_s(node->data, sizeof(node->data), data, data_len) != EOK) {
        return EXT_ERR_FAILURE;
    }

    ctx->current = (ctx->current + 1) % MRS_CFG_STA_FRAME_BUF_CNT;

    return EXT_ERR_SUCCESS;
}

td_bool mrs_sta_bc_buf_find(const td_pbyte data, td_u16 data_len)
{
    mrs_sta_srv_ctx *sta = mrs_sta_get_srv_ctx();
    mrs_sta_frame_buf_ctx *ctx = &sta->frame_buf;
    mrs_sta_frame_buf_item *node = ctx->item;
    td_u32 timestamp = uapi_get_seconds();
    td_u32 i;

    for (i = 0; i < MRS_CFG_STA_FRAME_BUF_CNT; i++) {
        if (node[i].valid == TD_FALSE) {
            continue;
        }

        if (timestamp > node[i].timestamp + MRS_CFG_STA_FRAME_BUF_TIMEOUT) {
            node[i].valid = TD_FALSE;
            continue;
        }

        if ((node[i].data_len == data_len)
            && (memcmp(node[i].data, data, data_len)) == 0) {
            /* Duplicated */
            node[i].timestamp = timestamp;
            return TD_TRUE;
        }
    }

    return TD_FALSE;
}

td_u32 mrs_sta_frame_buf_find(const mrs_mr_queue_item *item, td_pbyte *data, td_u16 *data_len)
{
    mrs_sta_srv_ctx *sta = mrs_sta_get_srv_ctx();
    td_u32 timestamp = uapi_get_seconds();
    mrs_sta_frame_buf_ctx *ctx = &sta->frame_buf;
    mrs_sta_frame_buf_item *node = ctx->item;
    td_u8 i;

    if ((data == TD_NULL) || (data_len == TD_NULL)) {
        return EXT_ERR_FAILURE;
    }

    for (i = 0; i < MRS_CFG_STA_FRAME_BUF_CNT; i++) {
        if (node->valid == TD_FALSE) {
            continue;
        }

        if (timestamp > node->timestamp + MRS_CFG_STA_FRAME_BUF_TIMEOUT) {
            node->valid = TD_FALSE;
            continue;
        }

        if ((item->id == node->id) && (item->seq == node->seq)) {
            *data = node->data;
            *data_len = node->data_len;
            node->timestamp = timestamp;
            return EXT_ERR_SUCCESS;
        }
    }

    return EXT_ERR_FAILURE;
}

td_bool mrs_sta_plc_retry_filter(EXT_CONST mrs_mr_queue_item *item)
{
    td_u32 ret;
    td_pbyte buf = TD_NULL;
    td_u16 buf_len;

    if (item->id == PLC_CMD_ID_BC) {
        return mrs_sta_bc_buf_find((td_u8 *)item->data, item->data_len);
    }

    ret = mrs_sta_frame_buf_find(item, &buf, &buf_len);
    if (ret != EXT_ERR_SUCCESS) {
        return TD_FALSE;
    }

    switch (item->id) {
        case PLC_CMD_ID_XR:
        case PLC_CMD_ID_PR:
            item->rx_handle(item, buf, buf_len);
            return TD_TRUE;

        default:
            break;
    }

    return TD_FALSE;
}

td_u8 mrs_sta_get_meter_protocol(td_void)
{
    mrs_sta_srv_ctx *sta = mrs_sta_get_srv_ctx();

    if ((mrs_is_product_relay()) || (sta == TD_NULL)) {
        return 0;
    }

    return sta->protocol;
}

td_void mrs_sta_set_meter_protocol(td_u8 proto)
{
    mrs_sta_srv_ctx *sta = mrs_sta_get_srv_ctx();

    if ((mrs_is_product_relay()) || (sta == TD_NULL)) {
        return;
    }

    sta->protocol = proto;
}

#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_STA) */


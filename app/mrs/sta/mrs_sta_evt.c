/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS STA EVENT report.
 */

#include "mrs_sta_evt.h"
#include "mrs_sta_io.h"
#include "mrs_sta_srv.h"
#include "mrs_sta_plc.h"
#include "mrs_common_tools.h"
#include "mrs_common_power_failure.h"
#include "mrs_sta_power_failure.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)

#define MRS_STA_EVT_FROM_SIMU               1
#define MRS_STA_EVT_FROM_METER              2
#define MRS_STA_DETECT_POWER_FAILURE_ENABLE 1 /* enable for STA power down detection */

#define MRS_STA_EVT_STATUS_IDLE   0
#define MRS_STA_EVT_STATUS_QUERY  1
#define MRS_STA_EVT_STATUS_REPORT 2

#define MRS_STA_645_EVT_DI 0x04001501

mrs_sta_evt_ctx g_sta_evt_ctx;

mrs_sta_evt_ctx *mrs_sta_get_evt_ctx(td_void)
{
    return &g_sta_evt_ctx;
}

/* Event control context reset */
td_void mrs_sta_event_ctx_reset(td_void);

/* Meter event detected */
td_u32 mrs_sta_event_io_callback(td_u8 praram, td_bool value);

/* Event option selector */
td_u32 mrs_sta_event_chl_notify(td_bool chl_status, uintptr_t option, td_bool *one_shot);

/* Query meter events */
td_void mrs_sta_event_query_meter_event(td_void);

/* Recevie handler */
td_void mrs_sta_event_uart_rx(EXT_CONST mrs_mr_queue_item *item, td_pbyte data, td_u16 data_len);

td_void mrs_sta_event_uart_timeout(td_void);

/* Create event report on PLC */
td_u32 mrs_sta_create_event_report_plc(const td_pbyte buffer, td_u16 length);

/* Send event report on PLC */
td_u32 mrs_sta_send_event_report_plc(td_void);

/* Query EVENT notify */
td_void mrs_sta_event_query_notify(td_void);

/* PLC ACK notify */
td_void mrs_sta_event_plc_ack_notify(td_void);

/* EVENT trigger notify */
td_void mrs_sta_event_trigger_notify(td_void);

td_u32 mrs_sta_event_init(td_void)
{
    (td_void) memset_s(&g_sta_evt_ctx, sizeof(g_sta_evt_ctx), 0, sizeof(g_sta_evt_ctx));
    g_sta_evt_ctx.max_retry = MRS_STA_EVT_PLC_TIMES_MAX;
    g_sta_evt_ctx.seq = (td_u16)uapi_get_random_num32(1, EXT_U16_MAX);

    mrs_sta_io_register_evtout_callback(mrs_sta_event_io_callback, MRS_STA_EVT_FROM_METER);

    mrs_sta_event_register_query_notify(mrs_sta_event_query_meter_event);
    mrs_sta_event_register_plc_ack_notify(TD_NULL);
    mrs_sta_event_register_trigger_notify(TD_NULL);

    return EXT_ERR_SUCCESS;
}

td_void mrs_sta_event_ctx_reset(td_void)
{
    mrs_sta_evt_ctx *evt = mrs_sta_get_evt_ctx();

    mrs_timer_stop(MRS_TIMER_ID_STA_EVT_REPORT);

    mrs_free(evt->report_frame);
    evt->retry = 0;

    mrs_sta_clear_chl_notify(mrs_sta_event_chl_notify);
}

td_u32 mrs_sta_event_io_callback(td_u8 param, td_bool value)
{
    mrs_sta_evt_ctx *evt = mrs_sta_get_evt_ctx();

    ext_unref_param(param);

    evt->hard_evt_out = value;
    evt->evt_out = value;

    if (evt->evt_out == TD_TRUE) {
        mrs_sta_event_ctx_reset();
        mrs_sta_event_trigger_notify();

        /* channel available */
        if (mrs_sta_plc_chl_status() == TD_TRUE) {
            mrs_sta_event_query_notify();
        } else {
            mrs_sta_register_chl_notify(mrs_sta_event_chl_notify, MRS_STA_EVT_STATUS_QUERY);
        }
    }

    return EXT_ERR_SUCCESS;
}

td_void mrs_sta_event_query_meter_event(td_void)
{
    td_u8 query_frame[] = {
        0x68, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x68, 0x11, 0x04, 0x34, 0x48, 0x33, 0x37, 0xC7, 0x16
    };
    mrs_sta_srv_ctx *sta = mrs_sta_get_srv_ctx();
    td_u16 payload_len = sizeof(mrs_mr_queue_item) + sizeof(query_frame);
    td_pbyte payload;
    mrs_mr_queue_item *item = TD_NULL;
    mrs_sta_evt_ctx *evt = mrs_sta_get_evt_ctx();

    if (!mrs_invalid_meter(sta->meter)) {
        if (memcpy_s(query_frame + 1, MRS_METER_ADDR_LEN, sta->meter, MRS_METER_ADDR_LEN) != EOK) {
            return;
        }
        query_frame[sizeof(query_frame) - 2] = mrs_checksum8(query_frame, sizeof(query_frame) - 2); /* 2: check sum */
    }

    payload = (td_pbyte)mrs_malloc(payload_len);
    if (payload == TD_NULL) {
        return;
    }

    (td_void) memset_s(payload, payload_len, 0, payload_len);
    item = (mrs_mr_queue_item *)payload;
    item->id = MRS_STA_ITME_ID_EVENT_QRY;
    item->seq = ++evt->sta_queue_seq;
    item->timeout = MRS_CFG_METER_TIMEOUT;
    item->protocol = METER_PROTO_645_2007;
    item->data_len = sizeof(query_frame);
    item->rx_handle = mrs_sta_event_uart_rx;
    if (memcpy_s(item->data, item->data_len, query_frame, item->data_len) != EOK) {
        mrs_free(payload);
        return;
    }

    if (mrs_sta_try_enqueue(&sta->mr_queue, item) != TD_TRUE) {
        mrs_free(payload);
    }

    mrs_sta_queue_notify();
}

td_void mrs_sta_event_uart_rx(EXT_CONST mrs_mr_queue_item *item, td_pbyte data, td_u16 data_len)
{
    mrs_proto_645_frame_inf frame;
    td_u32 ret;
    td_u32 di;

    ext_unref_param(item);

    if (memset_s(&frame, sizeof(frame), 0, sizeof(frame)) != EOK) {
        return;
    }

    if (mrs_proto_645_decode_frame(data, data_len, &frame) != EXT_ERR_SUCCESS) {
        return;
    }

    if (frame.data_len <= MRS_645_DATA_DI_SIZE) {
        return;
    }

    if (memcpy_s(&di, sizeof(di), frame.data, MRS_645_DATA_DI_SIZE) != EOK) {
        return;
    }

    mrs_proto_645_data_decode((td_u8 *)&di, sizeof(di));
    if (di != MRS_STA_645_EVT_DI) {
        return;
    }

    ret = mrs_sta_create_event_report_plc(data, data_len);
    if (ret != EXT_ERR_SUCCESS) {
        return;
    }

    ret = mrs_sta_send_event_report_plc();
    if (ret != EXT_ERR_SUCCESS) {
        return;
    }
}

td_u32 mrs_sta_create_event_report_plc(const td_pbyte buffer, td_u16 length)
{
    mrs_sta_evt_ctx *evt = mrs_sta_get_evt_ctx();
    mrs_sta_srv_ctx *sta = mrs_sta_get_srv_ctx();
    mrs_plc_frame_data *plc = TD_NULL;
    mrs_meter_event_info *inf = TD_NULL;
    td_u16 payload_len = sizeof(mrs_plc_frame_data) + sizeof(mrs_meter_event_info) + length;
    td_pbyte payload;

    payload = mrs_malloc(payload_len);
    if (payload == TD_NULL) {
        return EXT_ERR_MALLOC_FAILUE;
    }

    (td_void) memset_s(payload, payload_len, 0, payload_len);

    plc = (mrs_plc_frame_data *)payload;
    plc->id = PLC_CMD_ID_EVT;
    plc->payload_len = sizeof(mrs_meter_event_info) + length;
    plc->payload = payload + sizeof(mrs_plc_frame_data);

    inf = (mrs_meter_event_info *)plc->payload;
    inf->stru_ver = MRS_PLC_PROTO_VERSION;
    inf->stru_size = sizeof(mrs_meter_event_info);
    inf->dir = MRS_PLC_UP_FLG;
    inf->prm = MRS_EVENT_PRM_HOST;
    inf->func_code = MRS_EVENT_FUNC_METER_REPORT;
    inf->data_len = length;
    inf->seq = ++evt->seq;
    if (memcpy_s(inf->meter, sizeof(inf->meter), sta->meter, sizeof(sta->meter)) != EOK) {
        mrs_free(payload);
        return EXT_ERR_FAILURE;
    }

    if (length > 0 && memcpy_s(inf->data, length, buffer, length) != EOK) {
        mrs_free(payload);
        return EXT_ERR_FAILURE;
    }

    evt->report_frame = plc;

    return EXT_ERR_SUCCESS;
}

td_u32 mrs_sta_send_event_report_plc(td_void)
{
    mrs_sta_evt_ctx *evt = mrs_sta_get_evt_ctx();
    td_u32 ret;

    if (evt->report_frame == TD_NULL) {
        return EXT_ERR_BAD_DATA;
    }

    do {
        if (mrs_sta_plc_chl_status() != TD_TRUE) {
            break;
        }

        if (evt->retry >= evt->max_retry) {
            evt->retry = 0;
            break;
        }

        ret = mrs_plc_frame_send(evt->report_frame);
        if (ret != EXT_ERR_SUCCESS) {
            return ret;
        }

        evt->retry++;
        mrs_timer_start(MRS_TIMER_ID_STA_EVT_REPORT,
                        uapi_min(MRS_STA_EVT_PLC_TIMEOUT * evt->retry, MRS_STA_EVT_PLC_TIMEOUT_MAX),
                        EXT_TIMER_TYPE_ONCE);

        return EXT_ERR_SUCCESS;
    } while (0);

    mrs_sta_register_chl_notify(mrs_sta_event_chl_notify, MRS_STA_EVT_STATUS_REPORT);
    return EXT_ERR_NOT_CONNECT;
}

td_u32 mrs_sta_event_chl_notify(td_bool chl_status, uintptr_t option, td_bool *one_shot)
{
    *one_shot = chl_status;
    if (chl_status == TD_FALSE) {
        return EXT_ERR_SUCCESS;
    }

    if (option == MRS_STA_EVT_STATUS_QUERY) {
        mrs_sta_event_query_meter_event();
    } else {
        mrs_sta_send_event_report_plc();
    }

    return EXT_ERR_SUCCESS;
}

td_void mrs_sta_event_report_plc_timeout_handle(td_void)
{
    td_u32 ret;
    ret = mrs_sta_send_event_report_plc();
    if (ret != EXT_ERR_SUCCESS) {
        return;
    }
}

td_void mrs_sta_evt_plc_frame_rx(const mrs_plc_frame_data *plc)
{
    mrs_meter_event_info *evt_frm = (mrs_meter_event_info *)plc->payload;
    mrs_sta_evt_ctx *evt = mrs_sta_get_evt_ctx();
    mrs_power_failure_ctrl_st *power_failure_info = mrs_get_power_failure_ctrl();
    mrs_power_failure_rcv_report *power_failure_rcv_info = mrs_get_power_failure_report_info();

    /* check frame */
    if ((evt_frm->stru_size < sizeof(mrs_meter_event_info)) ||
        (plc->payload_len != evt_frm->stru_size + evt_frm->data_len)) {
        return;
    }

    /* processing for power down event */
    if (evt_frm->func_code == MRS_EVENT_FUNC_MODUL_REPORT && evt_frm->dir == MRS_PLC_UP_FLG &&
        power_failure_info->power_failure_report_enable == MRS_STA_DETECT_POWER_FAILURE_ENABLE) {
        if (evt_frm->data_len < DATA_FIX_SIZE) {
            return;
        }

        if (plc->form_serial_number != uapi_mdm_get_form_serial_num() ||
            power_failure_info->start_send_time >= power_failure_info->send_times ||
            uapi_get_join_net_state() == TD_FALSE) {
            return;
        }

        mrs_power_failure_get_others_frame_data(evt_frm);

        if (power_failure_rcv_info->device_power_failure == TD_FALSE &&
            power_failure_info->is_ready_to_send_power_failure == TD_FALSE) {
            mrs_handle_others_power_failure_frame();
        }
        return;
    }

    /* download frame after cco reply ack: station processes recieved ack
        * CNComments:  CCO回复ack确认后,下行,站点处理接收到的ack */
    if (evt_frm->func_code == MRS_EVNET_FUNC_ACK && evt_frm->dir == MRS_PLC_DN_FLG &&
        power_failure_info->power_failure_report_enable == MRS_STA_DETECT_POWER_FAILURE_ENABLE) {
        if (mrs_sta_handle_power_failue_ack(evt_frm->seq) == EXT_ERR_SUCCESS) {
            return;
        }
    }

    if (evt_frm->seq != evt->seq || evt_frm->dir != MRS_PLC_DN_FLG) {
        return;
    }

    if (evt_frm->func_code == MRS_EVNET_FUNC_ACK) {
        mrs_sta_event_ctx_reset();
        mrs_sta_event_plc_ack_notify();
    } else if (evt_frm->func_code == MRS_EVENT_FUNC_CCO_BUF_FULL) {
        mrs_timer_start(MRS_TIMER_ID_STA_EVT_REPORT, MRS_STA_EVT_PLC_TIMEOUT_MAX, EXT_TIMER_TYPE_ONCE);
    }
}

/* Register query EVENT_OUT notify */
td_void mrs_sta_event_register_query_notify(mrs_sta_evt_callback notify)
{
    mrs_sta_evt_ctx *evt = mrs_sta_get_evt_ctx();
    evt->query_notify = notify;
}

/* Register PLC ACK notify */
td_void mrs_sta_event_register_plc_ack_notify(mrs_sta_evt_callback notify)
{
    mrs_sta_evt_ctx *evt = mrs_sta_get_evt_ctx();
    evt->plc_ack_notify = notify;
}

/* Register EVENT_OUT trigger notify */
td_void mrs_sta_event_register_trigger_notify(mrs_sta_evt_callback notify)
{
    mrs_sta_evt_ctx *evt = mrs_sta_get_evt_ctx();
    evt->trigger_notify = notify;
}

/* Query EVENT notify */
td_void mrs_sta_event_query_notify(td_void)
{
    mrs_sta_evt_ctx *evt = mrs_sta_get_evt_ctx();
    if (evt->query_notify) {
        evt->query_notify();
    }
}

/* PLC ACK notify */
td_void mrs_sta_event_plc_ack_notify(td_void)
{
    mrs_sta_evt_ctx *evt = mrs_sta_get_evt_ctx();
    if (evt->plc_ack_notify) {
        evt->plc_ack_notify();
    }
}

/* EVENT trigger notify */
td_void mrs_sta_event_trigger_notify(td_void)
{
    mrs_sta_evt_ctx *evt = mrs_sta_get_evt_ctx();
    if (evt->trigger_notify) {
        evt->trigger_notify();
    }
}

#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_STA) */


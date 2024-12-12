/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS CCO EVENT report.
 */

#include "mrs_cco_evt.h"
#include "mrs_cco_srv.h"
#include "mrs_cco_queue.h"
#include "mrs_common_tools.h"
#include "mrs_common_power_failure.h"
#include "mrs_cco_power_failure.h"
#include "mrs_cco_archives.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
/* Events filters */
td_bool mrs_cco_evt_allow_enqueue(const td_u8 *meter, td_u16 seq);
td_bool mrs_cco_evt_filter_reported(const td_u8 *meter, td_u16 seq);
td_bool mrs_cco_evt_filter_repeat(const td_u8 *meter, td_u16 seq);
/* Send ack PLC frame */
td_void mrs_cco_evt_plc_ack(const td_u8 *meter, td_u16 seq, td_u8 func_code, const td_u8 *mac);
/* Remove aged events */
td_void mrs_cco_evt_reported_aged(mrs_cco_evt_reported_item *item);
/* Create 376.2 event report frame */
td_u32 mrs_cco_evt_create_1376_2_frame(mrs_meter_event_info *frame, td_pbyte *buffer, td_u16 *length);
/* Remove events from event queue */
td_void mrs_cco_evt_report_end(td_pvoid param);
/* Event report timeout */
td_u32 mrs_cco_evt_report_timeout(td_pvoid param);
/* Add event to reported list */
td_void mrs_cco_evt_reported_add_list(td_void);
/* Replace events when list is full */
td_void mrs_cco_evt_replace_reported_item(mrs_cco_evt_reported_item *report_item, const mrs_cco_evt_item *item);

mrs_cco_evt_ctx g_cco_evt_ctx;
mrs_cco_evt_ctx *mrs_cco_get_evt_ctx(td_void)
{
    return &g_cco_evt_ctx;
}

td_void mrs_cco_event_init(td_void)
{
    (td_void) memset_s(&g_cco_evt_ctx, sizeof(g_cco_evt_ctx), 0, sizeof(g_cco_evt_ctx));
    mrs_queue_init(&g_cco_evt_ctx.evt_q);
    uapi_list_init(&g_cco_evt_ctx.evt_reported_list);
}

td_void mrs_cco_rx_evt_plc_power_down_proc(const mrs_plc_frame_data *plc)
{
    mrs_meter_event_info *evt_frm = (mrs_meter_event_info *)plc->payload;
    /* power down */
    mrs_power_failure_ctrl_st *power_failure_info = mrs_get_power_failure_ctrl();
    td_u8 mac[EXT_PLC_MAC_ADDR_LEN] = { 0 };
    if (memcpy_s(mac, EXT_PLC_MAC_ADDR_LEN, plc->addr, EXT_PLC_MAC_ADDR_LEN) != EXT_ERR_SUCCESS) {
        return;
    }

    /* check validity of data */
    if ((evt_frm->stru_size + evt_frm->data_len != plc->payload_len) || (evt_frm->data_len <= DATA_FIX_SIZE)) {
        return;
    }

    if (plc->form_serial_number != uapi_mdm_get_form_serial_num()) {
        return;
    }
    mrs_power_failure_get_others_frame_data(evt_frm);

    /* if need send reply frame */
    if (mrs_cco_not_reply_ack(mac) != TD_TRUE) {
        /* CCO create reply frame */
        mrs_cco_evt_plc_ack(evt_frm->meter, evt_frm->seq, MRS_EVNET_FUNC_ACK, plc->addr);
    }

    /* report event to concentrator */
    if (power_failure_info->is_ready_to_send_power_failure == TD_FALSE &&
        evt_frm->data[0] == POWER_FAILURE) {
        mrs_printf("ENTER mrs_cco_evt_report().\n");
        mrs_cco_evt_report();
    }

    return;
}
td_u32 mrs_cco_rx_evt_plc_check(const mrs_plc_frame_data *plc)
{
    mrs_meter_event_info *evt_frm = (mrs_meter_event_info *)plc->payload;
    td_u32 ret = EXT_ERR_FAILURE;
    /* check frame */
    if ((evt_frm->dir != MRS_PLC_UP_FLG) || (evt_frm->stru_size < sizeof(mrs_meter_event_info)) ||
        (plc->payload_len != evt_frm->stru_size + evt_frm->data_len)) {
        return ret;
    }

    if (evt_frm->func_code != MRS_EVENT_FUNC_METER_REPORT) {
        return ret; /* Not support */
    }
    return EXT_ERR_SUCCESS;
}

td_u32 mrs_cco_rx_evt_plc_fill_ack_data(const mrs_plc_frame_data *plc, td_pbyte payload, td_u16 payload_len,
                                        const td_pbyte buffer, mrs_cco_evt_item **item)
{
    mrs_meter_event_info *evt_frm = (mrs_meter_event_info *)plc->payload;
    td_u16 length = payload_len - sizeof(mrs_cco_evt_item);
    mrs_cco_evt_item *evt_item = TD_NULL;
    if (payload == TD_NULL) {
        return EXT_ERR_FAILURE;
    }

    (td_void) memset_s(payload, payload_len, 0, payload_len);
    evt_item = (mrs_cco_evt_item *)payload;
    (td_void) memcpy_s(evt_item->meter, sizeof(evt_item->meter), evt_frm->meter, sizeof(evt_frm->meter));
    evt_item->seq = evt_frm->seq;
    evt_item->data_len = length;
    evt_item->fn = 255; /* fn 255 */
    if (memcpy_s(evt_item->data, evt_item->data_len, buffer, length) != EOK) {
        return EXT_ERR_FAILURE;
    }

    *item = evt_item;
    return EXT_ERR_SUCCESS;
}

td_void mrs_cco_evt_plc_frame_rx(mrs_plc_frame_data *plc)
{
    mrs_meter_event_info *evt_frm = (mrs_meter_event_info *)plc->payload;
    mrs_cco_evt_ctx *evt = mrs_cco_get_evt_ctx();
    td_u8 func_code = MRS_EVNET_FUNC_ACK;
    /* power down */
    mrs_power_failure_ctrl_st *power_failure_info = mrs_get_power_failure_ctrl();

    /* power down processing */
    if (evt_frm->func_code == MRS_EVENT_FUNC_MODUL_REPORT && evt_frm->dir == MRS_PLC_UP_FLG &&
        power_failure_info->power_failure_report_enable == MRS_CCO_DETECT_POWER_FAILURE_ENABLE) {
        mrs_cco_rx_evt_plc_power_down_proc(plc);
        return;
    } else if (mrs_cco_rx_evt_plc_check(plc) != EXT_ERR_SUCCESS) {
        return;
    }

    do {
        td_pbyte buffer = TD_NULL;
        td_u16 length = 0;
        td_u16 payload_len;
        td_pbyte payload = TD_NULL;
        mrs_cco_evt_item *item = TD_NULL;
        td_u32 ret;

        /* already in queue. send reply frame directly */
        if (mrs_cco_evt_allow_enqueue(evt_frm->meter, evt_frm->seq) != TD_TRUE) {
            break;
        }

        if (evt->evt_q_num >= MRS_CCO_EVT_ITEM_MAX) {
            func_code = MRS_EVENT_FUNC_CCO_BUF_FULL;
            break;
        }

        ret = mrs_cco_evt_create_1376_2_frame(evt_frm, &buffer, &length);
        if (ret != EXT_ERR_SUCCESS) {
            func_code = MRS_EVENT_FUNC_CCO_BUF_FULL;
            break;
        }

        payload_len = sizeof(mrs_cco_evt_item) + length;
        payload = (td_pbyte)mrs_malloc(payload_len);
        ret = mrs_cco_rx_evt_plc_fill_ack_data(plc, payload, payload_len, buffer, &item);
        if (ret != EXT_ERR_SUCCESS) {
            mrs_free(payload);
            mrs_free(buffer);
            return;
        }

        mrs_queue_enqueue(&evt->evt_q, &item->link);
        evt->evt_q_num++;
        evt->acd_flg = TD_TRUE;
        mrs_cco_evt_report_active();

        mrs_free(buffer);
    } while (0);

    mrs_cco_evt_plc_ack(evt_frm->meter, evt_frm->seq, func_code, plc->addr);
}

td_bool mrs_cco_evt_allow_enqueue(const td_u8 *meter, td_u16 seq)
{
    /* if already in reported queue, not allow join the queue */
    if (mrs_cco_evt_filter_reported(meter, seq) == TD_TRUE) {
        return TD_FALSE;
    }

    /* if already in queue of waiting to be reported, not allow join the queue */
    if (mrs_cco_evt_filter_repeat(meter, seq) == TD_TRUE) {
        return TD_FALSE;
    }

    return TD_TRUE;
}

td_bool mrs_cco_evt_filter_reported(const td_u8 *meter, td_u16 seq)
{
    mrs_cco_evt_ctx *evt = mrs_cco_get_evt_ctx();
    ext_list *list = &evt->evt_reported_list;
    ext_list *it = list->next;

    /* find the same meter, and age the reported list. */
    while (it != list) {
        mrs_cco_evt_reported_item *item = (mrs_cco_evt_reported_item *)it;
        it = it->next;

        if ((memcmp(meter, item->meter, sizeof(item->meter)) == 0) && (seq == item->seq)) {
            mrs_cco_evt_reported_aged(item);
            return TD_TRUE;
        }

        mrs_cco_evt_reported_aged(item);
    }

    return TD_FALSE;
}

td_void mrs_cco_evt_reported_aged(mrs_cco_evt_reported_item *item)
{
    mrs_cco_evt_ctx *evt = mrs_cco_get_evt_ctx();
    td_u32 cur_time = uapi_get_seconds();
    if (cur_time >= item->time + MRS_CCO_EVT_REPORTED_AGED) {
        uapi_list_delete(&item->link);
        mrs_free(item);
        evt->report_list_num--;
    }
}

td_bool mrs_cco_evt_filter_repeat(const td_u8 *meter, td_u16 seq)
{
    mrs_cco_evt_ctx *evt = mrs_cco_get_evt_ctx();
    mrs_srv_queue *q = (mrs_srv_queue *)&evt->evt_q;
    mrs_queue_node *it;

    for (it = q->head; it != TD_NULL; it = it->next) {
        mrs_cco_evt_item *item = (mrs_cco_evt_item *)it;
        if ((memcmp(meter, item->meter, sizeof(item->meter)) == 0) && (seq == item->seq)) {
            return TD_TRUE;
        }
    }

    return TD_FALSE;
}

td_void mrs_cco_evt_plc_ack(const td_u8 *meter, td_u16 seq, td_u8 func_code, const td_u8 *mac)
{
    mrs_plc_frame_data plc;
    mrs_meter_event_info evt_frm;

    (td_void) memset_s(&evt_frm, sizeof(evt_frm), 0, sizeof(evt_frm));
    evt_frm.stru_ver = MRS_PLC_PROTO_VERSION;
    evt_frm.stru_size = sizeof(evt_frm);
    evt_frm.dir = MRS_PLC_DN_FLG;
    evt_frm.prm = MRS_EVENT_PRM_SLAVE;
    evt_frm.func_code = func_code;
    evt_frm.seq = seq;
    evt_frm.data_len = 0;

    if (memcpy_s(evt_frm.meter, sizeof(evt_frm.meter), meter, MRS_METER_ADDR_LEN) != 0) {
        return;
    }

    (td_void) memset_s(&plc, sizeof(plc), 0, sizeof(plc));
    plc.id = PLC_CMD_ID_EVT;
    plc.payload_len = sizeof(evt_frm);
    plc.payload = (td_pbyte)&evt_frm;
    if (memcpy_s(plc.addr, sizeof(plc.addr), mac, EXT_PLC_MAC_ADDR_LEN) != EOK) {
        return;
    }

    mrs_plc_frame_send(&plc);
}

td_u32 mrs_cco_evt_create_1376_2_frame(mrs_meter_event_info *frame, td_pbyte *buffer, td_u16 *length)
{
    /* device type 1B + protocol type 1B + frame length 2B + frame->data_len */
    td_u16 payload_len = (td_u8)frame->data_len + 4; /* 4: 1+1+2 */
    td_pbyte payload = (td_pbyte)mrs_malloc(payload_len);
    mrs_proto_1376_2_encode encode;
    mrs_cco_srv_ctx *cco = mrs_cco_get_srv_ctx();
    td_u32 ret;
    td_u32 data_len;

    if (payload == TD_NULL) {
        return EXT_ERR_MALLOC_FAILUE;
    }

    (td_void) memset_s(&encode, sizeof(encode), 0, sizeof(encode));
    encode.data = payload;
    encode.length = payload_len;
    encode.prm = 1;
    encode.seq = ++cco->seq;
    encode.afn = 6;  /* afn 6 */
    encode.fn = 255; /* fn 255 */

    *payload++ = 0x01; /* electric meter */
    *payload++ = mrs_archives_get_meter_protocol(frame->meter);
    *payload++ = (td_u8)(frame->data_len & 0xFF);
    *payload++ = (td_u8)((frame->data_len >> 8) & 0xFF); /* low 8bits */
    data_len = payload_len - 4;                          /* data length: 4B + data length */
    if (data_len > 0) {
        if (memcpy_s(payload, data_len, (td_pbyte)frame + frame->stru_size, (td_u8)frame->data_len) != EOK) {
            mrs_free(encode.data);
            return EXT_ERR_MEMCPY_FAIL;
        }
    }

    ret = mrs_proto_1376_2_create_frame(&encode, buffer, length);
    mrs_free(encode.data);
    return ret;
}

td_void mrs_msg_on_evt_notify(EXT_CONST mrs_queue_msg *msg)
{
    mrs_cco_evt_ctx *evt = mrs_cco_get_evt_ctx();
    mrs_cco_evt_item *item = TD_NULL;
    mrs_cco_queue_item *cco_item = TD_NULL;

    ext_unref_param(msg);

    if (mrs_try_lock(evt->evt_q_lock) == 0) {
        return;
    }

    item = (mrs_cco_evt_item *)mrs_queue_top(&evt->evt_q);
    if (item == TD_NULL) {
        mrs_unlock(evt->evt_q_lock);
        return;
    }

    cco_item = (mrs_cco_queue_item *)mrs_malloc(sizeof(mrs_cco_queue_item) + item->data_len);
    if (cco_item == TD_NULL) {
        mrs_cco_evt_report_end(TD_NULL);
        return;
    }

    (td_void) memset_s(cco_item, sizeof(mrs_cco_queue_item) + item->data_len, 0,
                       sizeof(mrs_cco_queue_item) + item->data_len);
    cco_item->valid = TD_TRUE;
    cco_item->afn = 0x06;
    cco_item->fn = item->fn;
    cco_item->retry_max = MRS_CCO_EVT_RETRY_MAX;
    cco_item->timeout = MRS_CCO_EVT_REPORT_TIMEOUT;
    cco_item->data_len = item->data_len;

    if (memcpy_s(cco_item->data, cco_item->data_len, item->data, item->data_len) != EOK) {}

    cco_item->rx_handle = mrs_cco_evt_report_end;
    cco_item->total_timeout = mrs_cco_evt_report_timeout;
    mrs_cco_queue_enqueue(cco_item);
    mrs_cco_queue_active();
}

td_void mrs_cco_evt_report_end(td_pvoid param)
{
    mrs_cco_evt_ctx *evt = mrs_cco_get_evt_ctx();
    mrs_cco_evt_item *item = TD_NULL;

    ext_unref_param(param);

    mrs_cco_evt_reported_add_list();

    item = (mrs_cco_evt_item *)mrs_queue_dequeue(&evt->evt_q);
    mrs_free(item);

    mrs_unlock(evt->evt_q_lock);
    evt->evt_q_num--;

    if (evt->evt_q_num == 0) {
        evt->acd_flg = TD_FALSE;
        return;
    }
    mrs_cco_evt_report_active();
}

td_u32 mrs_cco_evt_report_timeout(td_pvoid param)
{
    mrs_cco_evt_report_end(param);
    return EXT_ERR_SUCCESS;
}

td_void mrs_cco_evt_reported_add_list(td_void)
{
    mrs_cco_evt_ctx *evt = mrs_cco_get_evt_ctx();
    mrs_cco_evt_item *item = TD_NULL;
    ext_list *list = TD_NULL;
    ext_list *it = TD_NULL;
    ext_list *next = TD_NULL;
    mrs_cco_evt_reported_item *temp = TD_NULL;

    item = (mrs_cco_evt_item *)mrs_queue_top(&evt->evt_q);
    if (item == TD_NULL) {
        mrs_unlock(evt->evt_q_lock);
        return;
    }

    list = &evt->evt_reported_list;
    it = list->next;
    while (it != list) {
        temp = (mrs_cco_evt_reported_item *)it;
        next = it->next;

        if (memcmp(temp->meter, item->meter, MRS_METER_ADDR_LEN) == 0) {
            mrs_cco_evt_replace_reported_item(temp, item);
            return;
        }

        it = next;
    }

    if (evt->report_list_num >= MRS_CCO_EVT_REPORT_ITEM_MAX) {
        temp = (mrs_cco_evt_reported_item *)it->next;
        mrs_cco_evt_replace_reported_item(temp, item);
        return;
    }

    temp = (mrs_cco_evt_reported_item *)mrs_malloc(sizeof(mrs_cco_evt_reported_item));
    if (temp == TD_NULL) {
        return;
    }

    (td_void) memcpy_s(temp->meter, sizeof(temp->meter), item->meter, sizeof(item->meter));
    temp->seq = item->seq;
    temp->time = uapi_get_seconds();
    uapi_list_tail_insert(list, &temp->link);
    evt->report_list_num++;
}

td_void mrs_cco_evt_replace_reported_item(mrs_cco_evt_reported_item *report_item, const mrs_cco_evt_item *item)
{
    mrs_cco_evt_ctx *evt = mrs_cco_get_evt_ctx();

    report_item->seq = item->seq;
    report_item->time = uapi_get_seconds();
    uapi_list_delete(&report_item->link);
    uapi_list_tail_insert(&evt->evt_reported_list, &report_item->link);
}

#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_CCO) */


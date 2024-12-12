/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS STA POWER FAILURE report.
 */

#include "mrs_common_power_failure.h"
#include "mrs_common_tools.h"
#include "mrs_sta_io.h"
#include "mrs_sta_power_failure.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)

/************************************************************
                       global variables
************************************************************/
mrs_power_failure_rcv_report g_rcv_report_info = { 0 };

mrs_power_failure_rcv_report *mrs_get_power_failure_report_info(td_void)
{
    return &g_rcv_report_info;
}

/* Handling Local Power Outage Events */
td_u32 mrs_sta_handle_local_power_failure(td_void)
{
    td_u32 ret;
    td_u32 random_wait_send_time;
    mrs_power_failure_ctrl_st *power_failure_info = mrs_get_power_failure_ctrl();
    mrs_power_failure_rcv_report *power_failure_rcv_info = mrs_get_power_failure_report_info();

    power_failure_rcv_info->my_tei = uapi_get_my_tei();
    power_failure_rcv_info->device_power_failure = TD_TRUE;
    power_failure_rcv_info->device_pull_reset = TD_TRUE;
    if (power_failure_info->power_failure_bitmap == TD_NULL) {
        return EXT_ERR_FAILURE;
    }
    /* The power failure tei information is updated to the global bitmap. */
    power_failure_info->power_failure_bitmap[power_failure_rcv_info->my_tei / BIT_SIZE] |= 1 <<
            (power_failure_rcv_info->my_tei % BIT_SIZE);
    random_wait_send_time = uapi_get_random_num32(power_failure_info->random_min_value,
        power_failure_info->random_max_value);

    ret = mrs_timer_start(MRS_TIMER_ID_STA_POWER_FAILURE, random_wait_send_time, EXT_TIMER_TYPE_ONCE);

    return ret;
}

td_void mrs_sta_create_power_failure_frame_proc(td_void)
{
    mrs_power_failure_ctrl_st *power_failure_info = mrs_get_power_failure_ctrl();

    if (power_failure_info->power_failure_bitmap == TD_NULL) {
        return;
    }

    if (mrs_data_is_all_zero(power_failure_info->power_failure_bitmap, POWER_FAILURE_BITMAP_LEN, 0) == TD_FALSE) {
        mrs_sta_create_power_failure_frame(POWER_FAILURE);
    }
}

td_u32 mrs_sta_set_power_failure_info(td_pbyte payload, td_u16 bitmap_len, EXT_CONST td_u8 *tei_point, td_u8 event_type)
{
    mrs_meter_event_info *event_info = TD_NULL;
    mrs_sta_evt_ctx *evt_ctx = mrs_sta_get_evt_ctx();
    mrs_sta_srv_ctx *sta_srv_ctx = mrs_sta_get_srv_ctx();
    mrs_power_failure_ctrl_st *power_failure_info = mrs_get_power_failure_ctrl();

    if (evt_ctx == TD_NULL) {
        return EXT_ERR_NO_INITILIZATION;
    }

    event_info = (mrs_meter_event_info *)payload;
    event_info->stru_ver = MRS_PLC_PROTO_VERSION;
    event_info->stru_size = sizeof(mrs_meter_event_info);
    event_info->dir = MRS_PLC_UP_FLG;
    event_info->prm = MRS_EVENT_PRM_HOST;
    event_info->func_code = MRS_EVENT_FUNC_MODUL_REPORT;
    event_info->data_len = DATA_FIX_SIZE + bitmap_len;
    event_info->seq = ++evt_ctx->seq;
    if (memcpy_s(event_info->meter, sizeof(event_info->meter), sta_srv_ctx->meter,
                 sizeof(sta_srv_ctx->meter)) != EXT_ERR_SUCCESS) {
        return EXT_ERR_FAILURE;
    }

    power_failure_info->ack_seq = event_info->seq;

    /* The STA proactively reports events (triggered by modules) and data is expanded. */
    event_info->data[0] = event_type;

    if (memcpy_s(&event_info->data[DATA_START_TEI_POSITION], sizeof(td_u8), tei_point,
                 sizeof(td_u8)) != EXT_ERR_SUCCESS) {
        return EXT_ERR_FAILURE;
    }

    if (memcpy_s(&event_info->data[DATA_END_TEI_POSITION], sizeof(td_u8), tei_point + 1,
                 sizeof(td_u8)) != EXT_ERR_SUCCESS) {
        return EXT_ERR_FAILURE;
    }

    mrs_sta_update_normal_bitmap(power_failure_info->power_failure_bitmap, &event_info->data[DATA_FIX_SIZE],
                                 bitmap_len);

    mrs_printf("[CREATE_PF_FRAME]: data_len= %d\n", event_info->data_len);
    mrs_logbuf("[CREATE_PF_FRAME]: ", event_info->data, event_info->data_len);

    return EXT_ERR_SUCCESS;
}

td_u32 mrs_sta_create_power_failure_frame(td_u8 event_type)
{
    mrs_plc_frame_data power_frame = { 0 };
    td_u16 payload_len;
    td_pbyte payload;
    td_u16 bitmap_len;
    td_u16 bitmap_max_tei;
    td_u16 bitmap_min_tei;
    td_u8 *tei_point = TD_NULL;
    mrs_power_failure_ctrl_st *power_failure_info = mrs_get_power_failure_ctrl();

    mrs_printf("ENTER mrs_sta_create_power_failure_frame().\n");

    if (power_failure_info->power_failure_bitmap == TD_NULL) {
        return EXT_ERR_FAILURE;
    }

    bitmap_min_tei = mrs_calc_min_tei(power_failure_info->power_failure_bitmap);
    bitmap_max_tei = mrs_calc_max_tei(power_failure_info->power_failure_bitmap);

    bitmap_len = (bitmap_max_tei - bitmap_min_tei + BIT_SIZE) / BIT_SIZE;

    tei_point = (td_u8 *)(&bitmap_min_tei);

    payload_len = sizeof(mrs_meter_event_info) + DATA_FIX_SIZE + bitmap_len;
    payload = mrs_malloc(payload_len);
    if (payload == TD_NULL) {
        return EXT_ERR_MALLOC_FAILUE;
    }

    if (memset_s(payload, payload_len, 0, payload_len) != EXT_ERR_SUCCESS) {
        mrs_free(payload);
        return EXT_ERR_FAILURE;
    }

    if (mrs_sta_set_power_failure_info(payload, bitmap_len, tei_point, event_type) != EXT_ERR_SUCCESS) {
        mrs_free(payload);
        return EXT_ERR_FAILURE;
    }

    power_frame.id = PLC_CMD_ID_EVT;
    power_frame.payload_len = payload_len;
    power_frame.payload = payload;

    mrs_sta_power_failure_set_send_info(event_type, &power_frame);
    mrs_free(payload);

    return EXT_ERR_SUCCESS;
}

td_void mrs_sta_power_failure_set_send_info(td_u8 event_type, mrs_plc_frame_data *power_frame)
{
    td_u32 index = 0;
    mrs_power_failure_ctrl_st *power_failure_info = mrs_get_power_failure_ctrl();
    mrs_power_failure_rcv_report *power_failure_rcv_info = mrs_get_power_failure_report_info();

    /* The module is not powered off and is transmitted in unicast mode. */
    if (power_failure_rcv_info->device_power_failure == TD_FALSE) {
        mrs_plc_frame_send(power_frame);
        mrs_timer_start(MRS_TIMER_ID_STA_POWER_FAILURE_ACK, power_failure_info->send_interval, EXT_TIMER_TYPE_ONCE);
        power_failure_info->wait_ack_flag = TD_TRUE;
        power_failure_info->wait_ack_type = event_type;

        return;
    }

    /* The module is powered off and broadcast (local broadcast or network-wide broadcast). */
    power_frame->addr[index] = 0;
    if (memset_s(power_frame->addr + 1, EXT_PLC_MAC_ADDR_LEN - 1, 0xFF, EXT_PLC_MAC_ADDR_LEN - 1) != EXT_ERR_SUCCESS) {
        return;
    }

    if (mrs_sta_send_power_failure_frame(power_frame) != EXT_ERR_SUCCESS) {
        return;
    }
}

/* The timer for waiting for the ACK message expires. */
td_void mrs_sta_handle_wait_power_failure_ack(td_void)
{
    mrs_power_failure_ctrl_st *power_failure_info = mrs_get_power_failure_ctrl();
    mrs_power_failure_rcv_report *power_failure_rcv_info = mrs_get_power_failure_report_info();

    power_failure_info->send_report_times++;

    /* The module is powered off. The ack is not processed. */
    if (power_failure_rcv_info->device_power_failure == TD_TRUE) {
        power_failure_info->wait_ack_flag = TD_FALSE;
        return;
    }

    if (power_failure_info->wait_ack_flag == TD_FALSE
        || power_failure_info->send_report_times == (power_failure_info->send_times + STA_DEFAULT_SEND_CNT)) {
        /* Clear some power outage information. */
        mrs_sta_power_failure_info_erase(power_failure_info);
        return;
    }

    /* Failed to wait for the ACK response and resend the message. */
    if (mrs_sta_create_power_failure_frame(power_failure_info->wait_ack_type) != EXT_ERR_SUCCESS) {
        return;
    }
}

/* A power outage site sends a power outage report packet. */
td_u32 mrs_sta_send_power_failure_frame(EXT_CONST mrs_plc_frame_data *power_frame)
{
    td_u32 ret;
    mrs_power_failure_ctrl_st *power_failure_info = mrs_get_power_failure_ctrl();

    if (power_frame == TD_NULL) {
        return EXT_ERR_BAD_DATA;
    }

    ret = mrs_plc_frame_send(power_frame);

    /* Wait interval after sending success (starting from 1) */
    power_failure_info->start_send_time++;

    if (power_failure_info->start_send_time <= power_failure_info->send_times) {
        mrs_timer_start(MRS_TIMER_ID_STA_POWER_FAILURE, power_failure_info->send_interval, EXT_TIMER_TYPE_ONCE);
    } else {
        /* Clear some power outage information. */
        mrs_sta_power_failure_info_erase(power_failure_info);
    }

    return ret;
}

td_void mrs_sta_power_failure_info_erase(mrs_power_failure_ctrl_st *power_failure_info)
{
    mrs_power_failure_ctrl_st *power_failure_ctrl = power_failure_info;

    power_failure_ctrl->ack_seq = 0;
    power_failure_ctrl->wait_ack_flag = TD_FALSE;
    power_failure_ctrl->ack_seq = 0;
    power_failure_ctrl->is_ready_to_send_power_failure = TD_FALSE;
    power_failure_ctrl->send_report_times = 0;

    if (memset_s(power_failure_ctrl->power_failure_bitmap, POWER_FAILURE_BITMAP_LEN, 0,
                 POWER_FAILURE_BITMAP_LEN) != EXT_ERR_SUCCESS) {
        return;
    }
}

/* Processes power outage reporting frames of other modules. */
td_u32 mrs_handle_others_power_failure_frame(td_void)
{
    td_u32 ret;
    mrs_power_failure_ctrl_st *power_failure_info = mrs_get_power_failure_ctrl();

    power_failure_info->is_ready_to_send_power_failure = TD_TRUE;
    power_failure_info->start_send_time = 1;

    /* Set the timer to wait for the 30s aggregation packet. After the timer expires, send the stop frame. */
    ret = mrs_timer_start(MRS_TIMER_ID_STA_POWER_FAILURE, power_failure_info->converge_time * MRS_US_TO_S_MASK,
                          EXT_TIMER_TYPE_ONCE);
    if (ret != EXT_ERR_SUCCESS) {
        return ret;
    }

    return ret;
}

td_u32 mrs_sta_handle_power_failue_ack(td_u16 seq)
{
    td_u32 ret = EXT_ERR_FAILURE;
    mrs_power_failure_ctrl_st *power_failure_info = mrs_get_power_failure_ctrl();

    if (power_failure_info->wait_ack_flag == TD_FALSE) {
        return EXT_ERR_SKIP;
    }

    /* ACK clearance flag (through the sequence number) */
    if (power_failure_info->ack_seq == seq) {
        power_failure_info->wait_ack_flag = TD_FALSE;
        power_failure_info->send_report_times = 0;
        return EXT_ERR_SUCCESS;
    }

    return ret;
}

/* Convert to standard bitmap_out */
td_void mrs_sta_update_normal_bitmap(td_u8 *bitmap_in, td_u8 *bitmap_out, td_u16 bitmap_len)
{
    td_u16 i;
    td_u16 j;
    td_u16 bitmap_max_tei;
    td_u16 bitmap_min_tei;

    bitmap_min_tei = mrs_calc_min_tei(bitmap_in);
    bitmap_max_tei = mrs_calc_max_tei(bitmap_in);

    for (i = bitmap_min_tei, j = 0; i <= bitmap_max_tei && j < bitmap_len * BIT_SIZE; i++, j++) {
        /* Determine which tei of the minimum tei is powered off, and position 1 in the new buf. */
        if (mrs_power_failure_bit_check(bitmap_in, i) == TD_TRUE) {
            mrs_power_failure_bit_set(bitmap_out, j);
        }
    }
}

td_void mrs_sta_reset_handle(td_void)
{
    /* Start the power outage detection. */
    uapi_start_power_failure_check();
    mrs_timer_start(MRS_TIMET_ID_PULL_RST_DELAY_TIMER, 300, EXT_TIMER_TYPE_ONCE); /* timeout: 300 */
}

td_void mrs_power_failure_pull_rst_handle(td_void)
{
    if (uapi_get_power_failure() == TD_TRUE) {
        return;
    }

    uapi_usr_reboot(EXT_SYS_REBOOT_CAUSE_USR0);
}

#if 1

power_failure_ctrl_st g_pwf_ctrl = { 0 };

td_void mrs_pwf_init(td_void)
{
    /* Start the periodic power outage detection timer. */
    mrs_timer_start(MRS_TIMER_ID_STA_PW_CHECK, 1000, EXT_TIMER_TYPE_PERIOD); /* timeout: 1000 */

    /* Start the power outage detection. */
    uapi_start_power_failure_check();
}

td_void mrs_pwf_check_main(td_void)
{
    td_bool is_power_failure;
    td_u8 next_pwf_state = 0;
    power_failure_ctrl_st *pwf_ctrl = &g_pwf_ctrl;

    if (pwf_ctrl->pwf_switch == TD_FALSE) {
        return;
    }

    /* This check is not completed. */
    if (uapi_is_check_finish() == TD_FALSE) {
        return;
    }

    /* Obtain the result of the last power outage detection. */
    is_power_failure = uapi_get_power_failure();

    /* Start a new power outage detection. */
    uapi_start_power_failure_check();

    switch (pwf_ctrl->pwf_state) {
        /* Self-check Status */
        case PWF_SELF_CHECK_STATE:
            next_pwf_state = mrs_pwf_self_check_state(is_power_failure);
            break;

        /* Power-on status */
        case PWF_POWER_UP_STATE:
            next_pwf_state = mrs_pwf_power_on_state(is_power_failure);
            break;

        /* Power-off status */
        case PWF_POWER_FAILURE_STATE:
            next_pwf_state = mrs_pwf_power_failure_state(is_power_failure);
            break;
        default:
            break;
    }

    pwf_ctrl->pwf_state = next_pwf_state;
}

td_u8 mrs_pwf_self_check_state(td_bool is_power_failure)
{
    /* The device is powered on and enters the power-on state. */
    if (is_power_failure == TD_FALSE) {
        return PWF_POWER_UP_STATE;
    }
    return PWF_SELF_CHECK_STATE;
}

td_u8 mrs_pwf_power_on_state(td_bool is_power_failure)
{
    mrs_power_failure_ctrl_st *power_failure_info = mrs_get_power_failure_ctrl();

    if (is_power_failure == TD_FALSE) {
        power_failure_info->start_send_time = 0;
        if (memset_s(&g_rcv_report_info, sizeof(mrs_power_failure_rcv_report), 0,
                     sizeof(mrs_power_failure_rcv_report)) != EXT_ERR_SUCCESS) {
            return PWF_SELF_CHECK_STATE;
        }
        return PWF_POWER_UP_STATE;
    }

    /* Check whether the module is removed and inserted. */
    if (uapi_get_power_failure_is_plug_module() == EXT_ERR_SUCCESS && uapi_get_join_net_state() == TD_TRUE) {
        /* The module is in the meter and enters the power-off state. The power failure is reported. */
        g_pwf_ctrl.pwf_power_failure_time = uapi_get_seconds();
        uapi_set_power_failure_state(TD_TRUE);

        mrs_sta_handle_local_power_failure();

        return PWF_POWER_FAILURE_STATE;
    } else {
        /* The module is removed and the status is switched to the self-check state. */
        return PWF_SELF_CHECK_STATE;
    }
}

td_u8 mrs_pwf_power_failure_state(td_bool is_power_failure)
{
    if (is_power_failure == TD_TRUE) {
        /* When the power failure time exceeds a certain threshold, the service function is restored and the self-check
           status is switched. */
        if (uapi_get_seconds() - g_pwf_ctrl.pwf_power_failure_time > g_pwf_ctrl.power_failure_recover_thd) {
            uapi_set_power_failure_state(TD_FALSE);
            return PWF_SELF_CHECK_STATE;
        }

        return PWF_POWER_FAILURE_STATE;
    }

    /* After the power-on, services are restored, and the self-check status is switched. */
    uapi_set_power_failure_state(TD_FALSE);

    return PWF_SELF_CHECK_STATE;
}
#endif

#endif

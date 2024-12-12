/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS CAPTURE FUN handle.(Development of Tested Module Based on 1901.1 Test System)
 */

#include "mrs_common_capture.h"
#include "mrs_common_tools.h"
#include "mrs_common_uart.h"
#include "dfx_app.h"

td_bool g_test_mode_enable;
ext_uart_port g_mrs_default_port = EXT_UART_0;
ext_uart_port g_mrs_diag_port = EXT_UART_1;
diag_cmd_test_mode_info_ind g_dfx_test_mode_info;
td_void mrs_handle_set_tone_mask(td_u16 test_duration, td_u8 pre_mode);
td_void mrs_handle_set_freq(td_u16 test_duration, td_u8 pre_mode);
static td_void mrs_set_test_mode_enable(td_bool enable);
static td_u32 mrs_check_msdu_to_com(EXT_CONST ext_capture_msdu_info *data);
static td_void mrs_dfx_enter_test_mode(td_u8 mode, td_u8 pre_mode);
static td_void mrs_config_uart_param(td_u32 rate, td_u32 parity);
static td_u32 mrs_check_phy_tans_test_to_com(EXT_CONST td_u8 *fc, EXT_CONST td_u8 *data, td_u16 pb_size);
static td_void mrs_dfx_test_mode_cnt(td_u8 test_mode);
static td_u32 mrs_change_baud_rate(td_u32 rate, td_u32 parity);
static td_void mrs_dfx_test_mode_err_frame_info(td_u8 pb_type);

td_void mrs_common_capture_init(td_void)
{
    mrs_set_test_mode_enable(TD_FALSE);

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
    /* Time of parsing test packet, 35s */
    mrs_timer_start(MRS_TIMER_ID_CCO_ENTER_TESTMODE, MRS_PARSE_TEST_PACKET_TIME * MRS_SEC_TO_MS, EXT_TIMER_TYPE_ONCE);
#endif

#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)
    mrs_timer_start(MRS_TIMER_ID_STA_ENTER_TESTMODE, MRS_PARSE_TEST_PACKET_TIME * MRS_SEC_TO_MS, EXT_TIMER_TYPE_ONCE);
#endif

    /* Register capture packet fun */
    uapi_register_capture_packet_function(mrs_capture_fun);
    uapi_mdm_register_test_mode(PHYSICAL_TEST_MODE_APP_TO_COM_LOOP, mrs_handle_app_to_com_loop_mode);
    uapi_mdm_register_test_mode(PHYSICAL_TEST_MODE_APP_TO_PLC_LOOP, mrs_handle_app_to_plc_loop_mode);
    uapi_mdm_register_test_mode(PHYSICAL_TEST_MODE_EXIT, mrs_handle_msdu_to_com_mode);
    uapi_mdm_register_test_mode(PHYSICAL_TEST_MODE_SET_FREQ, mrs_handle_set_freq);
    uapi_mdm_register_test_mode(PHYSICAL_TEST_MODE_SET_TONE_MASK_MODE, mrs_handle_set_tone_mask);
    /* Set capture mode */
    uapi_mdm_set_capture_mode(EXT_MAC_CAPTURE_SINGLE_PB_MODE);
}

td_void mrs_common_capture_deinit(td_void)
{
    /* Deregisters the MSDU interface */
    uapi_register_capture_msdu_function(TD_NULL);

    if (uapi_mdm_get_phy_test_mode() == PHYSICAL_TEST_MODE_NONE) {
        uapi_mdm_set_capture_mode(EXT_MAC_NO_CAPTURE_MODE);
        /* This command is used to restore the MSDU flag, which is not intercepted */
        uapi_set_intercept_msdu_enable(TD_FALSE);
    }

    uapi_register_capture_packet_function(TD_NULL);
}

static td_u32 mrs_check_phy_tans_test_to_com(EXT_CONST td_u8 *fc, EXT_CONST td_u8 *data, td_u16 pb_size)
{
    td_u32 ret = EXT_ERR_FAILURE;
    td_u16 total_len;
    td_u16 length;
    td_u8 *payload = TD_NULL;
    td_u16 paylaod_len;

    if (fc == TD_NULL) {
        return ret;
    }

    /* The beacon frame is not sent to the serial port channel */
    if ((fc[0] & 0x07) == 0) {   /* 7 is DT MASK */
        return ret;
    }

    g_dfx_test_mode_info.check_phy_tans_test_cnt++;

    paylaod_len = pb_size;
    total_len = MRS_FC_SIZE + paylaod_len;

    payload = mrs_malloc(total_len);
    if (payload == TD_NULL) {
        return EXT_ERR_MALLOC_FAILUE;
    }

    if (memset_s(payload, total_len, 0, total_len) != EXT_ERR_SUCCESS) {
        return EXT_ERR_FAILURE;
    }

    if (memcpy_s(payload, total_len, fc, MRS_FC_SIZE) != EXT_ERR_SUCCESS) {
        return EXT_ERR_FAILURE;
    }

    if (data != TD_NULL) {
        if (memcpy_s(payload + MRS_FC_SIZE, total_len - MRS_FC_SIZE, data, paylaod_len) != EXT_ERR_SUCCESS) {
            return EXT_ERR_FAILURE;
        }
    }

    if (total_len > EXT_DMS_FRM_MAX_PAYLOAD_SIZE) {
        length = total_len - EXT_DMS_FRM_MAX_PAYLOAD_SIZE;
        ret = mrs_uart_tx(payload, EXT_DMS_FRM_MAX_PAYLOAD_SIZE);
        if (ret == EXT_ERR_SUCCESS) {
            ret = mrs_uart_tx(payload + EXT_DMS_FRM_MAX_PAYLOAD_SIZE, length);
        }
    } else {
        ret = mrs_uart_tx(payload, total_len);
    }

    mrs_free(payload);

    return ret;
}

td_u32 mrs_capture_fun(ext_mdm_capture_packet_pbb_type state, EXT_CONST td_u8 *fc, EXT_CONST td_u8 *payload,
    td_u16 pb_size)
{
    td_u8 test_mode = uapi_mdm_get_phy_test_mode();

    /* The current mode is the MAC layer transparent transmission test mode,exit */
    if (test_mode == PHYSICAL_TEST_MODE_EXIT) {
        return EXT_ERR_SKIP;
    }

    /* Processing After Entering the physical layer transparent transmission test mode */
    if (test_mode == PHYSICAL_TEST_MODE_APP_TO_COM_LOOP) {
        return mrs_check_phy_tans_test_to_com(fc, payload, pb_size);
    }

    mrs_dfx_test_mode_cnt(test_mode);
    if (state != PBB_CORRECT_TYPE || fc == TD_NULL || payload == TD_NULL) {
        mrs_dfx_test_mode_err_frame_info((td_u8)state);
        return EXT_ERR_SKIP;
    }

    return EXT_ERR_SUCCESS;
}

/* Enter the physical layer transparent transmission test mode */
td_void mrs_handle_app_to_com_loop_mode(td_u16 test_duration, td_u8 pre_mode)
{
    uapi_mdm_enter_phy_test_mode(PHYSICAL_TEST_MODE_APP_TO_COM_LOOP);

    mrs_dfx_enter_test_mode(PHYSICAL_TEST_MODE_APP_TO_COM_LOOP, pre_mode);

    mrs_config_uart_param(BR115200, MRS_SRV_UART_PARITY_NONE);
    /* Setting the Packet Capture Mode */
    uapi_mdm_set_capture_mode(EXT_MAC_INTERCEPT_SINGLE_PB_MODE);
#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
    /* test mode duration£¬min */
    mrs_timer_start(MRS_TIMER_ID_CCO_PHY_TRANS_TESTMODE, mrs_min_to_ms(test_duration), EXT_TIMER_TYPE_ONCE);
#endif

#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)
    mrs_timer_start(MRS_TIMER_ID_STA_PHY_TRANS_TESTMODE, mrs_min_to_ms(test_duration), EXT_TIMER_TYPE_ONCE);
#endif

#if defined(PRODUCT_CFG_PRODUCT_TYPE_NDM)
    ext_unref_param(test_duration);
#endif
}

/* Enter the physical layer return test mode */
td_void mrs_handle_app_to_plc_loop_mode(td_u16 test_duration, td_u8 pre_mode)
{
    uapi_mdm_enter_phy_test_mode(PHYSICAL_TEST_MODE_APP_TO_PLC_LOOP);

    mrs_dfx_enter_test_mode(PHYSICAL_TEST_MODE_APP_TO_PLC_LOOP, pre_mode);
    /* Setting the Packet Capture Mode */
    uapi_mdm_set_capture_mode(EXT_MAC_INTERCEPT_LOOP_MODE);

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
    /* test mode duration£¬min */
    mrs_timer_start(MRS_TIMER_ID_CCO_PHY_LOOP_TESTMODE, mrs_min_to_ms(test_duration), EXT_TIMER_TYPE_ONCE);
#endif

#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)
    mrs_timer_start(MRS_TIMER_ID_STA_PHY_LOOP_TESTMODE, mrs_min_to_ms(test_duration), EXT_TIMER_TYPE_ONCE);
#endif

#if defined(PRODUCT_CFG_PRODUCT_TYPE_NDM)
    ext_unref_param(test_duration);
#endif
}

/* Enter the MAC layer transparent transmission test mode */
td_void mrs_handle_msdu_to_com_mode(td_u16 test_duration, td_u8 pre_mode)
{
    uapi_mdm_enter_phy_test_mode(PHYSICAL_TEST_MODE_EXIT);

    mrs_dfx_enter_test_mode(PHYSICAL_TEST_MODE_EXIT, pre_mode);
    /* Register and capture MSDU packets to the serial port channel. */
    uapi_register_capture_msdu_function(mrs_capture_msdu_fun);
    /* Intercepts MSDU and does not report it to the application layer and network management layer */
    uapi_set_intercept_msdu_enable(TD_TRUE);

    mrs_config_uart_param(BR115200, MRS_SRV_UART_PARITY_NONE);

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
    /* test mode duration£¬min */
    mrs_timer_start(MRS_TIMER_ID_CCO_MAC_MSDU_TESTMODE, mrs_min_to_ms(test_duration), EXT_TIMER_TYPE_ONCE);
#endif

#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)
    mrs_timer_start(MRS_TIMER_ID_STA_MAC_MSDU_TESTMODE, mrs_min_to_ms(test_duration), EXT_TIMER_TYPE_ONCE);
#endif

#if defined(PRODUCT_CFG_PRODUCT_TYPE_NDM)
    ext_unref_param(test_duration);
#endif
}

td_void mrs_handle_set_freq(td_u16 test_duration, td_u8 pre_mode)
{
    uapi_mdm_set_work_freq((td_u8)(test_duration));
    mrs_dfx_enter_test_mode(PHYSICAL_TEST_MODE_SET_FREQ, pre_mode);
}

td_void mrs_handle_set_tone_mask(td_u16 test_duration, td_u8 pre_mode)
{
    uapi_mdm_set_tone_mask_mode((td_u8)(test_duration));
    mrs_dfx_enter_test_mode(PHYSICAL_TEST_MODE_SET_TONE_MASK_MODE, pre_mode);
}

td_void mrs_capture_msdu_fun(EXT_CONST ext_capture_msdu_info *data)
{
    td_u8 mode = uapi_mdm_get_phy_test_mode();

    if (mode == PHYSICAL_TEST_MODE_EXIT) {
        mrs_check_msdu_to_com(data);
    }
}

static td_u32 mrs_check_msdu_to_com(EXT_CONST ext_capture_msdu_info *data)
{
    td_u32 ret;
    td_u16 total_length;
    td_u16 length;
    td_u8 *payload = TD_NULL;

    if (data == TD_NULL || data->msdu_size == 0 || data->msdu_data == TD_NULL) {
        return EXT_ERR_FAILURE;
    }

    g_dfx_test_mode_info.check_msdu_test_cnt++;

    total_length = data->msdu_size;
    payload = data->msdu_data;
    if (total_length > EXT_DMS_FRM_MAX_PAYLOAD_SIZE) {
        length = total_length - EXT_DMS_FRM_MAX_PAYLOAD_SIZE;
        ret = mrs_uart_tx(payload, EXT_DMS_FRM_MAX_PAYLOAD_SIZE);
        if (ret == EXT_ERR_SUCCESS) {
            ret = mrs_uart_tx(payload + EXT_DMS_FRM_MAX_PAYLOAD_SIZE, length);
        }
    } else {
        ret = mrs_uart_tx(payload, total_length);
    }

    return ret;
}

td_void mrs_set_work_freq_mode(td_u8 freq_mode)
{
    uapi_mdm_set_work_freq(freq_mode);
}

static td_void mrs_set_test_mode_enable(td_bool enable)
{
    g_test_mode_enable = enable;
}

td_bool mrs_get_test_mode_enable(td_void)
{
    return g_test_mode_enable;
}

td_u32 mrs_parse_app_data(EXT_CONST td_u8 *pbdata, td_u8 **pdata)
{
    td_u8 msdu_type;
    td_u8 is_mac_exist;
    td_u8 *buf = (td_u8 *)(pbdata + MRS_PBH_SIZE_P1901);
    td_u8 app_offset;

    msdu_type = buf[MRS_HEAD_MSDU_TYPE_OFFSET_P1901];
    if (msdu_type != MRS_MSDU_TYPE_APP_P1901) {
        return EXT_ERR_FAILURE;
    }

    is_mac_exist = buf[MRS_MAC_EXIST_FLAG_OFFSET_P1901] & 0x08;
    if (is_mac_exist == TD_TRUE) {
        app_offset = MRS_LONG_HEAD_SIZE_P1901;
    } else {
        app_offset = MRS_SHORT_HEAD_SIZE_P1901;
    }

    *pdata = buf + app_offset;

    return EXT_ERR_SUCCESS;
}

static td_void mrs_config_uart_param(td_u32 rate, td_u32 parity)
{
    mrs_change_baud_rate(rate, parity);
}

static td_u32 mrs_change_baud_rate(td_u32 rate, td_u32 parity)
{
    td_u32 ret;
    ext_uart_init_type uart_param = {BR115200, MRS_SRV_UART_DATABITS, MRS_SRV_UART_STOPBITS, MRS_SRV_UART_PARITY_NONE};
    uart_param.baud_rate = rate;
    uart_param.parity = parity;

    (td_void) uapi_uart_close(g_mrs_default_port);
    ret = uapi_uart_open(g_mrs_default_port);
    if (ret != EXT_ERR_SUCCESS) {
        return ret;
    }

    ret = uapi_uart_ioctl(g_mrs_default_port, UART_CFG_SET_ATTR, (uintptr_t)&uart_param);
    if (ret != EXT_ERR_SUCCESS) {
        return ret;
    }

    return ret;
}

td_void mrs_restore_uart_config(td_u32 rate, td_u32 parity)
{
    mrs_change_baud_rate(rate, parity);
}

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO) || defined(PRODUCT_CFG_PRODUCT_TYPE_STA)
td_void mrs_enter_test_mode(td_void)
{
    if (uapi_mdm_get_phy_test_mode() == PHYSICAL_TEST_MODE_APP_TO_COM_LOOP ||
        uapi_mdm_get_phy_test_mode() == PHYSICAL_TEST_MODE_EXIT) {
        return;
    }

    mrs_common_capture_deinit();
}

td_void mrs_exit_phy_tans_test_mode(td_void)
{
    td_u8 mode = uapi_mdm_get_phy_test_mode();
    if (mode == PHYSICAL_TEST_MODE_APP_TO_COM_LOOP) {
        uapi_mdm_exit_phy_test_mode();
    }
}

td_void mrs_exit_phy_loop_test_mode(td_void)
{
    uapi_mdm_exit_phy_test_mode();
}

td_void mrs_exit_mac_msdu_test_mode(td_void)
{
    td_u8 mode = uapi_mdm_get_phy_test_mode();
    if (mode == PHYSICAL_TEST_MODE_EXIT) {
        uapi_mdm_exit_phy_test_mode();
    }
}
#endif

static td_void mrs_dfx_test_mode_err_frame_info(td_u8 pb_type)
{
    g_dfx_test_mode_info.rcv_frame_err_cnt++;
    g_dfx_test_mode_info.payload_type = pb_type;
    g_dfx_test_mode_info.rcv_frame_err_time = uapi_get_seconds();
}

static td_void mrs_dfx_test_mode_cnt(td_u8 test_mode)
{
    g_dfx_test_mode_info.count++;
    g_dfx_test_mode_info.mode = test_mode;
    g_dfx_test_mode_info.latest_report_time = uapi_get_seconds();
}

td_void mrs_dfx_test_mode_invalid_frame(td_u8 port, td_u8 test_id)
{
    g_dfx_test_mode_info.invalid_port = port;
    g_dfx_test_mode_info.invalid_test_id = test_id;
}

static td_void mrs_dfx_enter_test_mode(td_u8 mode, td_u8 pre_mode)
{
    diag_cmd_phy_test_mode_inf *p_mode = &(g_dfx_test_mode_info.mode_info[mode]);

    p_mode->enter_test_mode_time = uapi_get_seconds();
    p_mode->test_mode_flag = TD_TRUE;
    p_mode->pre_mode = pre_mode;
}

td_void mrs_dfx_exit_test_mode(td_u8 mode, td_u8 cur_mode)
{
    diag_cmd_phy_test_mode_inf *p_mode = &(g_dfx_test_mode_info.mode_info[mode]);

    p_mode->exit_test_mode_time = uapi_get_seconds();
    p_mode->test_mode_flag = TD_FALSE;
    p_mode->pre_mode = cur_mode;
}

td_u32 mrs_cmd_query_test_mode_info(td_u16 id, td_pvoid cmd, td_u16 cmd_size, td_u8 option)
{
    diag_cmd_test_mode_info_ind *ind = &g_dfx_test_mode_info;

    ext_unref_param(cmd_size);
    ext_unref_param(cmd);

    return uapi_diag_report_packet(id, option, (td_pbyte)ind, sizeof(diag_cmd_test_mode_info_ind), TD_FALSE);
}


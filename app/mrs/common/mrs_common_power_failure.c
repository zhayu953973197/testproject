/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS COMMON POWER FAILURE handle.
 */

#include "mrs_common_power_failure.h"
#include "mrs_common_tools.h"
#include "mrs_sta_io.h"
#include "soc_mdm_mac.h"
#include "mrs_sta_power_failure.h"
#include "mrs_cco_power_failure.h"
/************************************************************
                       gloal variables
************************************************************/
mrs_power_failure_ctrl_st g_power_failure_info = { 0 };
#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)
EXT_EXTERN power_failure_ctrl_st g_pwf_ctrl;
#endif

td_void mrs_common_power_failure_init(td_void)
{
    mrs_power_failure_ctrl_st *power_failure_info = mrs_get_power_failure_ctrl();
    td_u16 len = POWER_FAILURE_BITMAP_LEN;
#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)
    power_failure_ctrl_st *pwf_ctrl = TD_NULL;
#endif

    power_failure_info->random_max_value = RANDOM_MAX_VALUE;
    power_failure_info->random_min_value = RANDOM_MIN_VALUE;
    power_failure_info->send_interval = SEND_INTERVAL;
    power_failure_info->converge_time = CONVERGE_TIME;
    power_failure_info->send_times = SEND_TIMES;
    power_failure_info->power_failure_report_enable = POWER_FAILURE_REPORT_ENABLE;

#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)
    uapi_set_power_failure_module_gpio_init(EXT_GPIO_IDX_0, 0, EXT_GPIO_DIRECTION_IN);

    pwf_ctrl = &g_pwf_ctrl;
    pwf_ctrl->pwf_switch = TD_TRUE;
    pwf_ctrl->power_failure_recover_thd = PWF_FAILURE_RECOVER_THD;
#endif

    power_failure_info->power_failure_bitmap = (td_u8 *)mrs_malloc(len);
    if (power_failure_info->power_failure_bitmap != TD_NULL) {
        memset_s(power_failure_info->power_failure_bitmap, len, 0, len);
    } else {
        return;
    }

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
    power_failure_info->cco_diff_bitmap = (td_u8 *)mrs_malloc(len);
    if (power_failure_info->cco_diff_bitmap != TD_NULL) {
        memset_s(power_failure_info->cco_diff_bitmap, len, 0, len);
    } else {
        mrs_free(power_failure_info->power_failure_bitmap);
        return;
    }

    power_failure_info->cco_last_send_bitmap = (td_u8 *)mrs_malloc(len);
    if (power_failure_info->cco_last_send_bitmap != TD_NULL) {
        memset_s(power_failure_info->cco_last_send_bitmap, len, 0, len);
    } else {
        mrs_free(power_failure_info->power_failure_bitmap);
        mrs_free(power_failure_info->cco_diff_bitmap);
        return;
    }
#endif
}

mrs_power_failure_ctrl_st *mrs_get_power_failure_ctrl(td_void)
{
    return &g_power_failure_info;
}

/* Obtain power outage data of other sites (update the bitmap). */
td_void mrs_power_failure_get_others_frame_data(const mrs_meter_event_info *evt_frm)
{
    td_u16 *data_tei = TD_NULL;
    td_u16 i;
    td_u16 temp_tei;
    mrs_power_failure_ctrl_st *power_failure_info = mrs_get_power_failure_ctrl();
    td_u8 *refresh_bitmap = TD_NULL;
    td_u8 rcv_tei[DATA_END_TEI_POSITION] = { 0 };

    if (power_failure_info->power_failure_bitmap == TD_NULL) {
        return;
    }

    refresh_bitmap = power_failure_info->power_failure_bitmap;

    for (i = 0; i < DATA_END_TEI_POSITION; i++) {
        rcv_tei[i] = evt_frm->data[i + 1];
    }

    data_tei = (td_u16 *)rcv_tei;

    mrs_printf("[start_tei = %d]", *data_tei);

    /* update the bitmap */
    for (i = 0; i < (evt_frm->data_len - DATA_FIX_SIZE) * BIT_SIZE; i++) {
        if (mrs_power_failure_bit_check(&evt_frm->data[DATA_FIX_SIZE], i) == TD_TRUE) {
            temp_tei = (*data_tei) + i;
            mrs_power_failure_bit_set(refresh_bitmap, temp_tei);
        }
    }
}

td_void mrs_power_failure_bit_set(td_u8 *map, td_u16 pos)
{
    td_u16 byte_offset = pos / BIT_SIZE;
    td_u16 bit_offset = pos % BIT_SIZE;

    bit_set(map[byte_offset], bit_offset);
}

td_bool mrs_power_failure_bit_check(const td_u8 *map, td_u16 pos)
{
    td_u16 byte_offset = pos / BIT_SIZE;
    td_u16 bit_offset = pos % BIT_SIZE;

    return (td_bool)(bit_get(map[byte_offset], bit_offset));
}

/* Calculate the minimum tei. */
td_u16 mrs_calc_min_tei(const td_u8 *bitmap)
{
    td_u16 i;
    td_u16 bitmap_min_tei = 0;

    for (i = MIN_START_TEI; i < POWER_FAILURE_BITMAP_LEN * BIT_SIZE; i++) {
        if (mrs_power_failure_bit_check(bitmap, i) == TD_TRUE) {
            bitmap_min_tei = i;
            break;
        }
    }

    return bitmap_min_tei;
}

td_u16 mrs_calc_max_tei(const td_u8 *bitmap)
{
    td_u16 i;
    td_u16 bitmap_max_tei = 0;

    for (i = (td_u16)(POWER_FAILURE_BITMAP_LEN * BIT_SIZE - 1); i >= 1; i--) {
        if (mrs_power_failure_bit_check(bitmap, i) == TD_TRUE) {
            bitmap_max_tei = i;
            break;
        }
    }

    return bitmap_max_tei;
}

/* Check whether the value of the bitmap data field is all 0. */
td_bool mrs_data_is_all_zero(const td_u8 *map, td_u32 len, td_u8 ch)
{
    td_u32 i;

    if (map == TD_NULL) {
        return TD_FALSE;
    }

    for (i = 0; i < len; i++) {
        if ((td_u8)map[i] != ch) {
            return TD_FALSE;
        }
    }

    return TD_TRUE;
}

#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA) || defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
td_u32 mrs_handle_power_failure_rejoin_event(td_u8 rejoin_reason)
{
    mrs_power_failure_ctrl_st *power_failure_info = mrs_get_power_failure_ctrl();

    ext_unref_param(rejoin_reason);
    mrs_printf("[PF_REJOIN_EVENT]: reason= %d\n", rejoin_reason);

#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)
    mrs_power_failure_rcv_report *power_failure_rcv_info = mrs_get_power_failure_report_info();

    if (power_failure_rcv_info->device_power_failure == TD_TRUE) {
        return EXT_ERR_FAILURE;
    }

    mrs_sta_power_failure_info_erase(power_failure_info);
#endif

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
    if (power_failure_info->cco_diff_bitmap != TD_NULL) {
        if (memset_s(power_failure_info->cco_diff_bitmap, POWER_FAILURE_BITMAP_LEN, 0,
                     POWER_FAILURE_BITMAP_LEN) != EXT_ERR_SUCCESS) {
            return EXT_ERR_FAILURE;
        }
    }

    mrs_cco_erase_common_bitmap(power_failure_info);
#endif
    return EXT_ERR_SUCCESS;
}
#endif

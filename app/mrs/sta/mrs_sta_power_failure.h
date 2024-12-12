/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS STA POWER FAILURE report.
 */

#ifndef __MRS_STA_POWER_FAILURE_H__
#define __MRS_STA_POWER_FAILURE_H__

#include "mrs_common_plc.h"
#include "mrs_msg.h"
#include "mrs_sta_srv.h"
#include "mrs_sta_evt.h"
#include "mrs_common_power_failure.h"
#include "soc_mdm_power_failure.h"
#ifdef __cplusplus
extern "C" {
#endif

#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)
/************************************************************
                                   Macro Definition
************************************************************/
#define STA_DEFAULT_SEND_CNT    10
#define PWF_SELF_CHECK_STATE    0 /* Self-check Status */
#define PWF_POWER_UP_STATE      1 /* Power-on Status */
#define PWF_POWER_FAILURE_STATE 2 /* Power-off Status */
#define PWF_FAILURE_RECOVER_THD 180 /* Power-failure protection threshold,unit:second */

/************************************************************
                             structs
************************************************************/
typedef struct {
    td_u16 my_tei;
    td_bool device_power_failure;
    td_bool device_pull_reset;

    td_bool is_plug_module;
    td_u8 pad[3]; /* reserved 3 bytes */
} mrs_power_failure_rcv_report;

typedef struct {
    td_bool pwf_switch;               /* Power outage detection switch */
    td_u8 pwf_state;                  /* Power outage detection status */
    td_u16 power_failure_recover_thd; /* Threshold for forcibly clearing the power failure (configured by the APP
                                         parameter) */

    td_u32 pwf_power_failure_time; /* Start time of a power outage, in seconds. */
} power_failure_ctrl_st;

mrs_power_failure_rcv_report *mrs_get_power_failure_report_info(td_void);
td_u32 mrs_sta_handle_local_power_failure(td_void);
td_void mrs_sta_create_power_failure_frame_proc(td_void);
td_u32 mrs_sta_create_power_failure_frame(td_u8 event_type);
td_void mrs_sta_power_failure_set_send_info(td_u8 event_type, mrs_plc_frame_data *power_frame);
td_void mrs_sta_handle_wait_power_failure_ack(td_void);
td_u32 mrs_sta_send_power_failure_frame(EXT_CONST mrs_plc_frame_data *power_frame);
td_u32 mrs_handle_others_power_failure_frame(td_void);
td_u32 mrs_sta_handle_power_failue_ack(td_u16 seq);
td_void mrs_sta_update_normal_bitmap(td_u8 *bitmap_in, td_u8 *bitmap_out, td_u16 bitmap_len);
td_void mrs_sta_power_failure_info_erase(mrs_power_failure_ctrl_st *power_failure_info);
td_void mrs_sta_reset_handle(td_void);
td_void mrs_power_failure_pull_rst_handle(td_void);
td_void mrs_pwf_init(td_void);
td_void mrs_pwf_check_main(td_void);
td_u8 mrs_pwf_self_check_state(td_bool is_power_failure);
td_u8 mrs_pwf_power_on_state(td_bool is_power_failure);
td_u8 mrs_pwf_power_failure_state(td_bool is_power_failure);
#endif

#ifdef __cplusplus
}
#endif

#endif

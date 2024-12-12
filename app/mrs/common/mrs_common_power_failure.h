/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS COMMON POWER FAILURE handle.
 */

#ifndef __MRS_COMMON_POWER_FAILURE_H__
#define __MRS_COMMON_POWER_FAILURE_H__

#include "mrs_common_plc.h"
#include "soc_mdm_mac.h"

/************************************************************
                        macro definition
************************************************************/
#define BIT_SIZE      8
#define POWER_FAILURE 1 /* Indicates the event type reported by the STA. The value 1 indicates a power
                                           outage event. */
#define DATA_FIX_SIZE               3
#define DATA_START_TEI_POSITION     1
#define DATA_END_TEI_POSITION       2
#define MRS_US_TO_S_MASK            1000
#define MIN_START_TEI               2
#define RANDOM_MAX_VALUE            200
#define RANDOM_MIN_VALUE            0
#define SEND_INTERVAL               2000
#define CONVERGE_TIME               30
#define SEND_TIMES                  10
#define POWER_FAILURE_REPORT_ENABLE 1

#define bit_set(x, i) ((x) = (td_u8)(((1 << (i)) | (x))))
#define bit_get(x, i) (((1 << (i)) & (x)) ? 1 : 0)

/************************************************************
                             structs
************************************************************/
typedef struct {
    td_u8 *power_failure_bitmap; /* Power failure collection bitmap */

    td_u16 random_min_value; /* Minimum time when a site is powered off. The unit is millisecond. Default value is 0. */
    td_u16 random_max_value; /* Indicates the maximum random reporting time of a power outage site, in milliseconds.
                                The default value is 200. */

    td_u16 send_interval; /* Interval for sending power outage sites, in milliseconds. The default value is 2000. */
    td_u8 send_times;     /* Indicates the number of power outage packets sent by a power outage site. The default value
                             is 10. */
    td_u8 converge_time;  /* Collection wait time, in seconds. The default value is 30. */

    td_u16 ack_seq; /* Indicates the sequence number of the ACK message. */
    td_bool wait_ack_flag;
    td_u8 wait_ack_type;

    td_u8 send_report_times; /* Number of times that a power failure occurs. */
    td_u8 start_send_time;   /* Indicates the number of times that a power failure message is sent when a power failure
                              occurs. */
    td_u8 power_failure_report_enable;
    td_bool is_ready_to_send_power_failure;

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
    td_u8 *cco_diff_bitmap;      /* The newly reported bitmap is required. */
    td_u8 *cco_last_send_bitmap; /* cco Content of the bitmap last reported by the concentrator */

    td_u8 cco_wait_erase;           /* cco Number of times that data is to be erased */
    td_bool cco_erease_bitmap_flag; /* cco Erase the bitmap flag bit. */
    td_u8 pad[2];                   /* reserved 2 */
#endif
} mrs_power_failure_ctrl_st;

/************************************************************
                             functions
************************************************************/
td_void mrs_common_power_failure_init(td_void);
mrs_power_failure_ctrl_st *mrs_get_power_failure_ctrl(td_void);
td_void mrs_power_failure_get_others_frame_data(const mrs_meter_event_info *evt_frm);
td_void mrs_power_failure_bit_set(td_u8 *map, td_u16 pos);
td_bool mrs_power_failure_bit_check(const td_u8 *map, td_u16 pos);
td_u16 mrs_calc_min_tei(const td_u8 *bitmap);
td_u16 mrs_calc_max_tei(const td_u8 *bitmap);
td_bool mrs_data_is_all_zero(const td_u8 *map, td_u32 len, td_u8 ch);
#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA) || defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
td_u32 mrs_handle_power_failure_rejoin_event(td_u8 rejoin_reason);
#endif
#endif

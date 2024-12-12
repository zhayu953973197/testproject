/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS CCO POWER FAILURE handle.
 */

#ifndef __MRS_CCO_POWER_FAILURE_H__
#define __MRS_CCO_POWER_FAILURE_H__

#include <dfx_app.h>
#include "soc_mdm_mac.h"
#include "mrs_common_power_failure.h"
#include "mrs_common_plc.h"
#include "mrs_config.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************
                                   macro definitions
************************************************************/
#define CCO_ERASE_BITMAP     (8)
#define MAX_REPORT_TEI_CNT   41
#define DEFAULT_MAX_NODE_NUM 512

/************************************************************
                             api definition
************************************************************/
td_bool mrs_cco_not_reply_ack(td_u8 rcv_mac[EXT_PLC_MAC_ADDR_LEN]);
td_u32 mrs_cco_evt_report(td_void);
td_void mrs_cco_rsp_power_failure_evt(td_void);
td_u32 mrs_cco_create_rsp_power_frame(td_void);
td_u32 mrs_cco_create_report_frame(td_pbyte *buf, td_u16 *data_len, td_u16 *start_tei_pos);
td_void mrs_cco_report_mac_addr(td_u8 *diff_bitmap, td_u16 start_pos, td_u16 report_len, td_u8 *mac_addr,
                                td_u16 *next_pos);
td_void mrs_cco_report_bitmap(td_void);
td_u32 mrs_cco_get_bitmap_tei_cnt(td_u16 bitmap_min_tei, td_u16 bitmap_max_tei, td_u8 *bitmap);
td_void mrs_cco_erase_bitmap(td_void);
td_u32 mrs_cco_erase_common_bitmap(mrs_power_failure_ctrl_st *power_failure_info);
td_void mrs_cco_power_failure_dfx_info(td_void);
td_u32 mrs_cmd_pwf(td_u16 id, td_pvoid cmd, td_u16 cmd_size, td_u8 option);

#ifdef __cplusplus
}
#endif

#endif
#endif

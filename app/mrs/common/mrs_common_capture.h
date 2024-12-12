/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS CAPTURE FUN handle.(Development of Tested Module Based on 1901.1 Test System)
 */

#ifndef __MRS_COMMON_CAPTURE_H__
#define __MRS_COMMON_CAPTURE_H__

#include "soc_mdm.h"
#include "soc_lowpower.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MRS_SEC_TO_MS  1000
#define MRS_MIN_TO_SEC 60
#define MRS_MS_PER_MIN      (MRS_MIN_TO_SEC * MRS_SEC_TO_MS)
#define mrs_min_to_ms(min)  ((td_u32)(min) * MRS_MS_PER_MIN)
#define MRS_PARSE_TEST_PACKET_TIME  35
#define MRS_HEAD_PORT_OFFSET     0
#define MRS_HEAD_PKT_ID_OFFSET   1
#define MRS_PLC_APP_HEAD_LEN     4
#define MRS_PBH_SIZE_P1901       4
#define MRS_PKT_ID_PLC_TEST                     0x0006  /* Test packet ID */
#define MRS_APP_PORT_NUM                        0x11    /* Meter port number */
#define MRS_HEAD_MSDU_TYPE_OFFSET_P1901         4       /* Msdu type offset */
#define MRS_MSDU_TYPE_APP_P1901                 48      /* Application layer packet */
#define MRS_MAC_EXIST_FLAG_OFFSET_P1901         12      /* Mac exit flag offset */

#define MRS_LONG_HEAD_SIZE_P1901  28  /* Long head size */
#define MRS_SHORT_HEAD_SIZE_P1901 16  /* Short head size */
#define MRS_FC_SIZE   16

typedef struct {
    td_u32 test_proc_id     : 6;    /* Test Protocol ID */
    td_u32 length           : 6;    /* Packet Length */
    td_u32 test_mode        : 4;    /* Test Mode */
    td_u32 pad              : 4;    /* Reserved */
    td_u32 test_para        : 12;   /* Test mode duration (unit: minute) */
}mrs_app_test_frame_st;

td_void mrs_common_capture_init(td_void);
td_void mrs_handle_app_to_com_loop_mode(td_u16 test_duration, td_u8 pre_mode);
td_void mrs_handle_app_to_plc_loop_mode(td_u16 test_duration, td_u8 pre_mode);
td_void mrs_handle_msdu_to_com_mode(td_u16 test_duration, td_u8 pre_mode);
td_bool mrs_get_test_mode_enable(td_void);
td_void mrs_dfx_exit_test_mode(td_u8 mode, td_u8 cur_mode);
td_u32 mrs_parse_app_data(EXT_CONST td_u8 *pbdata, td_u8 **pdata);
td_void mrs_capture_msdu_fun(EXT_CONST ext_capture_msdu_info *data);
td_void mrs_set_work_freq_mode(td_u8 freq_mode);
td_u32 mrs_capture_fun(ext_mdm_capture_packet_pbb_type state, EXT_CONST td_u8 *fc, EXT_CONST td_u8 *payload,
    td_u16 pb_size);
td_void mrs_enter_test_mode_func(EXT_CONST mrs_app_test_frame_st *frame, td_u8 mode);
td_void mrs_restore_uart_config(td_u32 rate, td_u32 parity);
#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO) || defined(PRODUCT_CFG_PRODUCT_TYPE_STA)
td_void mrs_enter_test_mode(td_void);
td_void mrs_common_capture_deinit(td_void);
td_void mrs_exit_phy_loop_test_mode(td_void);
td_void mrs_exit_mac_msdu_test_mode(td_void);
td_void mrs_exit_phy_tans_test_mode(td_void);
#endif
/* DFX INF */
td_void mrs_dfx_test_mode_invalid_frame(td_u8 port, td_u8 test_id);
td_u32 mrs_cmd_query_test_mode_info(td_u16 id, td_pvoid cmd, td_u16 cmd_size, td_u8 option);

#ifdef __cplusplus
}
#endif

#endif /* __MRS_COMMON_CAPTURE_H__ */

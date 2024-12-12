/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: CCO upgrade implementation.
 */

#ifndef __MRS_CCO_UPG_H__
#define __MRS_CCO_UPG_H__

#include "mrs_config.h"
#include "soc_mdm_upg.h"
#include "mrs_proto_1376_2.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
EXT_START_HEADER

#define MRS_CCO_UPG_ERROR_SEG (0x0000FFFF)
/* File flag */
#define MRS_CCO_UPG_FILE_FLAG_CLR (0x00)
#define MRS_CCO_UPG_FILE_FLAG_CCO (0x03)
#define MRS_CCO_UPG_FILE_FLAG_STA (0x08)

/* Data transfer format */
#define MRS_CCO_UPG_FILE_FLAG_OFFSET     (0)
#define MRS_CCO_UPG_FILE_FLAG_SIZE       (1)
#define MRS_CCO_UPG_FILE_PROPERTY_OFFSET (MRS_CCO_UPG_FILE_FLAG_OFFSET + MRS_CCO_UPG_FILE_FLAG_SIZE)
#define MRS_CCO_UPG_FILE_PROPERTY_SIZE   (1)
#define MRS_CCO_UPG_FILE_INS_OFFSET      (MRS_CCO_UPG_FILE_PROPERTY_OFFSET + MRS_CCO_UPG_FILE_PROPERTY_SIZE)
#define MRS_CCO_UPG_FILE_INS_SIZE        (1)
#define MRS_CCO_UPG_SEG_TOTOAL_OFFSET    (MRS_CCO_UPG_FILE_INS_OFFSET + MRS_CCO_UPG_FILE_INS_SIZE)
#define MRS_CCO_UPG_SEG_TOTOAL_SIZE      (2)
#define MRS_CCO_UPG_SEG_CURRENT_OFFSET   (MRS_CCO_UPG_SEG_TOTOAL_OFFSET + MRS_CCO_UPG_SEG_TOTOAL_SIZE)
#define MRS_CCO_UPG_SEG_CURRENT_SIZE     (4)
#define MRS_CCO_UPG_SEG_DATA_LEN_OFFSET  (MRS_CCO_UPG_SEG_CURRENT_OFFSET + MRS_CCO_UPG_SEG_CURRENT_SIZE)
#define MRS_CCO_UPG_SEG_DATA_LEN_SIZE    (2)
#define MRS_CCO_UPG_SEG_DATA_OFFSET      (MRS_CCO_UPG_SEG_DATA_LEN_OFFSET + MRS_CCO_UPG_SEG_DATA_LEN_SIZE)
#define MRS_CCO_UPG_FRAME_DATA_SIZE_MIN  (MRS_CCO_UPG_FILE_FLAG_SIZE + MRS_CCO_UPG_FILE_PROPERTY_SIZE + \
                                         MRS_CCO_UPG_FILE_INS_SIZE + MRS_CCO_UPG_SEG_TOTOAL_SIZE +     \
                                         MRS_CCO_UPG_SEG_CURRENT_SIZE + MRS_CCO_UPG_SEG_DATA_LEN_SIZE)

typedef struct {
    td_u8 file_flag;
    td_u8 file_property;
    td_u8 file_ins;
    td_u8 rsv;
    td_u32 seg_curr;
    td_u16 seg_totoal;
    td_u16 data_len;
    td_u8 *data;
} mrs_upg_param;

/* Initalize */
td_u32 mrs_upg_init(td_void);
/* File receive handler */
td_u32 mrs_upg_file_rx(mrs_upg_param *upg_param, td_u32 *segment);
/* Clear file handler */
td_u32 mrs_upg_clr(td_void);
/* Store 376.2 command frame */
td_void mrs_cco_upg_set_frame(const mrs_proto_1376_2_frame *frame);
/* Check CCO status of upgrade */
td_bool mrs_cco_upg_is_idle(td_void);
/* SW Verson reponse received */
td_void mrs_cco_upg_get_sw_ver_rx(const td_u8 addr[MRS_METER_ADDR_LEN], td_u8 *ver_str, td_u16 ver_len);
/* Receive file timeout reset */
td_void mrs_cco_upg_reset_file_rx_timeout(td_void);
/* Timer handlers */
td_void mrs_cco_upg_file_rx_on_timer(td_void);
td_void mrs_cco_upg_stop_on_timer(td_void);
td_void mrs_cco_upg_stat_query_bs_on_timer(td_void);
td_void mrs_cco_upg_stat_timeout(td_void);
td_void mrs_cco_upg_plc_wait_timeout(td_void);
td_void mrs_cco_upg_get_sw_ver_timeout(td_void);
td_void mrs_cco_upg_wait_resp_done_timeout(td_void);

EXT_END_HEADER
#endif /* PRODUCT_CFG_PRODUCT_TYPE_CCO */
#endif /* __MRS_CCO_UPG_H__ */


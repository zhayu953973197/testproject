/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS cco 1376.2 handle.
 */

#ifndef __MRS_CCO_1376_2_H__
#define __MRS_CCO_1376_2_H__

#include "mrs_proto_1376_2.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
#ifdef __cplusplus
extern "C" {
#endif


/* inner struct of 376.2 frame and reply frame */
typedef struct {
    mrs_proto_1376_2_frame *frame; /* < structure of frame */
    td_pbyte *out_buf;             /* < data domain data of reply frame */
    td_u16 *out_len;               /* < data length of reply frame data domain */
    td_u8 afn;                     /* < reply frame AFN */
    td_u8 fn;                      /* < reply frame FN */
    td_u16 rsv;
} mrs_fn_param;

/* reply with affirm frame */
td_u32 mrs_cco_1376_2_ack_frame(const mrs_fn_param *dl_frame, td_u16 wait_time);
/* reply with denial frame */
td_u32 mrs_cco_1376_2_deny_frame(const mrs_fn_param *dl_frame, td_u8 deny);
/* organize relpy frame */
td_u32 mrs_cco_1376_2_easy_encode(mrs_fn_param *param, td_pbyte data, td_u16 length);
/* CCO power-on initialization finished. report AFN03H-F10 by self. */
td_void mrs_cco_report_afn_03f10(td_void);

#define MRS_AFN_10_F27_NODE_PAD      3
#define MRS_AFN_QUERY_NODE_MAX_NUM   36
#define MRS_AFN_QUERY_TOPO_NODE_NUM_MAX 64
#define MRS_AFN_10_F230_DATA_FIX_LEN  5
#define MRS_AFN_10_F230_TOPO_INFO_LEN 11
#define MRS_CCO_AFN10_DOWN_DATA_LEN 3
#define MRS_CCO_AFN10_QUERY_NUM_OFFSET  2
#define MRS_CCO_TOPO_ROLE_OFFSET  4
#define MRS_CCO_TOPO_REPORT_NUM_OFFSET  4
#define MRS_CCO_START_SEQ_ONE 1
#ifdef __cplusplus
}
#endif

#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_CCO) */
#endif /* __MRS_CCO_1376_2_H__ */

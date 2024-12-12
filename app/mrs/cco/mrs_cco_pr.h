/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS cco mr - AFN=F1H-F1.
 */

#ifndef __MRS_CCO_PR_H__
#define __MRS_CCO_PR_H__

#include "mrs_proto_1376_2.h"
#include "mrs_common_plc.h"
#include "mrs_cco_mr_list.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
#ifdef __cplusplus
extern "C" {
#endif


/* organize PLC frame of parallel read meter */
td_u32 mrs_cco_create_pr_plc_frame(mr_task_node *node, td_pbyte buffer, td_u16 length);
/* organize 376.2 frame of parallel read meter */
td_u32 mrs_cco_create_pr_1376_2_frame(td_pbyte *out_buffer, td_u16 *out_len,
                                      mr_task_node *node, td_pbyte buffer, td_u16 length);

#ifdef __cplusplus
}
#endif

#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_CCO) */
#endif /* __MRS_CCO_PR_H__ */


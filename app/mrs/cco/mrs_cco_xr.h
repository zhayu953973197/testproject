/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS cco mr - AFN=13H-F1.
 */

#ifndef __MRS_CCO_XR_H__
#define __MRS_CCO_XR_H__

#include "mrs_proto_1376_2.h"
#include "mrs_common_plc.h"
#include "mrs_cco_mr_list.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
#ifdef __cplusplus
extern "C" {
#endif

/* organize transparent read meter frame with PLC treaty */
td_u32 mrs_cco_create_xr_plc_frame(mr_task_node *node, td_pbyte buffer, td_u16 length);
/* organize transparent read meter frame with 376.2 treaty */
td_u32 mrs_cco_create_xr_1376_2_frame(td_pbyte *out_buffer, td_u16 *out_len,
                                      mr_task_node *node, td_pbyte buffer, td_u16 length);

#ifdef __cplusplus
}
#endif
#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_CCO) */
#endif /* __MRS_CCO_XR_H__ */


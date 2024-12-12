/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: CCO broadcast implementation.
 */

#ifndef __MRS_CCO_BROADCAST_H__
#define __MRS_CCO_BROADCAST_H__

#include "mrs_config.h"
#include "mrs_proto_1376_2.h"
#include "mrs_common_plc.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
#ifdef __cplusplus
extern "C" {
#endif


#define MRS_CCO_BC_CTRL_LEN          (1)
#define MRS_CCO_BC_MSG_LEN_SIZE      (1)
#define MRS_CCO_BC_MSG_LONG_LEN_SIZE (2)

/* Broadcast command handler */
td_u32 mrs_cco_broadcast_proc(td_u8 *deny_code, const td_pbyte content, td_u16 content_len);
/* Handler of sending broadcast frame on PLC */
td_void mrs_cco_broadcast_send_timeout(td_void);

#ifdef __cplusplus
}
#endif


#endif /* define PRODUCT_CFG_PRODUCT_TYPE_CCO */
#endif /* __MRS_CCO_BROADCAST_H__ */

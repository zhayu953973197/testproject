/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS sta diag cmd handle.
 */

#ifndef __MRS_STA_DIAG_H__
#define __MRS_STA_DIAG_H__

#include "mrs_dfx.h"
#include "mrs_common_capture.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)
#ifdef __cplusplus
extern "C" {
#endif

#define MRS_DIAG_STA_CMD_OBJS_TBL                                                      \
    {                                                                                  \
        { ID_DIAG_CMD_UART_MSG_SEND_INF, ID_DIAG_CMD_PLC_MSG_PROC_INF, mrs_diag_dfx }, \
        { ID_DIAG_CMD_PHY_TEST_INFO, ID_DIAG_CMD_PHY_TEST_INFO, mrs_cmd_query_test_mode_info }, \
    }

#define MRS_DIAG_CMD_OBJS_TBL MRS_DIAG_STA_CMD_OBJS_TBL

#ifdef __cplusplus
}
#endif
#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_STA) */
#endif /* __MRS_STA_DIAG_H__ */

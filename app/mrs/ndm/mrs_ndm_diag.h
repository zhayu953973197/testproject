/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS ndm diag cmd handle.
 */

#ifndef __MRS_NDM_DIAG_H__
#define __MRS_NDM_DIAG_H__

#include "mrs_dfx.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_NDM)
#ifdef __cplusplus
extern "C" {
#endif

#define MRS_DIAG_NDM_CMD_OBJS_TBL                                                 \
    {                                                                             \
        { ID_DIAG_CMD_UART_MSG_SEND_INF, ID_DIAG_CMD_PLC_MSG_PROC_INF, TD_NULL }, \
    }

#define MRS_DIAG_CMD_OBJS_TBL MRS_DIAG_NDM_CMD_OBJS_TBL

#ifdef __cplusplus
}
#endif
#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_NDM) */
#endif /* __MRS_NDM_DIAG_H__ */

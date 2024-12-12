/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS cco diag cmd handle.
 */

#ifndef __MRS_CCO_DIAG_H__
#define __MRS_CCO_DIAG_H__

#include "mrs_cco_simu_ch.h"
#include "mrs_dfx.h"
#include "mrs_cco_power_failure.h"
#include "mrs_common_capture.h"


#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
#ifdef __cplusplus
extern "C" {
#endif


/* DBK command table */
#define MRS_DIAG_CCO_CMD_OBJS_TBL                                                          \
    {                                                                                      \
        { ID_DIAG_CMD_CSIM_PACKET,       ID_DIAG_CMD_CSIM_PACKET,      mrs_cco_simu_ch_rx },          \
            { ID_DIAG_CMD_SIMU_CTRL,         ID_DIAG_CMD_SIMU_CTRL,        mrs_cco_simu_ch_ctrl },        \
            { ID_DIAG_CMD_UART_MSG_SEND_INF, ID_DIAG_CMD_PLC_MSG_PROC_INF, mrs_diag_dfx }, \
            { ID_DIAG_CMD_TF_CTRL,           ID_DIAG_CMD_TF_RESULT_QUERY,  mrs_dfx_tf },              \
            { ID_DIAG_CMD_PWF_QUERY,         ID_DIAG_CMD_PWF_QUERY,        mrs_cmd_pwf },                 \
        { ID_DIAG_CMD_PHY_TEST_INFO, ID_DIAG_CMD_PHY_TEST_INFO, mrs_cmd_query_test_mode_info }, \
    }

#define MRS_DIAG_CMD_OBJS_TBL MRS_DIAG_CCO_CMD_OBJS_TBL

#ifdef __cplusplus
}
#endif
#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_CCO) */
#endif /* __MRS_CCO_DIAG_H__ */

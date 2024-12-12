/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS CCO Transfomer Identify handle.
 */

#ifndef __MRS_CCO_TF_H__
#define __MRS_CCO_TF_H__

#include "soc_sal.h"
#include "dfx_app.h"
#include "mrs_cco_plc.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)

#define MRS_CCO_TF_REPORT_RETRY_MAX       (MRS_CCO_REPORT_RETRY_MAX)
#define MRS_CCO_TF_REPORT_TIMEOUT  (MRS_CCO_REPORT_TIMEOUT)

#define IDENTIFY_DURATION_DEFAULT 120 /* Default duration of transformer district identification, in minute */
#define REPORT_DURATION_DEFAULT   60  /* Default duration for reporting  identification results, in minute */
#define START_CMD_SEND_INTERVAL   180 /* Interval for sending the command for starting the transformer district
                                          identification, in s */
#define REFRESH_STA_LIST_INTERVAL 120  /* Interval for refreshing the STA list, in s */
#define DELAY_WHITE_LIST_DEFAULT  1200 /* Duration for waiting for the whitelist to be enabled after the transformer
                                          district identification is complete. Unit: s */
#define QUERY_STA_INTERVAL     2  /* Interval for identifying consecutive query results, in s */
#define SEND_STOP_CMD_INTERVAL 60 /* Interval for sending the command for stopping the slave node registration.
                                          Unit: s */
#define QUERY_STA_TIMEOUT      12 /* The waiting times out when the STA identification result is queried. s */
#define QUERY_STA_MAX_WAY      3  /* Query the number of Stas concurrently. */
#define QUERY_MAX_CNT          50
#define IDENTIFY_DURATION_RATE 67 /* Percentage of the area identification duration to the total timeout period
                                          (67%) */

td_u32 mrs_cco_start_tf_identify(td_u32 duration);
td_void mrs_cco_query_tf_identify(td_void);
td_void mrs_cco_refresh_sta_tf_record(td_void);
td_void mrs_cco_send_tf_start_cmd(td_void);
td_void mrs_cco_send_tf_stop_cmd(td_void);
td_void mrs_cco_query_tf_proc(td_void);
td_void mrs_cco_tf_finished(td_void);
td_void mrs_cco_tf_query_timeout(td_void);
td_void mrs_cco_stop_tf_identify(td_void);
td_u32 mrs_dfx_tf_ctrl(diag_cmd_tf_ctrl_req *req, td_u16 cmd_size, td_u8 option);
td_u32 mrs_dfx_tf_result_query(const diag_cmd_tf_result_req *req, td_u16 cmd_size, td_u8 option);

#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_CCO) */
#endif /* __MRS_CCO_PLC_H__ */


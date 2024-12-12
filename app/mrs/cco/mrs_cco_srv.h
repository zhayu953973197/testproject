/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS cco service.
 */

#ifndef MRS_CCO_SERVICE_H__
#define MRS_CCO_SERVICE_H__

#include "mrs_cco_mr_manage.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
#ifdef __cplusplus
extern "C" {
#endif


#define MRS_CCO_STATUS_IDLE         (0x00)
#define MRS_CCO_STATUS_SEARCH_METER (0x01)
#define MRS_CCO_STATUS_UPGRADE      (0x02)

typedef struct {
    mr_task_list_head mr_list_head;
    mrs_mr_pool mr_pool;

    td_u8 simu_ch_status;
    td_u8 seq;
    td_u8 main_node[EXT_METER_ADDR_LEN];

    td_u8 status;       /* working status: 0-idle; 0x01-search meter; 0x02-upgrade */
    td_bool chl_status; /* PLC channel status */
    td_u16 plc_seq;
} mrs_cco_srv_ctx;

typedef td_void (*mrs_msg_timer_func)(td_void);
typedef struct {
    mrs_timer_id id;
    mrs_msg_timer_func func;
} mrs_msg_timer_func_map;

mrs_cco_srv_ctx *mrs_cco_get_srv_ctx(td_void);
td_u32 mrs_cco_srv_init(td_void);
td_void mrs_cco_simu_ctrl(td_bool enable);
td_bool mrs_cco_simu_status(td_void);
td_void mrs_cco_setup_whitelist(td_void);

#define mrs_srv_init() mrs_cco_srv_init()

#ifdef __cplusplus
}
#endif

#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_CCO) */
#endif /* __MRS_CCO_SERVICE_H__ */


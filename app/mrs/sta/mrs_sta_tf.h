/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS sta tf handle.
 */

#ifndef __MRS_STA_TF_H__
#define __MRS_STA_TF_H__

#include "mrs_sta_plc.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)
td_void mrs_sta_tf_cycle_check(td_void);
td_void mrs_sta_tf_meter_change(td_void);
#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_STA) */

#ifdef __cplusplus
}
#endif

#endif /* __MRS_STA_TF_H__ */

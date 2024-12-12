/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: Simulate meter service handler.
 */

#ifndef __MRS_STA_SIMU_METER_H__
#define __MRS_STA_SIMU_METER_H__

#include <soc_types.h>

#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)
#ifdef __cplusplus
extern "C" {
#endif


/* enable simulator meter */
td_void mrs_simu_meter_enable(td_bool enable);
/* simulator meter enable status */
td_bool mrs_simu_meter_flag(td_void);
/* processing for simulator meter NV changed */
td_void mrs_simu_meter_nv_detect(td_void);

#ifdef __cplusplus
}
#endif

#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_STA) */
#endif /* __MRS_STA_SIMU_METER_H__ */

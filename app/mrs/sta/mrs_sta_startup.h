/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS STA startup handle.
 */

#ifndef __MRS_STA_STARTUP_H__
#define __MRS_STA_STARTUP_H__

#include "soc_types.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)
#ifdef __cplusplus
extern "C" {
#endif

/* meter detection while STA power on */
td_void mrs_sta_startup_detect(td_void);
#ifdef __cplusplus
}
#endif

#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_STA) */
#endif /* __MRS_STA_STARTUP_H__ */


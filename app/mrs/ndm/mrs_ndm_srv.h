/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS NDM service.
 */

#ifndef MRS_NDM_SERVICE_H
#define MRS_NDM_SERVICE_H

#if defined(PRODUCT_CFG_PRODUCT_TYPE_NDM)
#include "soc_mdm.h"

#ifdef __cplusplus
extern "C" {
#endif


td_u32 mrs_ndm_srv_init(td_void);

#define mrs_srv_init() mrs_ndm_srv_init()

#ifdef __cplusplus
}
#endif

#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_NDM) */
#endif /* __MRS_NDM_SERVICE_H__ */

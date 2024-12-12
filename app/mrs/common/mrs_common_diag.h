/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS diag cmd handle.
 */

#ifndef __MRS_COMMON_DIAG_H__
#define __MRS_COMMON_DIAG_H__

#include <soc_types.h>

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
#include "mrs_cco_diag.h"
#endif

#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)
#include "mrs_sta_diag.h"
#endif

#if defined(PRODUCT_CFG_PRODUCT_TYPE_NDM)
#include "mrs_ndm_diag.h"
#endif

td_void mrs_diag_init(td_void);

#endif /* __MRS_COMMON_DIAG_H__ */

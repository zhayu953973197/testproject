/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS sta service (user).
 */

#ifndef __MRS_STA_USR_H__
#define __MRS_STA_USR_H__

#include "mrs_config.h"

#if (defined(PRODUCT_CFG_PRODUCT_TYPE_STA) && defined(MRS_USR_DEMO))
#ifdef __cplusplus
extern "C" {
#endif


#define MRS_STA_USR_STARTUP_TIME 1000
#define MRS_STA_USR_POLLING_TIME 1000

td_void mrs_proto_usr_init(td_void);
td_u32 mrs_sta_usr_srv_init(td_void);

#ifdef __cplusplus
}
#endif

#endif /* (defined(PRODUCT_CFG_PRODUCT_TYPE_STA) && defined(MRS_USR_DEMO)) */
#endif /* __MRS_STA_USR_H__ */

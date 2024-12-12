/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS CCO simulation channel.
 */

#ifndef __MRS_CCO_SIMU_CH_H__
#define __MRS_CCO_SIMU_CH_H__

#include "soc_types.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
#ifdef __cplusplus
extern "C" {
#endif

td_u32 mrs_cco_simu_ch_ctrl(td_u16 id, td_pvoid cmd, td_u16 cmd_size, td_u8 option);
td_u32 mrs_cco_simu_ch_rx(td_u16 id, td_pvoid cmd, td_u16 cmd_size, td_u8 option);

#ifdef __cplusplus
}
#endif

#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_CCO) */
#endif /* __MRS_CCO_SIMU_CH_H__ */


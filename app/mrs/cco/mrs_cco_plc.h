/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS CCO plc handle.
 */

#ifndef __MRS_CCO_PLC_H__
#define __MRS_CCO_PLC_H__

#include "mrs_common_plc.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
#ifdef __cplusplus
extern "C" {
#endif


/* processing for recieving transparent PLC frame */
td_void mrs_cco_xr_plc_frame_rx(mrs_plc_frame_data *plc);

/* processing for recieving parallel PLC frame */
td_void mrs_cco_pr_plc_frame_rx(mrs_plc_frame_data *plc);

/* CCO receive PLC event frame handler */
td_void mrs_cco_evt_plc_frame_rx(mrs_plc_frame_data *plc);

td_void mrs_cco_tf_plc_frame_rx(mrs_plc_frame_data *plc);

/* query station info */
td_void mrs_cco_qry_plc_frame_rx(mrs_plc_frame_data *plc);

/* CCO equip test frame */
td_void mrs_cco_dut_plc_frame_rx(mrs_plc_frame_data *plc);

#define mrs_xr_plc_frame_rx(plc) mrs_cco_xr_plc_frame_rx(plc)
#define mrs_pr_plc_frame_rx(plc) mrs_cco_pr_plc_frame_rx(plc)
#define mrs_bc_plc_frame_rx(plc)
#define mrs_evt_plc_frame_rx(plc) mrs_cco_evt_plc_frame_rx(plc)
#define mrs_tf_plc_frame_rx(plc)  mrs_cco_tf_plc_frame_rx(plc)
#define mrs_ciu_plc_frame_rx(plc)
#define mrs_qry_plc_frame_rx(plc) mrs_cco_qry_plc_frame_rx(plc)
#define mrs_sta_dut_plc_frame_rx(plc)

#ifdef __cplusplus
}
#endif
#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_CCO) */
#endif /* __MRS_CCO_PLC_H__ */


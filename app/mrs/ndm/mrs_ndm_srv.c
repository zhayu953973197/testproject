/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS NDM service.
 */

#include "mrs_ndm_srv.h"
#include "mrs_msg.h"
#include "mrs_proto_645.h"
#include "mrs_ndm_plc.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_NDM)
#ifdef __cplusplus
extern "C" {
#endif

td_u32 mrs_ndm_srv_init(td_void)
{
    mrs_proto_645_init();
    mrs_ndm_dut_init();
    return EXT_ERR_SUCCESS;
}

td_void mrs_msg_on_timer(EXT_CONST mrs_queue_msg *msg)
{
    switch (msg->param1) {
        case MRS_TIMER_ID_UART:
            mrs_proto_645_on_timer();
            break;
        default:
            break;
    }
}

#ifdef __cplusplus
}
#endif
#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_NDM) */

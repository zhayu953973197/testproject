/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS message structure.
 */

#ifndef MRS_MESSAGE_H
#define MRS_MESSAGE_H

#include <soc_types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    td_u16 id;
    td_u16 param0;
    uintptr_t param1;
    uintptr_t param2;
} mrs_queue_msg;

typedef enum {
    MRS_MSG_ID_TIMER,
    MRS_MSG_ID_DIAG,
    MRS_MSG_ID_PLC_RX,
    MRS_MSG_ID_PLC_TX,
    MRS_MSG_ID_PLC_CHL,
    MRS_MSG_ID_APP_FRAME_RX_13762,
    MRS_MSG_ID_APP_FRAME_RX_645,
    MRS_MSG_ID_APP_FRAME_RX_DLMS,
    MRS_MSG_ID_REMOTE_UPG,
    MRS_MSG_ID_MR_NOTIFY,
    MRS_MSG_ID_EVT_NOTIFY,
    MRS_MSG_ID_QUEUE_NOTIFY,
    MRS_MSG_ID_NV_CHANGE_NOTIFY,
    MRS_MSG_ID_MAX
} mrs_msg_id;

typedef td_void (*mrs_msg_handler_fct)(EXT_CONST mrs_queue_msg *);

td_void mrs_msg_on_timer(EXT_CONST mrs_queue_msg *msg);
td_void mrs_msg_on_diag(EXT_CONST mrs_queue_msg *msg);
td_void mrs_msg_on_plc_rx(EXT_CONST mrs_queue_msg *msg);
td_void mrs_msg_on_plc_tx(EXT_CONST mrs_queue_msg *msg);
td_void mrs_msg_on_plc_chl(EXT_CONST mrs_queue_msg *msg);
td_void mrs_msg_on_app_frame_rx_13762(EXT_CONST mrs_queue_msg *msg);
td_void mrs_msg_on_app_frame_rx_645(EXT_CONST mrs_queue_msg *msg);
td_void mrs_msg_on_app_frame_rx_dlms(EXT_CONST mrs_queue_msg *msg);
td_void mrs_msg_on_remote_upg(EXT_CONST mrs_queue_msg *msg);
td_void mrs_msg_on_mr_notify(EXT_CONST mrs_queue_msg *msg);
td_void mrs_msg_on_evt_notify(EXT_CONST mrs_queue_msg *msg);
td_void mrs_msg_on_queue_notify(EXT_CONST mrs_queue_msg *msg);
td_void mrs_msg_on_nv_change_notify(EXT_CONST mrs_queue_msg *msg);

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
/* message processing function list:  correspondence with message ID order */
#define MRS_MSG_HANDLER_MAP                \
    {                                      \
        mrs_msg_on_timer,                  \
            mrs_msg_on_diag,               \
            mrs_msg_on_plc_rx,             \
            mrs_msg_on_plc_tx,             \
            mrs_msg_on_plc_chl,            \
            mrs_msg_on_app_frame_rx_13762, \
            TD_NULL,                       \
            TD_NULL,                       \
            mrs_msg_on_remote_upg,         \
            mrs_msg_on_mr_notify,          \
            mrs_msg_on_evt_notify,         \
            mrs_msg_on_queue_notify,       \
            TD_NULL,                       \
    }
#endif

#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)
/* message processing function list:  correspondence with message ID order */
#define MRS_MSG_HANDLER_MAP              \
    {                                    \
        mrs_msg_on_timer,                \
            mrs_msg_on_diag,               \
            mrs_msg_on_plc_rx,           \
            mrs_msg_on_plc_tx,           \
            mrs_msg_on_plc_chl,          \
            TD_NULL,                     \
            mrs_msg_on_app_frame_rx_645, \
            TD_NULL,                     \
            TD_NULL,                     \
            TD_NULL,                     \
            TD_NULL,                     \
            TD_NULL,                     \
            mrs_msg_on_nv_change_notify, \
    }
#endif

#if defined(PRODUCT_CFG_PRODUCT_TYPE_NDM)
/* message processing function list:  correspondence with message ID order */
#define MRS_MSG_HANDLER_MAP              \
    {                                    \
        mrs_msg_on_timer,                \
            TD_NULL,                     \
            mrs_msg_on_plc_rx,           \
            mrs_msg_on_plc_tx,           \
            TD_NULL,  \
            TD_NULL,                     \
            mrs_msg_on_app_frame_rx_645, \
            TD_NULL,                     \
            TD_NULL,                     \
            TD_NULL,                     \
            TD_NULL,                     \
            TD_NULL,                     \
            TD_NULL,                     \
    }
#endif

#ifdef __cplusplus
}
#endif

#endif /* __MRS_MESSAGE_H__ */


/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: CCO broadcast implementation.
 */

#include "mrs_cco_broadcast.h"
#include "mrs_common_tools.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
#define MRS_CCO_BC_DL_VER 1

#define MRS_CCO_BC_STAT_IDLE 0
#define MRS_CCO_BC_STAT_BUSY 1

#define MRS_CCO_BC_TIMER_INTERVAL 1
#define MRS_CCO_BC_RESEND_MAX     5

typedef struct {
    td_u8 state;                   /* < Global context state */
    td_u8 retry;                   /* < Resend times */
    td_u16 interval;               /* < Resend polling period */
    mrs_broadcast_frame *bc_frame; /* < Resend data */
} mrs_bc_ctx;

/* Init global context */
td_u32 mrs_cco_broadcast_init(td_void);
/* Create broadcast frame for PLC sending */
td_u32 mrs_bc_create_frame(const td_u8 *msg, td_u16 msg_len);
/* Put frame to sending ready queue */
td_u32 mrs_bc_send_frame(td_void);
/* Reset broadcast parameters and free memory */
td_void mrs_cco_broadcast_reset(td_void);

mrs_bc_ctx *g_bc_ctx = TD_NULL;

#define mrs_cco_broadcast_get_ctx() (mrs_bc_ctx *)(g_bc_ctx)

td_u32 mrs_cco_broadcast_init(td_void)
{
    if (g_bc_ctx == TD_NULL) {
        g_bc_ctx = (mrs_bc_ctx *)mrs_malloc(sizeof(mrs_bc_ctx));
        if (g_bc_ctx == TD_NULL) {
            return EXT_ERR_MALLOC_FAILUE;
        }

        g_bc_ctx->interval = MRS_CCO_BC_TIMER_INTERVAL;
        g_bc_ctx->retry = 0;
        g_bc_ctx->bc_frame = TD_NULL;
        g_bc_ctx->state = MRS_CCO_BC_STAT_IDLE;
    }

    return EXT_ERR_SUCCESS;
}

td_u32 mrs_cco_broadcast_proc(td_u8 *deny_code, const td_pbyte content, td_u16 content_len)
{
    td_u32 ret = EXT_ERR_SUCCESS;
    mrs_bc_ctx *ctx = mrs_cco_broadcast_get_ctx();

    if (content_len == 0) {
        return ret;
    }

    if (ctx == TD_NULL) {
        ret = mrs_cco_broadcast_init();
        if (ret != EXT_ERR_SUCCESS) {
            *deny_code = MRS_1376_2_DENY_TIMEOUT;
            return EXT_ERR_MALLOC_FAILUE;
        }

        ctx = mrs_cco_broadcast_get_ctx();
    }

    /* Disallow multiable broadcast frames */
    if (ctx->state != MRS_CCO_BC_STAT_IDLE) {
        *deny_code = MRS_1376_2_DENY_CCO_BUSY;
        return EXT_ERR_BUSY;
    }

    /* Mark flag busy */
    ctx->state = MRS_CCO_BC_STAT_BUSY;

    ret = mrs_bc_create_frame(content, content_len);
    if (ret != EXT_ERR_SUCCESS) {
        *deny_code = MRS_1376_2_DENY_TIMEOUT;
        mrs_cco_broadcast_reset();
    } else {
        /* Move to PLC ready queue */
        mrs_bc_send_frame();
    }

    return ret;
}

td_u32 mrs_bc_create_frame(const td_u8 *msg, td_u16 msg_len)
{
    td_u32 frame_len = sizeof(mrs_broadcast_frame) + msg_len;
    mrs_bc_ctx *ctx = mrs_cco_broadcast_get_ctx();
    mrs_broadcast_frame *frame = TD_NULL;

    if ((msg == TD_NULL) || (msg_len == 0)) {
        return EXT_ERR_INVALID_PARAMETER;
    }

    frame = (mrs_broadcast_frame *)mrs_malloc(frame_len);
    if (frame == TD_NULL) {
        return EXT_ERR_MALLOC_FAILUE;
    }

    /* Set broadcast header */
    (td_void) memset_s(frame, frame_len, 0, frame_len);
    frame->stru_ver = MRS_CCO_BC_DL_VER;
    frame->stru_size = sizeof(mrs_broadcast_frame);
    frame->data_len = msg_len;
    if (memcpy_s(frame->data, frame->data_len, msg, msg_len) != EOK) {
        mrs_free(frame);
        return EXT_ERR_MEMCPY_FAIL;
    }
    /* Store the frame for resending */
    ctx->bc_frame = frame;

    return EXT_ERR_SUCCESS;
}

td_void mrs_cco_broadcast_send_timeout(td_void)
{
    td_u32 ret;
    mrs_bc_ctx *ctx = mrs_cco_broadcast_get_ctx();

    if ((ctx->retry >= MRS_CCO_BC_RESEND_MAX) || (ctx->bc_frame == TD_NULL)) {
        mrs_cco_broadcast_reset();
        return;
    }

    /* Retry */
    ctx->retry++;
    ret = mrs_bc_send_frame();
    if (ret != EXT_ERR_SUCCESS) {
        mrs_timer_start(MRS_TIMER_ID_BROADCAST_RESEND, ctx->interval * 1000, EXT_TIMER_TYPE_ONCE); /* time* 1000 */
    } else {
        mrs_cco_broadcast_reset();
    }
}

td_void mrs_cco_broadcast_reset(td_void)
{
    if (g_bc_ctx->bc_frame != TD_NULL) {
        mrs_free(g_bc_ctx->bc_frame);
        g_bc_ctx->bc_frame = TD_NULL;
    }

    mrs_free(g_bc_ctx);
    g_bc_ctx = TD_NULL;

    mrs_timer_stop(MRS_TIMER_ID_BROADCAST_RESEND);
}

td_u32 mrs_bc_send_frame(td_void)
{
    mrs_bc_ctx *ctx = mrs_cco_broadcast_get_ctx();
    mrs_broadcast_frame *bc_frame = ctx->bc_frame;
    mrs_plc_frame_data plc;
    td_u32 ret;

    if (bc_frame == TD_NULL) {
        mrs_cco_broadcast_reset();
        return EXT_ERR_INVALID_PARAMETER;
    }

    /* Set PLC header */
    (td_void) memset_s(&plc, sizeof(mrs_plc_frame_data), 0, sizeof(mrs_plc_frame_data));
    plc.id = PLC_CMD_ID_BC;
    plc.payload_len = bc_frame->stru_size + bc_frame->data_len;
    plc.payload = (td_u8 *)bc_frame;
    (td_void) memset_s(plc.addr, MRS_METER_ADDR_LEN, 0xff, MRS_METER_ADDR_LEN);

    ret = mrs_plc_frame_send(&plc);
    if (ret != EXT_ERR_SUCCESS) {
        mrs_timer_start(MRS_TIMER_ID_BROADCAST_RESEND, ctx->interval * 1000, EXT_TIMER_TYPE_ONCE); /* time *1000 */
        return ret;
    }

    mrs_cco_broadcast_reset();

    return EXT_ERR_SUCCESS;
}

#endif /* defined PRODUCT_CFG_PRODUCT_TYPE_CCO */

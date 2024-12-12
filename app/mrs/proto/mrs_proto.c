/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS UART protocol.
 */

#include "mrs_proto.h"
#include "mrs_common_uart.h"
#include "mrs_common_tools.h"
#ifdef __cplusplus
extern "C" {
#endif

mrs_proto_ctx *g_proto_ctx = TD_NULL;

td_u32 mrs_proto_init(td_void)
{
    if (g_proto_ctx != TD_NULL) {
        return EXT_ERR_INITILIZATION_ALREADY;
    }

    g_proto_ctx = (mrs_proto_ctx *)mrs_malloc(sizeof(mrs_proto_ctx));
    if (g_proto_ctx == TD_NULL) {
        return EXT_ERR_MALLOC_FAILUE;
    }

    (td_void) memset_s(g_proto_ctx, sizeof(mrs_proto_ctx), 0, sizeof(mrs_proto_ctx));

    mrs_uart_register_rx_callback(mrs_proto_rx);

    return EXT_ERR_SUCCESS;
}

td_void mrs_proto_rx(td_u8 ch)
{
    mrs_proto_ctx *proto_ctx = g_proto_ctx;
    mrs_proto_buffer *buf = TD_NULL;

    buf = &proto_ctx->buf;
    if (buf->rx_len < sizeof(buf->buffer)) {
        mrs_semaphore_wait(MRS_SEM_ID_UART);
        buf->buffer[buf->rx_len++] = ch;
        mrs_semaphore_signal(MRS_SEM_ID_UART);
    }

    if (buf->rx_len == sizeof(buf->buffer)) {
        if (proto_ctx->handler.rx_full != TD_NULL) {
            proto_ctx->handler.rx_full(buf);
        }
    }

    if (proto_ctx->handler.rx_notify != TD_NULL) {
        proto_ctx->handler.rx_notify(buf);
    }
}

td_void mrs_proto_clear_buffer(td_void)
{
    mrs_proto_ctx *proto_ctx = g_proto_ctx;

    mrs_semaphore_wait(MRS_SEM_ID_UART);
    proto_ctx->buf.rx_len = 0;
    mrs_semaphore_signal(MRS_SEM_ID_UART);
}

td_void mrs_proto_register_handler(const mrs_proto_handler *handler)
{
    mrs_proto_ctx *proto_ctx = g_proto_ctx;

    (td_void) memcpy_s(&proto_ctx->handler, sizeof(proto_ctx->handler), handler, sizeof(mrs_proto_handler));
}

td_void mrs_proto_parse(td_bool overtime)
{
    mrs_proto_ctx *proto_ctx = g_proto_ctx;

    if (proto_ctx->handler.parse != TD_NULL) {
        mrs_semaphore_wait(MRS_SEM_ID_UART);
        mrs_logbuf("[Rx  ]: ", proto_ctx->buf.buffer, (td_u16)proto_ctx->buf.rx_len);
        proto_ctx->handler.parse(&proto_ctx->buf);
        if (overtime == TD_TRUE) {
            proto_ctx->buf.rx_len = 0;
        }
        mrs_semaphore_signal(MRS_SEM_ID_UART);
    }
}

td_void mrs_proto_simu_ctrl(td_bool enable)
{
    mrs_proto_ctx *proto_ctx = g_proto_ctx;

    if (enable == proto_ctx->buf.simu_enable) {
        return;
    }

    mrs_proto_clear_buffer();
    proto_ctx->buf.simu_enable = enable;
}

#ifdef __cplusplus
}
#endif


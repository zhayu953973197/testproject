/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: tools.
 */

#include "mrs_common_tools.h"
#include "mrs_common_main.h"

#ifdef __cplusplus
extern "C" {
#endif

td_void mrs_timer_callback(uintptr_t param);

__hot td_void mrs_timer_callback(uintptr_t param)
{
    if (param < MRS_TIMER_ID_MAX) {
        mrs_msg_queue_send(MRS_MSG_ID_TIMER, 0, param, 0);
    }
}

__hot td_u32 mrs_timer_start(mrs_timer_id timer_id, td_u32 timeout, ext_timer_type_e option)
{
    mrs_main_srv_context *ctx = mrs_get_main_srv_ctx();

    if (ctx == TD_NULL) {
        return EXT_ERR_NO_INITILIZATION;
    }

    if (timer_id >= MRS_TIMER_ID_MAX) {
        return EXT_ERR_INVALID_PARAMETER;
    }

    return uapi_timer_start(ctx->timer_handle + timer_id, mrs_timer_callback, timeout, option, timer_id);
}

__hot td_u32 mrs_timer_stop(mrs_timer_id timer_id)
{
    mrs_main_srv_context *ctx = mrs_get_main_srv_ctx();

    if (ctx == TD_NULL) {
        return EXT_ERR_NO_INITILIZATION;
    }

    if (timer_id >= MRS_TIMER_ID_MAX) {
        return EXT_ERR_INVALID_PARAMETER;
    }

    return uapi_timer_stop(ctx->timer_handle + timer_id);
}

__hot td_u32 mrs_msg_queue_send(td_u16 id, td_u16 param0, uintptr_t param1, uintptr_t param2)
{
    mrs_main_srv_context *ctx = mrs_get_main_srv_ctx();
    mrs_queue_msg msg;

    if (ctx == TD_NULL) {
        return EXT_ERR_NO_INITILIZATION;
    }
    msg.id = id;
    msg.param0 = param0;
    msg.param1 = param1;
    msg.param2 = param2;

    return uapi_msg_queue_send(ctx->queue_id, (td_pvoid)&msg, 0, sizeof(mrs_queue_msg));
}

td_u32 mrs_semaphore_wait(mrs_sem_id sem_id)
{
    mrs_main_srv_context *ctx = mrs_get_main_srv_ctx();

    if (ctx == TD_NULL) {
        return EXT_ERR_NO_INITILIZATION;
    }

    if (sem_id >= MRS_SEM_ID_MAX) {
        return EXT_ERR_INVALID_PARAMETER;
    }

    return uapi_sem_wait(ctx->sem_id[sem_id], EXT_SYS_WAIT_FOREVER);
}

td_u32 mrs_semaphore_signal(mrs_sem_id sem_id)
{
    mrs_main_srv_context *ctx = mrs_get_main_srv_ctx();

    if (ctx == TD_NULL) {
        return EXT_ERR_NO_INITILIZATION;
    }

    if (sem_id >= MRS_SEM_ID_MAX) {
        return EXT_ERR_INVALID_PARAMETER;
    }

    return uapi_sem_signal(ctx->sem_id[sem_id]);
}

td_pbyte mrs_find_byte_in_stream(const td_pbyte buffer, td_u16 length, td_u8 ch)
{
    td_u16 i;

    if (buffer == TD_NULL) {
        return TD_NULL;
    }

    for (i = 0; i < length; i++) {
        if (buffer[i] == ch) {
            return buffer + i;
        }
    }

    return TD_NULL;
}

td_bool mrs_check_bcd_stream(const td_pbyte stream, td_u16 length)
{
    td_u16 i;

    if (stream == TD_NULL) {
        return TD_FALSE;
    }

    for (i = 0; i < length; i++) {
        if (((stream[i] & 0x0f) > 0x09) || ((stream[i] & 0xf0) > 0x90)) {
            return TD_FALSE;
        }
    }

    return TD_TRUE;
}

td_u8 mrs_checksum8(const td_pbyte buffer, td_u16 length)
{
    td_u16 i;
    td_u8 cs = 0;

    if (buffer == TD_NULL) {
        return 0;
    }

    for (i = 0; i < length; i++) {
        cs += buffer[i];
    }

    return cs;
}

#if defined(MRS_LOG_DEBUG)
td_void mrs_print_buffer(const char *hint, td_pbyte buffer, td_u16 length)
{
    td_u16 i;
    ext_unref_param(buffer);
    ext_unref_param(hint);
    mrs_printf(hint);

    for (i = 0; i < length; i++) {
        mrs_printf(" %02X", buffer[i]);
    }

    mrs_printf("\n");
}
#endif

td_void mrs_hex_invert(td_pbyte buffer, td_u16 length)
{
    td_u8 temp;
    td_u16 i;

    for (i = 0; i < length / 2; i++) { /* 2: half-and-half inversion */
        temp = buffer[i];
        buffer[i] = buffer[length - i - 1];
        buffer[length - i - 1] = temp;
    }
}

td_u32 mrs_convert_int_to_bcd(td_u32 data, td_u8 *bcd, td_u8 length)
{
    td_u8 i;
    td_u32 remain = data;
    td_u32 tail;

    if (bcd == TD_NULL) {
        return EXT_ERR_FAILURE;
    }

    for (i = 0; i < length; i++) {
        tail = remain % 100;                          /* 100: tail excepts hundreds */
        remain /= 100;                                /* 100: hundreds */
        bcd[i] = (td_u8)(tail / 10 * 16 + tail % 10); /* 10: tens, 16: hex */
    }

    return EXT_ERR_SUCCESS;
}

td_u32 mrs_time_sub_duration(td_u32 time, td_u32 duration)
{
    if (time >= duration) {
        return (time - duration);
    } else {
        return (0xFFFFFFFF - duration + time + 1);
    }
}

#ifdef __cplusplus
}
#endif


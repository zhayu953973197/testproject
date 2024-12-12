/*
 * Copyright (c) CompanyNameMagicTag 2018-2020. All rights reserved.
 * Description: APP DL/T645 protocol base API.
 */

#include "app_proto_645.h"
#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)
#include "app_main.h"
#include "app_common.h"

td_pbyte app_tools_find_byte(td_pbyte buffer, td_u16 length, td_u8 ch)
{
    td_u16 i;

    if (buffer != TD_NULL) {
        for (i = 0; i < length; i++) {
            if (ch == buffer[i]) {
                return buffer + i;
            }
        }
    }

    return TD_NULL;
}

td_bool app_check_frame_645(td_pbyte frame, td_u16 length)
{
    td_u8 cs;

    if ((frame == TD_NULL) ||
        (length > PROTO_645_DATAGRAM_LEN_MAX) ||
        (length < PROTO_645_DATAGRAM_LEN_MIN)) {
        return TD_FALSE;
    }

    if ((frame[0] != PROTO_645_HEAD_BYTE) ||
        (frame[PROTO_645_HEAD_BTYE7_OFFSET] != PROTO_645_HEAD_BYTE) ||
        (frame[length - 1] != PROTO_645_TAIL_BYTE)) {
        return TD_FALSE;
    }

    cs = app_tools_checksum8(frame, length - PROTO_645_CHECK_LEN);
    if (cs != frame[length - PROTO_645_CHECK_LEN]) {
        return TD_FALSE;
    }

    return TD_TRUE;
}

td_u32 app_find_frame_645(td_pbyte in_buffer, td_u16 length, td_u16 *start_pos, td_u16 *frame_length)
{
    td_u32 ret = EXT_ERR_SUCCESS;
    td_pbyte buffer = in_buffer;

    if ((in_buffer == TD_NULL) || (start_pos == TD_NULL) || (frame_length == TD_NULL)) {
        return EXT_ERR_INVALID_PARAMETER;
    }

    while (length > 0) {
        td_pbyte head = (td_pbyte)app_tools_find_byte(buffer, length, PROTO_645_HEAD_BYTE);
        td_u16 frm_len;

        if (head == TD_NULL) {
            ret = EXT_ERR_BAD_DATA;
            break;
        }

        length -= (td_u16)(head - buffer);
        buffer = head;

        if (length < PROTO_645_DATAGRAM_LEN_MIN) {
            ret = EXT_ERR_BAD_DATA;
            break;
        }

        if (buffer[PROTO_645_DATAGRAM_DATA_LEN_OFFSET] > PROTO_645_DATAGRAM_DATA_LEN_MAX) {
            buffer++;
            length--;
            continue;
        }

        frm_len = (td_u16)(buffer[PROTO_645_DATAGRAM_DATA_LEN_OFFSET] + PROTO_645_DATAGRAM_LEN_MIN);
        if ((length >= frm_len) && (app_check_frame_645(buffer, frm_len) == TD_TRUE)) {
            *start_pos = (td_u16)(buffer - in_buffer);
            *frame_length = frm_len;
            break;
        }

        buffer++;
        length--;
    }

    return ret;
}

td_void app_frame_645_encode(td_pbyte buffer, td_u16 length)
{
    td_u16 i;

    if (buffer == TD_NULL) {
        return;
    }
    for (i = 0; i < length; i++) {
        buffer[i] += 0x33;
    }
}

td_void app_frame_645_decode(td_pbyte buffer, td_u16 length)
{
    td_u16 i;

    if (buffer == TD_NULL) {
        return;
    }
    for (i = 0; i < length; i++) {
        buffer[i] -= 0x33;
    }
}

#endif

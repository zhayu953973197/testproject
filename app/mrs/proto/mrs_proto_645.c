/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS protocol
 */

#include "mrs_proto_645.h"
#include "mrs_common_tools.h"
#include "mrs_dfx.h"

#define MRS_645_HEAD_BYTE 0x68
#define MRS_645_TAIL_BYTE 0x16

static td_void mrs_proto_645_rx_notify(EXT_CONST mrs_proto_buffer *buf);
static td_void mrs_proto_645_parse(mrs_proto_buffer *buf);

td_u32 mrs_proto_645_find_frame(const td_pbyte buffer, const td_u16 length, td_u16 *start_pos, td_u16 *frm_len)
{
    td_u16 in_len = length;
    td_pbyte in_buf = buffer;

    if (in_buf == TD_NULL) {
        return EXT_ERR_BAD_DATA;
    }

    while (in_len > 0) {
        td_u16 frame_length;
        td_pbyte head = mrs_find_byte_in_stream(in_buf, (td_u16)in_len, MRS_645_HEAD_BYTE);
        if (head == TD_NULL) {
            return EXT_ERR_BAD_DATA;
        }

        in_len -= (td_u16)(head - in_buf);
        in_buf = head;
        if (in_len < MRS_645_FRAME_LEN_MIN) {
            return EXT_ERR_BAD_DATA;
        }

        frame_length = (td_u16)in_buf[9] + MRS_645_FRAME_LEN_MIN; /* index 9: data length */
        if ((frame_length <= MRS_645_FRAME_LEN_MAX) &&
            (in_len >= frame_length) &&
            (mrs_proto_645_check(in_buf, frame_length) == TD_TRUE)) {
            *start_pos = (td_u16)(in_buf - buffer);
            *frm_len = frame_length;
            return EXT_ERR_SUCCESS;
        }

        in_buf++;
        in_len--;
    }

    return EXT_ERR_BAD_DATA;
}

td_bool mrs_proto_645_check(const td_pbyte buffer, const td_u16 length)
{
    td_u8 cs;

    if ((buffer == TD_NULL) ||
        (length < MRS_645_FRAME_LEN_MIN) ||
        (length > MRS_645_FRAME_LEN_MAX)) {
        return TD_FALSE;
    }

    if ((buffer[0] != MRS_645_HEAD_BYTE) ||
        (buffer[7] != MRS_645_HEAD_BYTE) || /* index 7: head byte */
        (buffer[length - 1] != MRS_645_TAIL_BYTE)) {
        return TD_FALSE;
    }

    cs = mrs_checksum8(buffer, length - 2); /* check sum excepts 2bytes */
    if (cs != buffer[length - 2]) {         /* check sum excepts 2bytes */
        return TD_FALSE;
    }

    return TD_TRUE;
}

td_u32 mrs_proto_645_init(td_void)
{
    mrs_proto_handler handler = { mrs_proto_645_rx_notify, (mrs_proto_rx_full)TD_NULL, mrs_proto_645_parse };
    mrs_proto_register_handler(&handler);

    uapi_register_judge_is_rcv_p2p_pkt_by_dest_mac(mrs_proto_is_rcv_p2p_pkt_by_dest_mac);
    return EXT_ERR_SUCCESS;
}

static td_void mrs_proto_645_rx_notify(EXT_CONST mrs_proto_buffer *buf)
{
    ext_unref_param(buf);
    mrs_timer_start(MRS_TIMER_ID_UART, MRS_UART_FRAME_INTERVAL, EXT_TIMER_TYPE_ONCE);
}

static td_void mrs_proto_645_parse(mrs_proto_buffer *buf)
{
    td_u16 start_pos = 0;
    td_u16 frm_len = 0;
    td_u32 ret;

    ret = mrs_proto_645_find_frame(buf->buffer, buf->rx_len, &start_pos, &frm_len);
    if (ret == EXT_ERR_SUCCESS) {
        td_u16 payload_len = frm_len;
        td_pbyte payload = mrs_malloc(payload_len);
        if (payload == TD_NULL) {
            return;
        }

        if (memcpy_s(payload, payload_len, buf->buffer + start_pos, frm_len) != EOK) {
            mrs_free(payload);
            return;
        }
        mrs_dfx_uart_chl_rx_frame(frm_len, EXT_ERR_SUCCESS, TD_FALSE);
        ret = mrs_msg_queue_send(MRS_MSG_ID_APP_FRAME_RX_645, frm_len, (uintptr_t)payload, 0);
        if (ret != EXT_ERR_SUCCESS) {
            mrs_dfx_uart_chl_rx_frame(frm_len, ret, TD_FALSE);
            mrs_free(payload);
        }
    }
}

td_void mrs_proto_645_on_timer(td_void)
{
    mrs_proto_parse(TD_TRUE);
}

td_u32 mrs_proto_645_create_frame(const mrs_proto_645_frame_inf *frame_inf, td_u8 **frame, td_u16 *frame_len)
{
    const td_u16 buff_len = MRS_645_FRAME_LEN_MIN + frame_inf->data_len;
    td_u16 offset = 0;
    td_u8 *buff = TD_NULL;

    buff = mrs_malloc(buff_len);
    if (buff == TD_NULL) {
        return EXT_ERR_MALLOC_FAILUE;
    }

    (td_void) memset_s(buff, buff_len, 0, buff_len);

    /* First HEAD byte */
    buff[offset++] = MRS_645_HEAD_BYTE;

    /* ADDR byte */
    if (memcpy_s(buff + offset, buff_len - offset, frame_inf->addr, sizeof(frame_inf->addr)) != EOK) {
        mrs_free(buff);
        return EXT_ERR_BAD_DATA;
    }
    offset += MRS_METER_ADDR_LEN;

    /* Second HEAD byte */
    buff[offset++] = MRS_645_HEAD_BYTE;

    /* CTRL byte */
    buff[offset++] = frame_inf->ctrl;

    /* LEN byte */
    buff[offset++] = frame_inf->data_len;

    /* DATA field */
    if (frame_inf->data_len > 0) {
        if (memcpy_s(buff + offset, buff_len - offset, frame_inf->data, frame_inf->data_len) != EOK) {
            mrs_free(buff);
            return EXT_ERR_FAILURE;
        }
        mrs_proto_645_data_encode(buff + offset, frame_inf->data_len);
        offset += frame_inf->data_len;
    }

    /* checksum */
    buff[offset] = mrs_checksum8(buff, offset);
    offset++;

    buff[offset++] = MRS_645_TAIL_BYTE;

    *frame = buff;
    *frame_len = buff_len;

    return EXT_ERR_SUCCESS;
}

td_u32 mrs_proto_645_decode_frame(const td_pbyte buffer, const td_u16 length, mrs_proto_645_frame_inf *frame)
{
    if (mrs_proto_645_check(buffer, length) != TD_TRUE) {
        return EXT_ERR_BAD_DATA;
    }

    if (memcpy_s(frame->addr, sizeof(frame->addr), buffer + MRS_645_FRAME_METERADD_OFFSET,
                 MRS_METER_ADDR_LEN) != EOK) {
        return EXT_ERR_FAILURE;
    }

    frame->ctrl = buffer[MRS_645_FRAME_CTRL_OFFSET];
    frame->data_len = buffer[MRS_645_FRAME_LEN_OFFSET];
    frame->data = buffer + MRS_645_FRAME_DATA_OFFSET;

    return EXT_ERR_SUCCESS;
}

td_void mrs_proto_645_data_decode(td_u8 *buf, td_u32 len)
{
    while (len--) {
        buf[len] -= MRS_645_FRAME_HEX33;
    }
}

td_void mrs_proto_645_data_encode(td_u8 *buf, td_u32 len)
{
    while (len--) {
        buf[len] += MRS_645_FRAME_HEX33;
    }
}


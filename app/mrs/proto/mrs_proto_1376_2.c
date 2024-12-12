/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS protocol
 */

#include "mrs_proto_1376_2.h"
#include "mrs_common_tools.h"
#include "mrs_dfx.h"
#ifdef __cplusplus
extern "C" {
#endif

#define MRS_13762_HEAD_BYTE 0x68
#define MRS_13762_TAIL_BYTE 0x16

#define MRS_13762_HEAD_SIZE 1
#define MRS_13762_LEN_SIZE  2
#define MRS_13762_CTRL_SIZE 1
#define MRS_13762_R_SIZE    6
#define MRS_13762_AFN_SIZE  1
#define MRS_13762_FN_SIZE   2
#define MRS_13762_CS_SIZE   1
#define MRS_13762_TAIL_SIZE 1

#define MRS_13762_FRAME_MIN_SIZE (MRS_13762_HEAD_SIZE + MRS_13762_LEN_SIZE + MRS_13762_CTRL_SIZE + \
                                  MRS_13762_R_SIZE + MRS_13762_AFN_SIZE + MRS_13762_FN_SIZE +      \
                                  MRS_13762_CS_SIZE + MRS_13762_TAIL_SIZE)

#define MRS_13762_COM_FORM 3
#define MRS_13762_DIR_DN   0
#define MRS_13762_DIR_UP   1

#define MRS_YEAR_BASE 2000

static td_void mrs_proto_1376_2_rx_notify(EXT_CONST mrs_proto_buffer *buf);
static td_void mrs_proto_1376_2_rx_full_notify(EXT_CONST mrs_proto_buffer *buf);
static td_void mrs_proto_1376_2_parse(mrs_proto_buffer *buf);
static td_u8 mrs_proto_1376_2_get_fn(const td_u8 dt[2]); /* 2bytes */

td_u32 mrs_proto_1376_2_init(td_void)
{
    mrs_proto_handler handler = { mrs_proto_1376_2_rx_notify, mrs_proto_1376_2_rx_full_notify, mrs_proto_1376_2_parse };
    mrs_proto_register_handler(&handler);
    return EXT_ERR_SUCCESS;
}

td_bool mrs_proto_1376_2_check(td_pbyte buffer, td_u16 length)
{
    td_u16 frm_len;
    td_u8 cs;

    if ((buffer == TD_NULL) || (length < MRS_13762_FRAME_MIN_SIZE)) {
        return TD_FALSE;
    }

    if ((buffer[0] != MRS_13762_HEAD_BYTE) || (buffer[length - 1] != MRS_13762_TAIL_BYTE)) {
        return TD_FALSE;
    }

    frm_len = uapi_make_u16(buffer[1], buffer[2]); /* index 1&2: length */
    if (frm_len != length) {
        return TD_FALSE;
    }

    cs = mrs_checksum8(buffer + 3, length - 5); /* shifts 3B, length is 5B less than all */
    if (cs != buffer[length - 2]) {             /* check sum shifts 2bytes */
        return TD_FALSE;
    }

    return TD_TRUE;
}

static td_u32 mrs_proto_1376_2_check_sucess_for_parse(mrs_proto_buffer *buf, td_pbyte head,
    td_pbyte *buffer, td_u16 *length)
{
    td_u16 frm_len;
    frm_len = uapi_make_u16(head[1], head[2]); /* index 1&2: length */
    if (frm_len > sizeof(buf->buffer)) {
        (*buffer)++;
        (*length)--;
        return EXT_ERR_CONTINUE;
    }

    if (*length < frm_len) {
        if (memmove_s(buf->buffer, sizeof(buf->buffer), head, *length) != EOK) {}
        buf->rx_len = *length;
        return EXT_ERR_FAILURE;
    }

    if (mrs_proto_1376_2_check(head, frm_len) == TD_TRUE) {
        td_pbyte payload = (td_pbyte)mrs_malloc(frm_len);
        td_u32 ret;

        if (payload == TD_NULL) {
            buf->rx_len = 0;
            return EXT_ERR_FAILURE;
        }

        (td_void) memcpy_s(payload, frm_len, head, frm_len);
        mrs_dfx_uart_chl_rx_frame(frm_len, EXT_ERR_SUCCESS, TD_FALSE);
        ret = mrs_msg_queue_send(MRS_MSG_ID_APP_FRAME_RX_13762, frm_len, (uintptr_t)payload, 0);
        if (ret != EXT_ERR_SUCCESS) {
            mrs_dfx_uart_chl_rx_frame(frm_len, ret, TD_FALSE);
            mrs_free(payload);

            buf->rx_len = 0;
            return EXT_ERR_FAILURE;
        }

        (*buffer) += frm_len;
        *length -= frm_len;
        return EXT_ERR_CONTINUE;
    }

    return EXT_ERR_SUCCESS;
}

static td_void mrs_proto_1376_2_parse(mrs_proto_buffer *buf)
{
    td_pbyte buffer = buf->buffer;
    td_u16 length = buf->rx_len;
    td_u32 ret;

    while (length > 0) {
        td_pbyte head = mrs_find_byte_in_stream(buffer, length, MRS_13762_HEAD_BYTE);
        if (head == TD_NULL) {
            buf->rx_len = 0;
            return;
        }

        length -= (td_u16)(head - buffer);
        buffer = head;

        if (length <= 2) { /* length less than 2bytes */
            if (memmove_s(buf->buffer, sizeof(buf->buffer), head, length) != EOK) {}
            buf->rx_len = length;
            return;
        }

        ret = mrs_proto_1376_2_check_sucess_for_parse(buf, head, &buffer, &length);
        if (ret == EXT_ERR_CONTINUE) {
            continue;
        } else if (ret == EXT_ERR_FAILURE) {
            return;
        }

        buffer++;
        length--;
    }

    buf->rx_len = 0;
}

static td_void mrs_proto_1376_2_rx_notify(EXT_CONST mrs_proto_buffer *buf)
{
    mrs_timer_start(MRS_TIMER_ID_UART,
                    (buf->simu_enable == TD_FALSE) ? MRS_UART_FRAME_INTERVAL : MRS_SIMU_FRAME_INTERVAL,
                    EXT_TIMER_TYPE_ONCE);
}

static td_void mrs_proto_1376_2_rx_full_notify(EXT_CONST mrs_proto_buffer *buf)
{
    ext_unref_param(buf);
    mrs_proto_parse(TD_FALSE);
}

td_void mrs_proto_1376_2_on_timer(td_void)
{
    mrs_proto_parse(TD_TRUE);
}

static td_u32 mrs_proto_1376_2_get_frame_for_decode(mrs_proto_1376_2_frame *frame,
    td_pbyte *data, td_u16 length)
{
    td_u16 min_size_13762 = (td_u16)MRS_13762_FRAME_MIN_SIZE;
    td_u16 addr_len = 0;

    frame->comm = **data & 0x3F;
    frame->prm = (**data >> 6) & 0x01; /* the 6th bit */
    frame->dir = (**data >> 7) & 0x01; /* the 7th bit means dir */
    if (frame->dir == 1) {
        /* ignore up frame */
        return EXT_ERR_BAD_DATA;
    }

    /* R filed */
    (*data)++;
    frame->route_flag = **data & 0x1;
    frame->sub_flag = (**data >> 1) & 0x01;
    frame->module_flag = (**data >> 2) & 0x01;   /* the 2nd bit means module flag */
    frame->conflict_flag = (**data >> 3) & 0x01; /* the 3rd bit means conflict flag */
    frame->relay_level = (**data >> EXT_U4_BITS) & EXT_U4_MAX;   /* high 4bits means relay level */

    (*data)++;
    frame->ch_flag = **data & EXT_U4_MAX;
    frame->err_corr_code = (**data >> EXT_U4_BITS) & EXT_U4_MAX; /* high 4bits means err correction code */

    (*data)++;
    frame->expect_bytes_phase = *((*data)++);

    *data += 2; /* skip rate(2B) */
    frame->seq = *((*data)++);

    if (frame->module_flag == 1) {
        addr_len = (frame->relay_level + 2) * EXT_METER_ADDR_LEN; /* relay level add 2 */
        if (addr_len + min_size_13762 > length) {
            return EXT_ERR_BAD_DATA;
        }

        if (memcpy_s(frame->src_addr, sizeof(frame->src_addr), *data, EXT_METER_ADDR_LEN) != EOK) {
            return EXT_ERR_BAD_DATA;
        }

        *data += EXT_METER_ADDR_LEN + EXT_METER_ADDR_LEN * frame->relay_level;
        if (memcpy_s(frame->dst_addr, sizeof(frame->dst_addr), *data, EXT_METER_ADDR_LEN) != EOK) {
            return EXT_ERR_BAD_DATA;
        }
        *data += EXT_METER_ADDR_LEN;
    }

    frame->data_len = length - min_size_13762 - addr_len;
    frame->afn = *((*data)++);
    frame->fn = mrs_proto_1376_2_get_fn(*data);
    if (frame->fn == 0) {
        return EXT_ERR_BAD_DATA;
    }

    *data += 2; /* fn 2B */

    return EXT_ERR_SUCCESS;
}

td_u32 mrs_proto_1376_2_decode(td_pbyte buffer, td_u16 length, mrs_proto_1376_2_frame **frame)
{
    mrs_proto_1376_2_frame tmp_frame;
    td_pbyte payload;
    td_u16 payload_len;
    td_pbyte data = buffer;

    if (buffer == TD_NULL) {
        return EXT_ERR_INVALID_PARAMETER;
    }

    if (mrs_proto_1376_2_check(buffer, length) != TD_TRUE) {
        return EXT_ERR_BAD_DATA;
    }

    (td_void) memset_s(&tmp_frame, sizeof(tmp_frame), 0, sizeof(tmp_frame));

    /* C field */
    data += MRS_13762_HEAD_SIZE;
    data += MRS_13762_LEN_SIZE;

    if (mrs_proto_1376_2_get_frame_for_decode(&tmp_frame, &data, length) != EXT_ERR_SUCCESS) {
        return EXT_ERR_BAD_DATA;
    }

    payload_len = sizeof(mrs_proto_1376_2_frame) + tmp_frame.data_len;
    payload = (td_pbyte)mrs_malloc(payload_len);
    if (payload == TD_NULL) {
        return EXT_ERR_MALLOC_FAILUE;
    }

    if (memcpy_s(payload, sizeof(mrs_proto_1376_2_frame), &tmp_frame, sizeof(tmp_frame)) != EOK) {
        mrs_free(payload);
        return EXT_ERR_FAILURE;
    }

    (td_void) memcpy_s(payload + sizeof(mrs_proto_1376_2_frame), payload_len - sizeof(mrs_proto_1376_2_frame), data,
        tmp_frame.data_len);

    *frame = (mrs_proto_1376_2_frame *)payload;

    return EXT_ERR_SUCCESS;
}

static td_u8 mrs_proto_1376_2_get_fn(const td_u8 dt[2]) /* 2B */
{
    td_u8 i;

    if (dt[1] >= 32) { /* if the 1st byte more than 32 */
        return 0;
    }

    for (i = 0; i < 8; i++) { /* less than 8 */
        if ((dt[0] & (1 << i)) != 0) {
            break;
        }
    }

    if (i == 8) { /* if equal to 8 */
        return 0;
    }

    return (td_u8)(dt[1] * 8 + i + 1); /* result: 1stByte *8 +i + 1 */
}

static td_u32 mrs_proto_1376_2_set_frame_for_create(EXT_CONST mrs_proto_1376_2_encode *param,
    td_u16 payload_len, td_pbyte payload, td_u16 *offset)
{
    /* head byte */
    payload[(*offset)++] = MRS_13762_HEAD_BYTE;

    /* len */
    payload[(*offset)++] = (td_u8)(payload_len & EXT_U8_MAX);
    payload[(*offset)++] = (td_u8)((payload_len >> EXT_U8_BITS) & EXT_U8_MAX);

    /* C field */
    payload[(*offset)++] = (MRS_13762_DIR_UP << 7) |               /* the 7th bit */
        (param->prm << 6) | MRS_13762_COM_FORM; /* the 6th bit */

    /* R field */
    payload[(*offset)++] = (td_u8)(param->module_flag << 2); /* module flag shifts 2bits */
    payload[(*offset)++] = 0;
    payload[(*offset)++] = 0;
    payload[(*offset)++] = 0;
    payload[(*offset)++] = param->evt_flag;
    payload[(*offset)++] = param->seq;

    /* addr */
    if (param->module_flag == 1) {
        if (memcpy_s(payload + *offset, payload_len - *offset, param->src_addr, EXT_METER_ADDR_LEN) != EOK) {
            return EXT_ERR_FAILURE;
        }
        (*offset) += EXT_METER_ADDR_LEN;

        if (memcpy_s(payload + *offset, payload_len - *offset, param->dst_addr, EXT_METER_ADDR_LEN) != EOK) {
            return EXT_ERR_FAILURE;
        }
        (*offset) += EXT_METER_ADDR_LEN;
    }

    /* AFN */
    payload[(*offset)++] = param->afn;

    /* Fn(Dt) */
    payload[(*offset)++] = (1 << ((param->fn - 1) & 0x7));
    payload[(*offset)++] = (param->fn - 1) / 8; /* fn calc method : devide 8 */

    /* app data */
    if (param->length > 0 && memcpy_s(payload + *offset, payload_len - *offset, param->data, param->length) != EOK) {
        return EXT_ERR_FAILURE;
    }
    (*offset) += param->length;

    return EXT_ERR_SUCCESS;
}

td_u32 mrs_proto_1376_2_create_frame(const mrs_proto_1376_2_encode *param, td_pbyte *buffer, td_u16 *length)
{
    td_pbyte payload;
    td_u16 payload_len = MRS_13762_FRAME_MIN_SIZE;
    td_u16 offset = 0;

    if ((param == TD_NULL) || (buffer == TD_NULL) || (length == TD_NULL)) {
        return EXT_ERR_INVALID_PARAMETER;
    }

    if (param->fn < 1) {
        return EXT_ERR_INVALID_PARAMETER;
    }

    payload_len += (param->module_flag == 1) ? 12 : 0; /* addr 12 */
    payload_len += param->length;

    payload = (td_pbyte)mrs_malloc(payload_len);
    if (payload == TD_NULL) {
        return EXT_ERR_MALLOC_FAILUE;
    }

    (td_void) memset_s(payload, payload_len, 0, payload_len);
    if (mrs_proto_1376_2_set_frame_for_create(param, payload_len, payload, &offset) != EXT_ERR_SUCCESS) {
        mrs_free(payload);
        return EXT_ERR_FAILURE;
    }

    /* cs */
    payload[offset++] = mrs_checksum8(payload + 3, payload_len - 5); /* cs shifts 3B, length is 5B less than all */

    /* tail byte */
    payload[offset++] = MRS_13762_TAIL_BYTE;

    *buffer = payload;
    *length = payload_len;

    return EXT_ERR_SUCCESS;
}

td_u32 mrs_1376_2_time_verify(const td_pbyte date_time, td_u8 length)
{
    td_u8 temp[7]; /* date time 7B */

    if (length != 6) { /* length of variable data_time must equal to 6B */
        return EXT_ERR_INVALID_PARAMETER;
    }

    temp[0] = date_time[0]; /* index 0 */
    temp[1] = date_time[1]; /* index 1 */
    temp[2] = date_time[2]; /* index 2 */
    temp[3] = 0; /* index 3 */
    temp[4] = date_time[3]; /* index 4 = 3 */
    temp[5] = date_time[4]; /* index 5 = 4 */
    temp[6] = date_time[5]; /* index 6 = 5 */

    return mrs_1376_2_time_week_verify(temp, sizeof(temp));
}

td_u32 mrs_1376_2_time_week_verify(const td_pbyte date_time, td_u8 length)
{
    td_u16 year;
    td_u8 month;
    td_u8 month_day[12] = { 0x31, 0x28, 0x31, 0x30, 0x31, 0x30, 0x31, 0x31, 0x30, 0x31, 0x30, 0x31 }; /* 12months */

    if ((length != 7) || (mrs_check_bcd_stream(date_time, length) == TD_FALSE)) { /* date time 7B */
        return EXT_ERR_BAD_DATA;
    }

    year = (td_u16)bcd2int(date_time[6]) + MRS_YEAR_BASE; /* year: index 6 */
    month = bcd2int(date_time[5]);                        /* month : index 5 */
    if ((month == 0) || (month > 12)) { /* month is between 0 and 12 */
        return EXT_ERR_BAD_DATA;
    }

    if (is_leap_year(year)) {
        month_day[2] = 0x29; /* index 2: 0x29 */
    }

    /* check 'second' */
    if (not_in_range(0x01, 0x60, date_time[0] + 1)) {
        return EXT_ERR_BAD_DATA;
    }

    /* check 'minute' */
    if (not_in_range(0x01, 0x60, date_time[1] + 1)) {
        return EXT_ERR_BAD_DATA;
    }

    /* check 'hour' */
    if (not_in_range(0x01, 0x24, date_time[2] + 1)) { /* index 2 add 1: hour is between 0x01 and 0x24 */
        return EXT_ERR_BAD_DATA;
    }

    /* check 'week' */
    if (not_in_range(0x01, 0x07, date_time[3] + 1)) { /* index 3: week is between 0x01 and 0x7 */
        return EXT_ERR_BAD_DATA;
    }

    /* check 'day' */
    if (not_in_range(0x01, month_day[month - 1], date_time[4])) { /* index 4: between 0x01 and max day of month */
        return EXT_ERR_BAD_DATA;
    }

    /* check 'month' */
    if (not_in_range(0x01, 0x12, date_time[5])) { /* index 5: month is between 0x01 and 0x12 */
        return EXT_ERR_BAD_DATA;
    }

    return EXT_ERR_SUCCESS;
}

#ifdef __cplusplus
}
#endif


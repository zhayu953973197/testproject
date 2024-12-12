/*
 * Copyright (c) CompanyNameMagicTag 2018-2020. All rights reserved.
 * Description: APP DL/T645 protocol base API.
 */

#ifndef APP_PROTO_645_H
#define APP_PROTO_645_H

#include "soc_types.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)
#ifdef __cplusplus
extern "C" {
#endif

#define PROTO_645_DATAGRAM_DATA_LEN_MAX    200
#define PROTO_645_DATAGRAM_HEAD_LEN_MAX    12
#define PROTO_645_DATAGRAM_LEN_MIN         PROTO_645_DATAGRAM_HEAD_LEN_MAX
#define PROTO_645_DATAGRAM_LEN_MAX         (PROTO_645_DATAGRAM_DATA_LEN_MAX + PROTO_645_DATAGRAM_HEAD_LEN_MAX)
#define PROTO_645_HEAD_BYTE                0x68
#define PROTO_645_TAIL_BYTE                0x16
#define PROTO_645_HEAD_BTYE7_OFFSET        7
#define PROTO_645_CHECK_LEN                2
#define PROTO_645_DATAGRAM_DATA_LEN_OFFSET 9

td_pbyte app_tools_find_byte(td_pbyte buffer, td_u16 length, td_u8 ch);
td_bool app_check_frame_645(td_pbyte frame, td_u16 length);
td_u32 app_find_frame_645(td_pbyte buffer, td_u16 length, td_u16 *start_pos, td_u16 *frame_length);
td_void app_frame_645_encode(td_pbyte buffer, td_u16 length);
td_void app_frame_645_decode(td_pbyte buffer, td_u16 length);

#ifdef __cplusplus
}
#endif
#endif
#endif

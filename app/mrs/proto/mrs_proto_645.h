/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS protocol
 */

#ifndef __MRS_PROTO_645_H__
#define __MRS_PROTO_645_H__

#include "mrs_proto.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MRS_645_97_DATA_DI_SIZE 2
#define MRS_645_DATA_DI_SIZE    4

#define MRS_645_CTRL_READ_DATA 0x11

#define MRS_645_FRAME_LEN_MIN      12
#define MRS_645_FRAME_DATA_LEN_MAX 200
#define MRS_645_FRAME_LEN_MAX      (MRS_645_FRAME_LEN_MIN + MRS_645_FRAME_DATA_LEN_MAX)
#define MRS_645_FRAME_1ST_HEAD_LEN 1
#define MRS_645_FRAME_METERADD_LEN 6
#define MRS_645_FRAME_2ND_HEAD_LEN 1
#define MRS_645_FRAME_CTRL_LEN     1
#define MRS_645_FRAME_LEN_LEN      1

#define MRS_645_FRAME_1ST_HEAD_OFFSET 0
#define MRS_645_FRAME_METERADD_OFFSET (MRS_645_FRAME_1ST_HEAD_OFFSET + MRS_645_FRAME_1ST_HEAD_LEN)
#define MRS_645_FRAME_2ND_HEAD_OFFSET (MRS_645_FRAME_METERADD_OFFSET + MRS_645_FRAME_METERADD_LEN)
#define MRS_645_FRAME_CTRL_OFFSET     (MRS_645_FRAME_2ND_HEAD_OFFSET + MRS_645_FRAME_2ND_HEAD_LEN)
#define MRS_645_FRAME_LEN_OFFSET      (MRS_645_FRAME_CTRL_OFFSET + MRS_645_FRAME_CTRL_LEN)
#define MRS_645_FRAME_DATA_OFFSET     (MRS_645_FRAME_LEN_OFFSET + MRS_645_FRAME_LEN_LEN)

#define MRS_645_FRAME_HEX33 0x33
#define MRS_645_FRAME_WILD_MAC "\xAA\xAA\xAA\xAA\xAA\xAA"
#define mrs_is_wild_mac(mac) (memcmp((mac), MRS_645_FRAME_WILD_MAC, 6) == 0)

typedef struct {
    td_u8 addr[MRS_METER_ADDR_LEN];
    td_u8 ctrl;
    td_u8 data_len;
    td_u8 *data;
} mrs_proto_645_frame_inf;

typedef struct {
    td_u16 datagram_size;
    td_u16 reserved;
    td_pbyte datagram;
} mrs_raw_frame_t;

td_u32 mrs_proto_645_find_frame(const td_pbyte buffer, const td_u16 length, td_u16 *start_pos, td_u16 *frm_len);
td_bool mrs_proto_645_check(const td_pbyte buffer, const td_u16 length);
td_void mrs_proto_645_on_timer(td_void);

/* Init */
td_u32 mrs_proto_645_init(td_void);

/* Generate 645 frame */
td_u32 mrs_proto_645_create_frame(const mrs_proto_645_frame_inf *frame_inf, td_u8 **frame, td_u16 *frame_len);
td_u32 mrs_proto_645_decode_frame(const td_pbyte buffer, const td_u16 length, mrs_proto_645_frame_inf *frame);

/* Decode 645 frame data */
td_void mrs_proto_645_data_decode(td_u8 *buf, td_u32 len);
/* Encode 645 frame data */
td_void mrs_proto_645_data_encode(td_u8 *buf, td_u32 len);

td_bool mrs_proto_is_rcv_p2p_pkt_by_dest_mac(EXT_CONST td_u8 *dest_mac, td_u8 mac_len);

#ifdef __cplusplus
}
#endif

#endif /* __MRS_PROTO_645_H__ */

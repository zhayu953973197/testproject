/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS UART protocol.
 */

#ifndef __MRS_PROTO_H__
#define __MRS_PROTO_H__

#include "mrs_config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    td_u16 rx_len;
    td_bool simu_enable;
    td_u8 rsv;
    td_u8 buffer[MRS_DATA_BUFFER_SIZE];
} mrs_proto_buffer;

typedef td_void (*mrs_proto_rx_notify)(EXT_CONST mrs_proto_buffer *buf);
typedef td_void (*mrs_proto_rx_full)(EXT_CONST mrs_proto_buffer *buf);
typedef td_void (*mrs_proto_parse_fct)(mrs_proto_buffer *buf);

typedef struct {
    mrs_proto_rx_notify rx_notify;
    mrs_proto_rx_full rx_full;
    mrs_proto_parse_fct parse;
} mrs_proto_handler;

typedef struct {
    mrs_proto_buffer buf;
    mrs_proto_handler handler;
} mrs_proto_ctx;

td_u32 mrs_proto_init(td_void);

td_void mrs_proto_clear_buffer(td_void);

td_void mrs_proto_register_handler(const mrs_proto_handler *hanlder);

td_void mrs_proto_parse(td_bool overtime);

td_void mrs_proto_rx(td_u8 ch);

td_void mrs_proto_simu_ctrl(td_bool enable);

#ifdef __cplusplus
}
#endif
#endif /* __MRS_PROTO_H__ */


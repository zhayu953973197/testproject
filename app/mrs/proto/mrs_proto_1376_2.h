/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS protocol
 */

#ifndef __MRS_PROTO_1376_2_H__
#define __MRS_PROTO_1376_2_H__

#include "mrs_proto.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Second node length */
#define MRS_1376_2_SEC_NODE_NUM_LEN_MIN 1
#define MRS_1376_2_SEC_NODE_INFO_LEN    3
#define MRS_1376_2_SEC_NODE_ITEM_SIZE   7
#define MRS_3762_SEC_NODE_MAX_NUM       32

typedef enum {
    MRS_1376_2_DENY_TIMEOUT,           /* 00H: */
    MRS_1376_2_DENY_INVALID_DATA,      /* 01H: */
    MRS_1376_2_DENY_LEN_ERR,           /* 02H: */
    MRS_1376_2_DENY_CS_ERR,            /* 03H: */
    MRS_1376_2_DENY_FN_NOT_EXIST,      /* 04H: */
    MRS_1376_2_DENY_FORMAT_ERR,        /* 05H: */
    MRS_1376_2_DENY_METER_REPEAT,      /* 06H: */
    MRS_1376_2_DENY_METER_NOT_EXIST,   /* 07H: */
    MRS_1376_2_DENY_METER_NO_RESPONSE, /* 08H: */
    MRS_1376_2_DENY_CCO_BUSY,          /* 09H: */
    MRS_1376_2_DENY_CCO_NOT_SUPPORT,   /* 0AH: */
    MRS_1376_2_DENY_STA_NO_RESPONSE,   /* 0BH: */
    MRS_1376_2_DENY_STA_NOT_IN_TOPO,   /* 0CH: */
    MRS_1376_2_DENY_RM_POOL_FULL,      /* 0DH: */
    MRS_1376_2_DENY_RM_645ID_OVERRUN,  /* 0EH: */
    MRS_1376_2_DENY_RM_METER_BUSY,     /* 0FH: */
} mrs_proto_1376_2_deny_code;

typedef struct {
    /* C: conrtol domain */
    /*
     * communication mode
     * 0: reserved
     * 1: concentrated routing carrier communication
     * 2: distributed routing carrier communication
     * 3: broband carrier communcation
     * 4~9: reserved
     * 10: Micropower wireless communication
     * 11~19: reserved
     * 20: ethernet communication
     * 21~63: reserved
 */
    td_u8 comm : 6;

    /*
     * start identification bit
     * 0: means frame sent from slave station
     * 1: means frame sent from host station
 */
    td_u8 prm : 1;

    /*
     * transmit direction bit
     * 0: means this is a download frame sent from concentrator
     * 1: means this is a upload frame sent from communication module
 */
    td_u8 dir : 1;

    /* R: control domain */
    td_u8 route_flag : 1;    /* route flag: 0-with route; 1-without route */
    td_u8 sub_flag : 1;      /* secondary node flag: 0-no secondary node; 1-with secondary nodes */
    td_u8 module_flag : 1;   /* communication module flag: 0-concentrator module; 1-slave node module */
    td_u8 conflict_flag : 1; /* conflict detection: 0-no conflict; 1-with conflicts */
    td_u8 relay_level : 4;   /* relay level: 0~15, 0-no relay */

    td_u8 ch_flag : 4;       /* channel ID:0~15, 0-not distinguish channel */
    td_u8 err_corr_code : 4; /* erro correction code: 0~15; 0-uncoded;  1-RS coding; 2~15 reserved */

    td_u8 expect_bytes_phase; /* DOWN: length of expected reply bytes ; UP: [3:0] phase feature [7:4] meter feature */

    td_u8 rsv0;         /* fill with 0 */
    td_u8 evt_flag : 1; /* UP: event identification */
    td_u8 rsv1 : 7;     /* fill with 0 */

    td_u8 seq; /* frame sequence */

    td_u8 afn; /* function code */

    /* address domain */
    td_u8 src_addr[EXT_METER_ADDR_LEN];
    td_u8 dst_addr[EXT_METER_ADDR_LEN];

    td_u8 fn;
    td_u8 rsv2;
    td_u16 data_len;

    td_u8 content[0];
} mrs_proto_1376_2_frame;

typedef struct {
    td_u8 prm : 1;
    td_u8 module_flag : 1;
    td_u8 evt_flag : 1;
    td_u8 relay_level : 4;
    td_u8 rsv0 : 1;
    td_u8 seq;
    td_u8 phase;
    td_u8 rsv1;
    td_u8 src_addr[EXT_METER_ADDR_LEN];
    td_u8 dst_addr[EXT_METER_ADDR_LEN];
    td_u8 afn;
    td_u8 fn;
    td_u16 length;
    td_u8 *data;
} mrs_proto_1376_2_encode;

td_u32 mrs_proto_1376_2_init(td_void);

td_bool mrs_proto_1376_2_check(td_pbyte buffer, td_u16 length);

td_void mrs_proto_1376_2_on_timer(td_void);

td_u32 mrs_proto_1376_2_decode(td_pbyte buffer, td_u16 length, mrs_proto_1376_2_frame **frame);

td_u32 mrs_proto_1376_2_create_frame(const mrs_proto_1376_2_encode *param, td_pbyte *buffer, td_u16 *length);

td_u32 mrs_1376_2_time_verify(const td_pbyte date_time, td_u8 length);

td_u32 mrs_1376_2_time_week_verify(const td_pbyte date_time, td_u8 length);

#ifdef __cplusplus
}
#endif
#endif /* __MRS_PROTO_1376_2_H__ */


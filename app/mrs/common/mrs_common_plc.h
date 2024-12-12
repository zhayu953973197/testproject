/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: PLC protocol definition - application layer.
 */

#ifndef __MRS_COMMON_PLC_H__
#define __MRS_COMMON_PLC_H__

#include <soc_mdm.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    td_u16 id;
    td_u8 addr[EXT_PLC_MAC_ADDR_LEN];
    td_u16 bc_flag : 1;
    td_u16 option : 15;
    td_u16 payload_len;
    td_u8 *payload;
    td_u8 form_serial_number;
    td_u8 pad[3];    /* reserved 3 bytes */
} mrs_plc_frame_data;

#define PLC_BC_MAC       "\xFF\xFF\xFF\xFF\xFF\xFF"
#define is_plc_bc_mac(x) (memcmp((x), PLC_BC_MAC, 6) == 0)

#define PLC_DATA_LEN_MAX 512

/* PLC ID definition */
#define PLC_CMD_ID_XR  0x0001 /* Data transmit: ANF=13H-F1. */
#define PLC_CMD_ID_PR  0x0003 /* Data transmit: ANF=F1H-F1. */
#define PLC_CMD_ID_BC  0x0004 /* Broadcast. */
#define PLC_CMD_ID_EVT 0x0008 /* Event report. */

#define PLC_CMD_ID_METER_LIST   0x0011 /* Search meter: get meter list. */
#define PLC_CMD_ID_START_SEARCH 0x0012 /* Search meter: start search. */
#define PLC_CMD_ID_STOP_SEARCH  0x0013 /* Search meter: stop search. */

#define PLC_CMD_ID_CIU_TRANSMIT 0x00C1 /* Transmit plc frame between CIU & STA */
#define PLC_CMD_ID_QRY_STA_INF  0x00C2 /* Query STA information */

#define PLC_CMD_ID_DUT_CCO_TEST 0xF001 /* For CCO dut test only. */
#define PLC_CMD_ID_DUT_STA_TEST 0xF002 /* For STA dut test only. */

#define MRS_PLC_PROTO_VERSION 1

#define MRS_PLC_DN_FLG 0
#define MRS_PLC_UP_FLG 1

#define MRS_PLC_PRM_HOST  1 /* host station */
#define MRS_PLC_PRM_SLAVE 0 /* slave station */

/* product type definition: device type with 1376.2 treaty for concentrator and CCO. */
#define MRS_DEVICE_TYPE_1376_2_CLT   0
#define MRS_DEVICE_TYPE_1376_2_METER 1
#define MRS_DEVICE_TYPE_1376_2_RELAY 2

/* product type definition: device type with treaty for PLC and CCO. */
#define MRS_DEVICE_TYPE_PLC_METER  0
#define MRS_DEVICE_TYPE_PLC_CLT_I  1
#define MRS_DEVICE_TYPE_PLC_CLT_II 2
#define MRS_DEVICE_TYPE_PLC_RELAY  3

typedef struct {
    td_u16 stru_ver : 6;  /* treaty version,equal to 1. */
    td_u16 stru_size : 6; /* length of frame head */
    td_u16 rsv0 : 4;      /* setting bytes,equal to 0 */
    td_u16 protocol : 4;  /* protocol type of transmit data */
    td_u16 data_len : 12; /* length of transmit data */
    td_u16 seq;           /* frame sequence */
    td_u8 timeout;        /* down: frame timeout time; up: equal to 0. */
    td_u8 dir : 1;        /* direction bit: 0-down; 1-up */
    td_u8 rsv1 : 7;       /* equal to 0 */
    td_u8 data[0];        /* data for transmitting */
} mrs_data_transmit_xr;

typedef struct {
    td_u16 stru_ver : 6;   /* treaty version,equal to 1. */
    td_u16 stru_size : 6;  /* length of frame head */
    td_u16 no_rsp_flg : 1; /* retry flag of no response */
    td_u16 nak_flg : 1;    /* retry flag of denial */
    td_u16 try_max : 2;    /* max retry count */
    td_u16 protocol : 4;   /* protocol type of transmit data */
    td_u16 data_len : 12;  /* length of transmit data */
    td_u16 seq;            /* frame sequence */
    td_u8 timeout;         /* frame timeout time(100ms) */
    td_u8 gap;             /* frame interval(10ms) */
    td_u8 data[0];         /* data for transmitting */
} mrs_data_transmit_pr_dn;

typedef struct {
    td_u16 stru_ver : 6;  /* treaty version,equal to 1. */
    td_u16 stru_size : 6; /* length of frame head */
    td_u16 rsv0 : 4;      /* reply status, equal to 0. */
    td_u16 protocol : 4;  /* protocol type of transmit data */
    td_u16 data_len : 12; /* length of transmit data */
    td_u16 seq;           /* frame sequence */
    td_u16 status;        /* reply status */
    td_u8 data[0];        /* data for transmitting */
} mrs_data_transmit_pr_up;

/* Broadcast definition */
typedef struct {
    td_u16 stru_ver : 6;  /* treaty version,equal to 1. */
    td_u16 stru_size : 6; /* length of frame head */
    td_u16 rsv0 : 4;
    td_u16 rsv1 : 4;
    td_u16 data_len : 12; /* length of transmit data */
    td_u8 data[0];        /* data for transmitting */
} mrs_broadcast_frame;

#define MRS_EVENT_PRM_SLAVE 0
#define MRS_EVENT_PRM_HOST  1

#define MRS_EVNET_FUNC_ACK            1
#define MRS_EVNET_FUNC_REPORT_ENABLE  2
#define MRS_EVNET_FUNC_REPORT_DISABLE 3
#define MRS_EVENT_FUNC_CCO_BUF_FULL   4

#define MRS_EVENT_FUNC_METER_REPORT 1
/* expanding function code */
#define MRS_EVENT_FUNC_MODUL_REPORT 2 /* event function code: report by self(up frame), trigger by module */

typedef struct {
    td_u32 stru_ver : 6;            /* treaty version,equal to 1. */
    td_u32 stru_size : 6;           /* length of frame head */
    td_u32 dir : 1;                 /* direction bit */
    td_u32 prm : 1;                 /* start bit */
    td_u32 func_code : 6;           /* function code */
    td_u32 data_len : 12;           /* length of transmit data */
    td_u16 seq;                     /* frame sequence */
    td_u8 meter[EXT_METER_ADDR_LEN]; /* meter address: DOWN-no definition for now, equal to 0.
                                                                    * UP-address of meter with events */
    td_u8 data[0];                  /* data for transmitting */
} mrs_meter_event_info;

typedef struct {
    td_u8 addr[EXT_METER_ADDR_LEN]; /* meter address */
    td_u8 protocol;                /* protocol type */
    td_u8 type : 4;                /* 0:electric meter 1:I-type collector  2: II-type collector */
    td_u8 padd : 4;
} mrs_cmd_meterlist_item;

/* macro definition of enable for transformer identify */
#define MRS_TF_ENABLE  1
#define MRS_TF_DISABLE 0

/* macro definition of transformer identify result */
#define MRS_TF_RESULT_NONE       0 /* no result. */
#define MRS_TF_RESULT_BELONG     1 /* result: self transformer. */
#define MRS_TF_RESULT_NOT_BELONG 2 /* result: other transformer. */

/* macro definition of transformer identify status */
typedef enum {
    MRS_TF_STATUS_DEFAULT = 0,   /* identify closed */
    MRS_TF_STATUE_IDENTIFYING,   /* in process of transformer identifying */
    MRS_TF_STATUS_REPORT_RESULT, /* transformer identify finished, in process of reporting results(only CCO) */
    MRS_TF_STATUS_COMPLETED,     /* transformer identify finished,  (CCO: reporting results to concentrator finished.
                                                            * STA: identify finished and closed identify)
 */
} mrs_tf_status;

typedef enum {
    MRS_SM_SEARCH_RESULT = 0, /* command: query result of slave node registration */
    MRS_SM_START_SEARCH,      /* command: start register of slave nodes */
    MRS_SM_OPTION_MAX,
} mrs_sm_option;

typedef struct {
    td_u16 interface_ver : 6; /* treaty version */
    td_u16 stru_len : 6;      /* length of struct */
    td_u16 force_resp : 1;    /* compulsive reply flag */
    td_u16 option : 3;        /* parameter for search meter (mrs_sm_option_e): fill with MRS_SM_START_SEARCH */
    td_u16 reserved;
    td_u32 seq; /* search meter sequence: sequence increase when CCO send new query data to STA.
                            * sequence keep unchanging when send the frame again. */
} mrs_plc_start_search_meter;

typedef struct {
    td_u16 interface_ver : 6; /* treaty version */
    td_u16 stru_len : 6;      /* length of struct */
    td_u16 reserved1 : 4;
    td_u16 reserved2;
    td_u32 seq;
} mrs_plc_stop_search_meter;

typedef struct {
    td_u16 interface_ver : 6; /* treaty version */
    td_u16 stru_len : 6;      /* length of struct */
    td_u16 force_resp : 1;    /* compulsive reply flag */
    td_u16 option : 3;        /* parameter for search meter (mrs_sm_option_e): fill with MRS_SM_SEARCH_RESULT */
    td_u16 enable : 1;        /* enable for transformer identify: 0-disable, 1-enable */
    td_u16 reserved : 15;
    td_u32 seq; /* search meter sequence:
                            * Every search time is different with sequence. Sequence increase when search again. */
    td_u8 src[EXT_METER_ADDR_LEN];
    td_u8 dst[EXT_METER_ADDR_LEN];
} mrs_plc_meterlist_dl;

/* get results of search meter or map(UP) */
typedef struct {
    td_u16 interface_ver : 6; /* treaty version */
    td_u16 stru_len : 6;      /* length of struct */
    td_u16 status : 1;        /* status bit: used for compulsive reply */
    td_u16 option : 3;        /* search meter option (MRS_SM_OPTION_E) */
    td_u8 meter_num;          /* quatity of meter */
    td_u8 type;               /* 0:electric meter  1:I-type collector  2: II-type collector  3: */

    td_u8 asset[EXT_METER_ADDR_LEN]; /* collector/electric meter address(little endian) */
    td_u8 id[EXT_METER_ADDR_LEN];    /* STA address(little endian) */

    td_u32 seq; /* search meter sequence: Every search time is different with sequence.
                            * STA reply CCO with sequence for this search time. */

    td_u32 result : 2;    /* transformer identify results */
    td_u32 reserved : 30; /* reserved */

    td_u8 src[EXT_METER_ADDR_LEN];
    td_u8 dst[EXT_METER_ADDR_LEN];

    td_u8 meter_list[0]; /* meter address, address includes structure(mrs_cmd_meterlist_item) */
} mrs_plc_meterlist_up;


#define MRS_DIR_CIU_TO_STA 0
#define MRS_DIR_STA_TO_CIU 1

typedef struct {
    td_u32 stru_ver : 6;  /* treaty version,equal to 1 */
    td_u32 stru_size : 6; /* length of frame head */
    td_u32 dir : 1;       /* direction bit */
    td_u32 prm : 1;       /* start bit */
    td_u32 data_len : 10; /* length of transmit data */
    td_u32 seq : 8;       /* frame sequence */
    td_u8 data[0];        /* data for transmitting */
} mrs_ciu_transmit_inf;

/*
 * frame format for inquiring station info
 */
enum {
    MRS_QUERY_SW_VERSION = 0,
    MRS_QUERY_INVALID,
};

typedef struct {
    td_u8 id;
    td_u8 len;
    td_u8 pad[2]; /* reserved 2 bytes */
    td_u8 *data;
} sta_qry_element;

typedef struct {
    td_u16 stru_ver : 6;  /* treaty version,equal to 1 */
    td_u16 stru_size : 6; /* length of frame head */
    td_u16 rsv : 4;
    td_u8 seq;         /* frame sequence */
    td_u8 element_cnt; /* elements count in info list */
    td_u8 element[0];  /* info list */
} mrs_query_sta_inf;

/*
 * For CCO/STA dut test only
 */
#define MRS_SET_POWER_GAIN      0x0001 /* set transmission power */
#define MRS_QRY_STA_VERSION     0x0002 /* query version info */
#define MRS_SET_POWER_GAIN_TIME 60

#define MRS_DUT_VENDOR 0xa55aa55a

typedef struct {
    td_u16 stru_ver : 6;  /* treaty version,equal to 1 */
    td_u16 stru_size : 6; /* length of frame head */
    td_u16 dir : 1;       /* direction bit */
    td_u16 rsv : 3;
    td_u16 seq;      /* frame sequence */
    td_u32 crc;      /* CRC32 check: start from vendor to data */
    td_u32 vendor;   /* manufacturer ID, equal to #MRS_DUT_VENDOR */
    td_u16 cmd_id;   /* test command ID */
    td_u16 data_len; /* data length */
    td_u8 data[0];   /* data content */
} mrs_dut_test_frame;

typedef struct {
    td_u16 stru_ver : 6;  /* treaty version,equal to 1 */
    td_u16 stru_size : 6; /* length of frame head */
    td_u16 dir : 1;       /* direction bit */
    td_u16 rsv : 3;
    td_u16 seq;                        /* frame sequence */
    td_u32 crc;                        /* CRC32 check: start from vendor to data */
    td_u32 vendor;                     /* manufacturer ID, equal to #MRS_DUT_VENDOR */
    td_u8 src_addr[EXT_METER_ADDR_LEN]; /* address of sender */
    td_u16 rsv1;
    td_u16 cmd_id;   /* test command ID */
    td_u16 data_len; /* data length */
    td_u8 data[0];   /* data content */
} mrs_dut_cco_frame;

td_u32 mrs_plc_chl_init(td_void);

td_u32 mrs_plc_chl_tx(td_void);

td_u32 mrs_convert_meter_to_mac(const td_pbyte meter, const td_u8 meter_len, td_pbyte mac, td_u8 mac_len);

td_u32 mrs_plc_frame_send(const mrs_plc_frame_data *frame);

#define mrs_convert_mac_to_meter(mac, mac_len, meter, meter_len) \
    mrs_convert_meter_to_mac(mac, mac_len, meter, meter_len)

#ifdef __cplusplus
}
#endif
#endif /* __MRS_COMMON_PLC_H__ */


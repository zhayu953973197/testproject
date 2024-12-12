/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS cco 1376.2 handle.
 */

#include "mrs_cco_1376_2.h"
#include "mrs_proto_1376_2.h"
#include "mrs_msg.h"
#include "mrs_common_uart.h"
#include "mrs_common_tools.h"
#include "mrs_cco_mr_list.h"
#include "mrs_cco_xr.h"
#include "mrs_cco_pr.h"
#include "mrs_cco_archives.h"
#include "mrs_cco_srv.h"
#include "mrs_cco_evt.h"
#include "mrs_cco_queue.h"
#include "mrs_cco_upg.h"
#include "mrs_cco_broadcast.h"
#include "mrs_cco_tf.h"
#include "soc_mdm_pi.h"
#include "mrs_cco_tf.h"
#include "mrs_dfx.h"
#include "app_nv.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
#define MRS_10_F21_FIX_LEN  3
#define MRS_10_F21_NODE_LEN 11

#define MRS_TOPO_ROLE_INVALID 0
#define MRS_TOPO_ROLE_STA     1
#define MRS_TOPO_ROLE_PROXY   2
#define MRS_TOPO_ROLE_CCO     4

#define MRS_MR_TYPE_PR_F1_F1   0
#define MRS_MR_TYPE_XR_02_F1   1
#define MRS_MR_TYPE_XR_02_F255 2
#define MRS_MR_TYPE_XR_13_F1   3
#define MRS_MR_TYPE_XR_13_F255 4

typedef td_u32 (*mrs_1376_2_afn_func)(mrs_fn_param *param);
typedef td_u32 (*mrs_create_plc_frame)(mr_task_node *, td_pbyte, td_u16);
typedef td_u32 (*mrs_get_mr_data)(mrs_proto_1376_2_frame *, td_pbyte *, td_u16 *, td_u8 *, td_u8 *);

typedef struct {
    td_u32 afn_data;
    mrs_1376_2_afn_func func;
} mrs_afn_func_map;

typedef struct {
    td_u8 state_lo;
    td_u8 state_ml;
    td_u8 state_mh;
    td_u8 state_hi;
    td_u16 wait_time;
} mrs_afn_00_f1;


#pragma pack(1)
typedef struct {
    td_u8 node_addr[MRS_METER_ADDR_LEN];
    td_u16 node_tei;
    td_u8 node_parent_addr[MRS_METER_ADDR_LEN];
    td_u8 signal;     /* signal itensity */
    td_u8 stat_nw;    /* net status */
    td_u32 longitude; /* longitude: 6 bits valid */
    td_u32 latitude;  /* latitude: 6 bits valid */
    td_u8 phase;      /* phase */
    td_u8 pad[MRS_AFN_10_F27_NODE_PAD];     /* pad(3) */
} mrs_afn_10_f27_node;
#pragma pack()

td_u32 mrs_cco_queue_rx(mrs_fn_param *param);
td_u32 mrs_afn_0301_buf(td_pbyte buffer, td_u16 length);
td_u32 mrs_afn_0310_buf(td_pbyte *buffer, td_u16 *length);
td_u32 mrs_afn_10_f2_get_numbers(td_u16 *start_num, td_u8 *meter_num, td_u16 meters_total);
td_u32 mrs_afn_10_f2_fill_reply_content(td_u8 *reply_content, td_u16 reply_content_len,
                                        td_u16 start_num, td_u8 meter_num);
td_void mrs_afn_10_f27_get_numbers(td_u16 *start_num, td_u8 *node_num);
td_u16 mrs_afn_10_f27_get_start_index(const ext_mdm_nm_topo_node_info *topo_info, td_u16 start_num);
td_void mrs_afn_10_f27_get_node_info(ext_mdm_nm_topo_node_info *topo_info, const td_u8 *phase_info,
                                     td_u16 i, mrs_afn_10_f27_node *node);
mrs_create_plc_frame mrs_1376_2_mr_handle_get_node_info(const mrs_proto_1376_2_frame *frame,
                                                        td_u8 option, mr_task_node *node);
td_u32 mrs_afn_10_f230_fill_up_data(td_u8 *payload, td_u16 start_seq, td_u8 need_node_num,
    td_u8 *actual_report_num);
td_u32 mrs_afn_10_f230_fill_node_info(ext_mdm_nm_topo_node_info *topo_info, td_u8 *payload, td_u16 *data_offset,
    td_u16 my_tei);
td_u32 mrs_query_topo_down_check(td_bool *start_flag, mrs_fn_param *param, td_u16 *start_seq, td_u8 *query_num);
td_u32 mrs_afn10_query_data_check(mrs_fn_param *afn_param, td_u8 *query_num, td_u16 *start_seq,
    td_bool is_create_deny);
td_u32 mrs_1376_2_afn_xx_xx(mrs_fn_param *param);

td_u32 mrs_1376_2_afn_00_f1(mrs_fn_param *param);
td_u32 mrs_1376_2_afn_00_f2(mrs_fn_param *param);

td_u32 mrs_1376_2_afn_01_f1(mrs_fn_param *param);
td_u32 mrs_1376_2_afn_01_f2(mrs_fn_param *param);
td_u32 mrs_1376_2_afn_01_f3(mrs_fn_param *param);

td_u32 mrs_1376_2_afn_02_f1(mrs_fn_param *param);
td_u32 mrs_1376_2_afn_02_f255(mrs_fn_param *param);

td_u32 mrs_1376_2_afn_03_f1(mrs_fn_param *param);
td_u32 mrs_1376_2_afn_03_f4(mrs_fn_param *param);
td_u32 mrs_1376_2_afn_03_f10(mrs_fn_param *param);
td_u32 mrs_1376_2_afn_03_f16(mrs_fn_param *param);

td_u32 mrs_1376_2_afn_05_f1(mrs_fn_param *param);
td_u32 mrs_1376_2_afn_05_f3(mrs_fn_param *param);
td_u32 mrs_1376_2_afn_05_f16(mrs_fn_param *param);
td_u32 mrs_1376_2_afn_05_f255(mrs_fn_param *param);

td_u32 mrs_1376_2_afn_10_f1(mrs_fn_param *param);
td_u32 mrs_1376_2_afn_10_f2(mrs_fn_param *param);
td_u32 mrs_1376_2_afn_10_f4(mrs_fn_param *param);
td_u32 mrs_1376_2_afn_10_f27(mrs_fn_param *param);
td_u32 mrs_1376_2_afn_10_f230(mrs_fn_param *param);

td_u32 mrs_1376_2_afn_11_f1(mrs_fn_param *param);
td_u32 mrs_1376_2_afn_11_f2(mrs_fn_param *param);
td_u32 mrs_1376_2_afn_11_f5(mrs_fn_param *param);
td_u32 mrs_1376_2_afn_11_f6(mrs_fn_param *param);

td_u32 mrs_1376_2_afn_12_f1(mrs_fn_param *param);
td_u32 mrs_1376_2_afn_12_f2(mrs_fn_param *param);
td_u32 mrs_1376_2_afn_12_f3(mrs_fn_param *param);

td_u32 mrs_1376_2_afn_13_f1(mrs_fn_param *param);
td_u32 mrs_1376_2_afn_13_f255(mrs_fn_param *param);

td_u32 mrs_1376_2_afn_15_f1(mrs_fn_param *param);

td_u32 mrs_1376_2_afn_f1_f1(mrs_fn_param *param);

td_u32 mrs_1376_2_mr_handle(mrs_fn_param *param, td_u8 option);
td_u32 mrs_cco_get_mr_data_13_f1(mrs_proto_1376_2_frame *frame, td_pbyte *data, td_u16 *data_len, td_u8 *proto,
                                 td_u8 *deny);
td_u32 mrs_cco_get_mr_data_13_f255(mrs_proto_1376_2_frame *frame, td_pbyte *data, td_u16 *data_len, td_u8 *proto,
                                   td_u8 *deny);
td_u32 mrs_cco_get_mr_data_02_f1(mrs_proto_1376_2_frame *frame, td_pbyte *data, td_u16 *data_len, td_u8 *proto,
                                 td_u8 *deny);
td_u32 mrs_cco_get_mr_data_02_f255(mrs_proto_1376_2_frame *frame, td_pbyte *data, td_u16 *data_len, td_u8 *proto,
                                   td_u8 *deny);
td_u32 mrs_cco_get_mr_data_f1_f1(mrs_proto_1376_2_frame *frame, td_pbyte *data, td_u16 *data_len, td_u8 *proto,
                                 td_u8 *deny);

/* !!!NOTICE!!! elements in this array must in asending order by afn. And searching by halving method */
mrs_afn_func_map g_afn_func_map[] = {
    { 0x00000001, mrs_1376_2_afn_00_f1 },
    { 0x00000002, mrs_1376_2_afn_00_f2 },

    { 0x00000101, mrs_1376_2_afn_01_f1 },
    { 0x00000102, mrs_1376_2_afn_01_f2 },
    { 0x00000103, mrs_1376_2_afn_01_f3 },

    { 0x00000201, mrs_1376_2_afn_02_f1 },
    { 0x000002FF, mrs_1376_2_afn_02_f255 },

    { 0x00000301, mrs_1376_2_afn_03_f1 },
    { 0x00000304, mrs_1376_2_afn_03_f4 },
    { 0x0000030A, mrs_1376_2_afn_03_f10 },
    { 0x00000310, mrs_1376_2_afn_03_f16 },

    { 0x00000501, mrs_1376_2_afn_05_f1 },
    { 0x00000503, mrs_1376_2_afn_05_f3 },
    { 0x00000510, mrs_1376_2_afn_05_f16 },
    { 0x000005FF, mrs_1376_2_afn_05_f255 },

    { 0x00001001, mrs_1376_2_afn_10_f1 },
    { 0x00001002, mrs_1376_2_afn_10_f2 },
    { 0x00001004, mrs_1376_2_afn_10_f4 },
    { 0x0000101B, mrs_1376_2_afn_10_f27 },
    { 0x000010E6, mrs_1376_2_afn_10_f230 },
    { 0x00001101, mrs_1376_2_afn_11_f1 },
    { 0x00001102, mrs_1376_2_afn_11_f2 },
    { 0x00001105, mrs_1376_2_afn_11_f5 },
    { 0x00001106, mrs_1376_2_afn_11_f6 },

    { 0x00001201, mrs_1376_2_afn_12_f1 },
    { 0x00001202, mrs_1376_2_afn_12_f2 },
    { 0x00001203, mrs_1376_2_afn_12_f3 },

    { 0x00001301, mrs_1376_2_afn_13_f1 },
    { 0x000013FF, mrs_1376_2_afn_13_f255 },

    { 0x00001501, mrs_1376_2_afn_15_f1 },

    { 0x0000f101, mrs_1376_2_afn_f1_f1 },
};

td_u32 mrs_cco_1376_2_afn_dispatch(mrs_proto_1376_2_frame *frame, td_pbyte *buffer, td_u16 *length);
mrs_1376_2_afn_func mrs_cco_376_2_find_afn_handler(td_u8 afn, td_u8 fn);

td_void mrs_msg_on_app_frame_rx_13762(EXT_CONST mrs_queue_msg *msg)
{
    td_u32 ret;
    mrs_proto_1376_2_frame *frame = TD_NULL;
    td_pbyte payload = (td_pbyte)msg->param1;
    td_u16 length = msg->param0;

    if (payload == TD_NULL) {
        return;
    }

    mrs_dfx_uart_chl_rx(payload, length, TD_TRUE);
    ret = mrs_proto_1376_2_decode(payload, length, &frame);
    if (ret == EXT_ERR_SUCCESS && frame != TD_NULL) {
        td_pbyte out_buf = TD_NULL;
        td_u16 out_len = 0;

        ret = mrs_cco_1376_2_afn_dispatch(frame, &out_buf, &out_len);
        if ((ret == EXT_ERR_SUCCESS) && (out_buf != TD_NULL)) {
            mrs_uart_tx(out_buf, out_len);
        }

        mrs_free(out_buf);
        mrs_free(frame);
    } else {
        mrs_dfx_uart_chl_err_rx(length, ret, TD_TRUE);
    }

    mrs_dfx_uart_chl_rx_frame(length, ret, TD_TRUE);

    mrs_free(payload);
}

mrs_1376_2_afn_func mrs_cco_376_2_find_afn_handler(td_u8 afn, td_u8 fn)
{
    td_u32 afn_data = (td_u16)((afn << 8) + fn); /* AFN: high 8 bit */
    td_s32 low, high, mid;

    low = 0;
    if (afn_data == g_afn_func_map[low].afn_data) {
        return g_afn_func_map[low].func;
    }

    high = ext_array_count(g_afn_func_map) - 1;
    if (afn_data == g_afn_func_map[high].afn_data) {
        return g_afn_func_map[high].func;
    }

    while (low <= high) {
        mid = low + (high - low) / 2; /* get middle index by devide 2 */

        if (afn_data == g_afn_func_map[mid].afn_data) {
            return g_afn_func_map[mid].func;
        }

        if (afn_data < g_afn_func_map[mid].afn_data) {
            high = mid - 1;
        } else {
            low = mid + 1;
        }
    }

    return mrs_1376_2_afn_xx_xx;
}

td_u32 mrs_cco_1376_2_afn_dispatch(mrs_proto_1376_2_frame *frame, td_pbyte *buffer, td_u16 *length)
{
    mrs_1376_2_afn_func afn_func = mrs_cco_376_2_find_afn_handler(frame->afn, frame->fn);
    mrs_fn_param param;

    param.frame = frame;
    param.out_buf = buffer;
    param.out_len = length;
    param.afn = frame->afn;
    param.fn = frame->fn;
    param.rsv = 0;
    return afn_func(&param);
}

td_u32 mrs_cco_1376_2_ack_frame(const mrs_fn_param *dl_frame, td_u16 wait_time)
{
    mrs_fn_param param;
    mrs_afn_00_f1 data;

    (td_void) memcpy_s(&param, sizeof(param), dl_frame, sizeof(mrs_fn_param));
    param.afn = 0;
    param.fn = 1;

    (td_void) memset_s(&data, sizeof(data), 0, sizeof(data));
    data.wait_time = wait_time;

    return mrs_cco_1376_2_easy_encode(&param, (td_pbyte)&data, (td_u16)sizeof(data));
}

td_u32 mrs_cco_1376_2_deny_frame(const mrs_fn_param *dl_frame, td_u8 deny)
{
    mrs_fn_param param;

    (td_void) memcpy_s(&param, sizeof(param), dl_frame, sizeof(mrs_fn_param));
    param.afn = 0;
    param.fn = 2; /* deny fn: 2 */

    return mrs_cco_1376_2_easy_encode(&param, (td_pbyte)&deny, (td_u16)sizeof(deny));
}

td_u32 mrs_cco_1376_2_easy_encode(mrs_fn_param *param, td_pbyte data, td_u16 length)
{
    mrs_proto_1376_2_encode encode;

    if ((param == TD_NULL) || ((length != 0) && (data == TD_NULL))) {
        return EXT_ERR_INVALID_PARAMETER;
    }

    (td_void) memset_s(&encode, sizeof(encode), 0, sizeof(encode));
    encode.seq = param->frame->seq;
    encode.afn = param->afn;
    encode.fn = param->fn;
    encode.length = length;
    encode.data = data;
    return mrs_proto_1376_2_create_frame(&encode, param->out_buf, param->out_len);
}

td_u32 mrs_1376_2_afn_xx_xx(mrs_fn_param *param)
{
    /* reply denial frame, when received unsupported frame */
    return mrs_cco_1376_2_deny_frame(param, MRS_1376_2_DENY_FN_NOT_EXIST);
}

td_u32 mrs_cco_queue_rx(mrs_fn_param *param)
{
    mrs_cco_queue *cco_queue = mrs_get_cco_queue();
    mrs_cco_queue_item *item = TD_NULL;
    mrs_cco_queue_item *temp = TD_NULL;

    ext_unref_param(param);

    mrs_timer_stop(MRS_TIMER_ID_CCO_TO_HOST);

    if (mrs_cco_queue_get_fsm() != FSM_CCO_QUEUE_WAIT_ACK) {
        return EXT_ERR_SUCCESS;
    }

    item = (mrs_cco_queue_item *)mrs_queue_top(&cco_queue->queue);
    if (item == TD_NULL) {
        return EXT_ERR_FAILURE;
    }

    if (item->rx_handle) {
        item->rx_handle(item->param);
    }

    temp = (mrs_cco_queue_item *)mrs_queue_top(&cco_queue->queue);
    if (temp == item) {
        mrs_cco_queue_end(cco_queue);
    }

    return EXT_ERR_SUCCESS;
}

/*
 * 376.2 frame processing function
 */
td_u32 mrs_1376_2_afn_00_f1(mrs_fn_param *param)
{
    return mrs_cco_queue_rx(param);
}

td_u32 mrs_1376_2_afn_00_f2(mrs_fn_param *param)
{
    return mrs_cco_queue_rx(param);
}

/* AFN=01H initialization */
td_u32 mrs_1376_2_afn_01_f1(mrs_fn_param *param)
{
    static td_bool recv_flag = TD_FALSE;

    if (recv_flag == TD_TRUE) {
        return mrs_cco_1376_2_deny_frame(param, MRS_1376_2_DENY_CCO_BUSY);
    }

    recv_flag = TD_TRUE;
    mrs_cco_1376_2_ack_frame(param, 1);
    mrs_timer_start(MRS_TIMER_ID_CCO_REBOOT, MRS_CCO_REBOOT_TIME, EXT_TIMER_TYPE_ONCE);

    return EXT_ERR_SUCCESS;
}

td_u32 mrs_1376_2_afn_01_f2(mrs_fn_param *param)
{
    if (mrs_archives_clear() != EXT_ERR_SUCCESS) {
        return mrs_cco_1376_2_deny_frame(param, MRS_1376_2_DENY_TIMEOUT);
    }

    return mrs_cco_1376_2_ack_frame(param, 0);
}

td_u32 mrs_1376_2_afn_01_f3(mrs_fn_param *param)
{
    return mrs_cco_1376_2_ack_frame(param, 0);
}

/* AFN=02H data transmit */
td_u32 mrs_1376_2_afn_02_f1(mrs_fn_param *param)
{
    mrs_cco_upg_reset_file_rx_timeout(); /* Reset timeout calculator when CCO downloads file */
    return mrs_1376_2_mr_handle(param, MRS_XR_02H_F1);
}

td_u32 mrs_1376_2_afn_02_f255(mrs_fn_param *param)
{
    mrs_cco_upg_reset_file_rx_timeout(); /* Reset timeout calculator when CCO downloads file */
    return mrs_1376_2_mr_handle(param, MRS_XR_02H_F255);
}

/* AFN=03H inquiry */
td_u32 mrs_afn_0301_buf(td_pbyte buffer, td_u16 length)
{
    ext_mdm_user_info inf;

    if (length < 9) { /* buffer length: 9 */
        return EXT_ERR_INVALID_PARAMETER;
    }

    uapi_get_user_info(&inf);
    /* use little-endian */
    *buffer++ = (td_u8)inf.manu_code[0];
    *buffer++ = (td_u8)inf.manu_code[1];
    *buffer++ = (td_u8)inf.chip_code[0];
    *buffer++ = (td_u8)inf.chip_code[1];
    mrs_convert_int_to_bcd(inf.day, buffer++, 1);
    mrs_convert_int_to_bcd(inf.month, buffer++, 1);
    mrs_convert_int_to_bcd(inf.year, buffer++, 1);
    *buffer++ = (td_u8)(inf.sw_ver & 0xff);
    *buffer++ = (td_u8)((inf.sw_ver >> 8) & 0xff); /* low 8 bit */

    return EXT_ERR_SUCCESS;
}

td_u32 mrs_1376_2_afn_03_f1(mrs_fn_param *param)
{
    td_u32 ret;
    td_u8 buf[9] = { 0 }; /* buf length: 9 */

    ret = mrs_afn_0301_buf(buf, sizeof(buf));
    if (ret != EXT_ERR_SUCCESS) {
        return ret;
    }

    return mrs_cco_1376_2_easy_encode(param, buf, sizeof(buf));
}

td_u32 mrs_1376_2_afn_03_f4(mrs_fn_param *param)
{
    mrs_cco_srv_ctx *cco = mrs_cco_get_srv_ctx();

    if (cco == TD_NULL) {
        return EXT_ERR_NO_INITILIZATION;
    }

    return mrs_cco_1376_2_easy_encode(param, cco->main_node, sizeof(cco->main_node));
}


#pragma pack(1)
typedef struct {
    td_u8 comm_mode : 4;  /* bytes of local communication mode: communication mode */
    td_u8 route_type : 1; /* bytes of local communication mode: route manage mode */
    td_u8 node_mode : 1;  /* bytes of local communication mode: measured point info mode */
    td_u8 period_mr : 2;  /* bytes of local communication mode: periodically read meter mode */

    td_u8 trans_delay : 3; /* bytes of local communication mode: supportable for transmitting delay parameter */
    td_u8 fail_switch : 2; /* bytes of local communication mode: failed node change starting mode */
    td_u8 bc_confirm : 1;  /* bytes of local communication mode: affirm braodcast command mode */
    td_u8 bc_cmd_ch : 2;   /* bytes of local communication mode: broadcast command execute mode with channel */

    td_u8 ch_num : 4;         /* bytes of local communication mode:quantity of channel */
    td_u8 power_down_inf : 4; /* bytes of local communication mode:power down info of low voltage grid */

    td_u8 speed_num : 3;  /* bytes of local communication mode: quantity of rate, equal to 1 */
    td_u8 rsv0 : 5;
    td_u8 rsv1;
    td_u8 rsv2;

    td_u8 xr_timeout;                   /* maximum monitoring timeout time of slave node (unit : s) */
    td_u16 bc_cmd_timeout;              /* maximum timeout time of broadcast command (unit : s) */
    td_u16 frame_max_len;               /* maximum supportable length of frame */
    td_u16 upg_pkt_max;                 /* maximum supportable length of a package for file transmit */
    td_u8 upg_wait_time;                /* waitting time of upgrade */
    td_u8 main_node[EXT_METER_ADDR_LEN]; /* address of main node */
    td_u16 meter_max;                   /* maximum supportable quantity of slave nodes */
    td_u16 meter_num;                   /* quantity of present secondary nodes */
    td_u8 release_date[3];              /* release date of communication module protocol (BCD) ,3bytes */
    td_u8 backup_date[3];               /* latest filed date of communication module protocol (BCD) , 3bytes */
    td_u8 version_inf[9];               /* manufacturer code and version info of communication module, 9bytes */
    td_u16 speed_rate : 15;             /* communication rate, equal to 1 */
    td_u16 speed_unit : 1;              /* rate unit identification */
} mrs_cco_local_runtime_inf;
#pragma pack()

td_u32 mrs_afn_0310_buf(td_pbyte *buffer, td_u16 *length)
{
    mrs_cco_local_runtime_inf *inf = (mrs_cco_local_runtime_inf *)mrs_malloc(sizeof(mrs_cco_local_runtime_inf));
    mrs_cco_srv_ctx *cco = mrs_cco_get_srv_ctx();

    if (inf == TD_NULL) {
        return EXT_ERR_MALLOC_FAILUE;
    }

    if (cco == TD_NULL) {
        mrs_free(inf);
        return EXT_ERR_NO_INITILIZATION;
    }

    (td_void) memset_s(inf, sizeof(mrs_cco_local_runtime_inf), 0, sizeof(mrs_cco_local_runtime_inf));

    inf->comm_mode = 2;  /* 2: broadband power line communication */
    inf->route_type = 1; /* with route management feature */
    inf->node_mode = 1;  /* need send slave node info */
    inf->period_mr = 1;  /* 01 means communication module only support periodically read meter by concentrator */

    inf->trans_delay = 0; /* supportable for transmit delay parameter: unsupported */
    inf->fail_switch = 2; /* 2: failed node starting mode changed by concentrator */
    inf->bc_confirm = 1;  /* reply affirm frame before executing broadcast command */
    inf->bc_cmd_ch = 0;   /* execute broadcast command without channel identification */

    inf->ch_num = 1;    /* quantity of carrier channels */
    inf->speed_num = 1; /* quantity of rate */

    inf->xr_timeout = MRS_CFG_XR_13762_TIMEOUT;
    inf->bc_cmd_timeout = MRS_CFG_BC_TIMEOUT;
    inf->frame_max_len = MRS_DATA_BUFFER_SIZE;
    inf->upg_pkt_max = MRS_CFG_UPG_PKT_MAX;
    inf->upg_wait_time = MRS_CFG_UPG_WAIT_TIME;
    (td_void) memcpy_s(inf->main_node, sizeof(inf->main_node), cco->main_node, sizeof(cco->main_node));
    inf->meter_max = MRS_CFG_METER_MAX;
    inf->meter_num = mrs_archives_get_number();
    inf->release_date[0] = 0x18;
    inf->release_date[1] = 0x09;
    inf->release_date[2] = 0x13; /* index 2: filled with 0x13 */
    inf->backup_date[0] = 0x18;
    inf->backup_date[1] = 0x09;
    inf->backup_date[2] = 0x13; /* index 2: filled with 0x13 */
    if (mrs_afn_0301_buf(inf->version_inf, sizeof(inf->version_inf)) != EXT_ERR_SUCCESS) {
        mrs_free(inf);
        return EXT_ERR_INVALID_PARAMETER;
    }
    inf->speed_rate = 0;
    inf->speed_unit = 1;

    *buffer = (td_pbyte)inf;
    *length = sizeof(mrs_cco_local_runtime_inf);

    return EXT_ERR_SUCCESS;
}

td_u32 mrs_1376_2_afn_03_f10(mrs_fn_param *param)
{
    td_u32 ret;
    td_pbyte buf = TD_NULL;
    td_u16 buf_len = 0;

    ret = mrs_afn_0310_buf(&buf, &buf_len);
    if (ret != EXT_ERR_SUCCESS) {
        return ret;
    }

    ret = mrs_cco_1376_2_easy_encode(param, buf, buf_len);
    mrs_free(buf);

    return ret;
}

td_void mrs_cco_report_afn_03f10(td_void)
{
    mrs_proto_1376_2_encode encode;
    mrs_cco_srv_ctx *cco = mrs_cco_get_srv_ctx();
    td_u32 ret;
    td_pbyte buf = TD_NULL;
    td_u16 buf_len = 0;
    td_u16 payload_len = 0;
    td_pbyte payload = TD_NULL;
    mrs_cco_queue_item *item = TD_NULL;

    ret = mrs_afn_0310_buf(&buf, &buf_len);
    if (ret != EXT_ERR_SUCCESS) {
        return;
    }

    do {
        (td_void) memset_s(&encode, sizeof(encode), 0, sizeof(encode));
        encode.afn = 3; /* afn equal to 3 */
        encode.fn = 10; /* fn equal to 10 */
        encode.seq = ++cco->seq;
        encode.length = buf_len;
        encode.data = buf;

        ret = mrs_proto_1376_2_create_frame(&encode, &payload, &payload_len);
        if (ret != EXT_ERR_SUCCESS) {
            break;
        }

        item = (mrs_cco_queue_item *)mrs_malloc(sizeof(mrs_cco_queue_item) + payload_len);
        if (item == TD_NULL) {
            break;
        }

        (td_void) memset_s(item, sizeof(mrs_cco_queue_item) + payload_len, 0, sizeof(mrs_cco_queue_item) + payload_len);
        item->valid = TD_TRUE;
        item->afn = encode.afn;
        item->fn = encode.fn;
        item->retry_max = 0;
        item->timeout = 500; /* element timeout time: 500 */
        item->data_len = payload_len;

        if (memcpy_s(item->data, item->data_len, payload, payload_len) != EOK) {
            mrs_free(payload);
            mrs_free(buf);
            mrs_free(item);
            return;
        }

        mrs_cco_queue_enqueue(item);
        mrs_cco_queue_active();
    } while (0);

    mrs_free(payload);
    mrs_free(buf);
}

td_u32 mrs_1376_2_afn_03_f11(mrs_fn_param *param)
{
    mrs_proto_1376_2_frame *frame = param->frame;
    td_u8 payload[33] = { 0 }; /* payload length equal to 33 */

    if (frame->data_len != 1) {
        return EXT_ERR_BAD_DATA;
    }

    payload[0] = frame->content[0];
    switch (frame->content[0]) {
        case 0x00:
        case 0x11:
            payload[1] = 0x03; /* 0000 0011 1,2 */
            break;

        case 0x01:
        case 0x12:
            payload[1] = 0x07; /* 0000 0111 1,2,3 */
            break;

        case 0x03:
            payload[1] = 0x09; /* 0000 1001 1,4 */
            payload[2] = 0x86; /* index 2: 0x86: 1000 0110 10,11,16 */
            break;

        case 0x05:
            payload[1] = 0x01; /* 0000 0001 1 */
            payload[2] = 0x80; /* index 2: 0x80: 1000 0000 16 */
            break;

        case 0x13:
        case 0x15:
        case 0xF1:
            payload[1] = 0x01; /* 0000 0001 1 */
            break;

        case 0x06:
            payload[1] = 0x1C; /* 0001 1100 3,4,5 */
            break;

        case 0x10:
            payload[1] = 0x0B; /* 0000 1011 1,2,4 */
            payload[3] = 0x10; /* index 3: 0x10: 0001 0000 21 */
            break;

        default:
            break;
    }

    return mrs_cco_1376_2_easy_encode(param, payload, sizeof(payload));
}

td_u32 mrs_1376_2_afn_03_f16(mrs_fn_param *param)
{
    td_u8 freq = uapi_get_whole_net_work_freq();
    return mrs_cco_1376_2_easy_encode(param, &freq, sizeof(freq));
}

td_u32 mrs_1376_2_afn_05_f1(mrs_fn_param *param)
{
    mrs_cco_srv_ctx *cco = mrs_cco_get_srv_ctx();
    mrs_proto_1376_2_frame *frame = param->frame;
    app_dev_addr_inf nv = { { 0 }, { 0 } };
    td_u32 ret;
    errno_t err;

    if (cco == TD_NULL) {
        return EXT_ERR_NO_INITILIZATION;
    }

    if (frame->data_len != sizeof(cco->main_node)) {
        return mrs_cco_1376_2_deny_frame(param, MRS_1376_2_DENY_LEN_ERR);
    }

    if (memcmp(cco->main_node, frame->content, EXT_METER_ADDR_LEN) == 0) {
        return mrs_cco_1376_2_ack_frame(param, 0);
    }

    err = memcpy_s(cco->main_node, sizeof(cco->main_node), frame->content, frame->data_len);
    if (err != EOK) {
        return mrs_cco_1376_2_deny_frame(param, MRS_1376_2_DENY_TIMEOUT);
    }

    mrs_cco_1376_2_ack_frame(param, 0);

    ret = uapi_nv_read(ID_NV_APP_DEV_ADDR_INF, &nv, sizeof(nv));
    if (ret == EXT_ERR_SUCCESS) {
        mrs_hex_invert(nv.dev_addr, (td_u16)sizeof(nv.dev_addr));
        if (memcmp(nv.dev_addr, cco->main_node, sizeof(cco->main_node)) != 0) {
            td_u8 mac[EXT_METER_ADDR_LEN] = { 0 };

            err = memcpy_s(nv.dev_addr, sizeof(nv.dev_addr), cco->main_node, sizeof(cco->main_node));
            if (err == EOK) {
                mrs_hex_invert(nv.dev_addr, (td_u16)sizeof(nv.dev_addr));
                uapi_nv_write(ID_NV_APP_DEV_ADDR_INF, &nv, sizeof(nv));
            }

            mrs_convert_meter_to_mac(cco->main_node, sizeof(cco->main_node), mac, sizeof(mac));
            uapi_set_cco_mac_addr(mac);
        }
    }

    return EXT_ERR_SUCCESS;
}

/* AFN=05H control command */
td_u32 mrs_1376_2_afn_05_f3(mrs_fn_param *param)
{
    td_u32 ret;
    mrs_proto_1376_2_frame *frame = param->frame;
    td_u16 data_len = frame->data_len;
    td_u8 *content = frame->content;
    td_u8 deny_code = MRS_1376_2_DENY_TIMEOUT;

    do {
        if (data_len != MRS_CCO_BC_CTRL_LEN + MRS_CCO_BC_MSG_LEN_SIZE + content[1]) {
            deny_code = MRS_1376_2_DENY_LEN_ERR;
            ret = EXT_ERR_BAD_DATA;
            break;
        }

        ret = mrs_cco_broadcast_proc(&deny_code, (td_pbyte)&content[MRS_CCO_BC_CTRL_LEN + MRS_CCO_BC_MSG_LEN_SIZE],
                                     content[1]);
    } while (0);

    if (ret != EXT_ERR_SUCCESS) {
        mrs_cco_1376_2_deny_frame(param, deny_code);
    } else {
        mrs_cco_1376_2_ack_frame(param, MRS_CFG_BC_TIMEOUT);
    }

    return EXT_ERR_SUCCESS;
}

td_u32 mrs_1376_2_afn_05_f16(mrs_fn_param *param)
{
    mrs_proto_1376_2_frame *frame = param->frame;
    td_u8 new_freq;
    td_u8 old_freq;

    if (frame->data_len != 1) {
        return mrs_cco_1376_2_deny_frame(param, MRS_1376_2_DENY_LEN_ERR);
    }

    new_freq = frame->content[0];
    /* 1: 2.5M-5.7M_IS
    *  5: 0.5M-3.7M_IS
    */
    if ((new_freq != 1) && (new_freq != 5)) { /* frequency must be 5: 0.5M-3.7M_IS */
        return mrs_cco_1376_2_deny_frame(param, MRS_1376_2_DENY_INVALID_DATA);
    }

    old_freq = uapi_get_whole_net_work_freq();
    if (new_freq != old_freq) {
        uapi_set_whole_net_work_freq(frame->content[0], 300); /* change frequency delay time: 300 */
        return mrs_cco_1376_2_ack_frame(param, 300);        /* wait time: 300 */
    }

    return mrs_cco_1376_2_ack_frame(param, 0);
}

td_u32 mrs_1376_2_afn_05_f255(mrs_fn_param *param)
{
    /* Long message type */
    td_u32 ret;
    mrs_proto_1376_2_frame *frame = param->frame;
    td_u16 data_len = frame->data_len;
    td_u16 msg_len;
    td_u8 *content = frame->content;
    td_u8 deny_code = MRS_1376_2_DENY_TIMEOUT;

    do {
        msg_len = uapi_make_u16(content[1], content[2]); /* make 1 and 2 bytes into U16 */
        if (data_len != MRS_CCO_BC_CTRL_LEN + MRS_CCO_BC_MSG_LONG_LEN_SIZE + msg_len) {
            deny_code = MRS_1376_2_DENY_LEN_ERR;
            ret = EXT_ERR_BAD_DATA;
            break;
        }

        ret = mrs_cco_broadcast_proc(&deny_code,
                                     (td_pbyte)&content[MRS_CCO_BC_CTRL_LEN + MRS_CCO_BC_MSG_LONG_LEN_SIZE], msg_len);
    } while (0);

    if (ret != EXT_ERR_SUCCESS) {
        mrs_cco_1376_2_deny_frame(param, deny_code);
    } else {
        mrs_cco_1376_2_ack_frame(param, MRS_CFG_BC_TIMEOUT);
    }

    return EXT_ERR_SUCCESS;
}

/* AFN=10H route inquiry */
td_u32 mrs_1376_2_afn_10_f1(mrs_fn_param *param)
{
    td_u16 slave_num[2] = { 0 }; /* slave number use 2 bytes */

    slave_num[0] = mrs_archives_get_number();
    slave_num[1] = ARCHIVES_NUM_MAX;

    return mrs_cco_1376_2_easy_encode(param, (td_pbyte)slave_num, sizeof(slave_num));
}

typedef struct {
    td_u8 node_addr[EXT_METER_ADDR_LEN];
    td_u8 relay_level : 4;
    td_u8 quality : 4;
    td_u8 phase : 3;
    td_u8 protocol : 3;
    td_u8 rsv : 2;
} mrs_afn_10_f2_node;

td_u32 mrs_afn_10_f2_get_numbers(td_u16 *start_num, td_u8 *meter_num, td_u16 meters_total)
{
    td_u32 ret = EXT_ERR_SUCCESS;
    td_u8 meter = *meter_num;
    td_u16 start = *start_num;
    do {
        /* if starting sequence start from 0, the quantity is one fewer than start from 1 */
        if (start == 0) {
            if (meter > 0) {
                meter--;
            }
        } else {
            start--;
        }

        /* Calc enough memory */
        if (meters_total <= start) {
            ret = EXT_ERR_FAILURE;
            break;
        }

        if (meters_total < (start + (td_u16)meter)) {
            meter = (td_u8)(meters_total - start);
        }

        if (meter > MRS_3762_SEC_NODE_MAX_NUM) {
            /* Out of range */
            meter = MRS_3762_SEC_NODE_MAX_NUM;
        }
    } while (TD_FALSE);
    *meter_num = meter;
    *start_num = start;
    return ret;
}
td_u32 mrs_afn_10_f2_fill_reply_content(td_u8 *reply_content, td_u16 reply_content_len,
                                        td_u16 start_num, td_u8 meter_num)
{
    td_u32 ret;
    meter_item *meter_items = TD_NULL; /* < The meter list after index */
    td_u8 i;

    do {
        if (reply_content == TD_NULL) {
            ret = EXT_ERR_MALLOC_FAILUE;
            break;
        }

        /* Get meter addresses from archives */
        ret = (td_u32)memset_s(reply_content, reply_content_len, 0, reply_content_len);
        if (ret != EOK) {
            ret = EXT_ERR_FAILURE;
            break;
        }

        meter_items = mrs_archives_get_by_idx(start_num);
        if (meter_items == TD_NULL) {
            ret = EXT_ERR_BAD_DATA;
            break;
        }

        /* Build reply */
        for (i = 0; i < meter_num; i++) {
            mrs_afn_10_f2_node node;
            ret = (td_u32)memset_s(&node, sizeof(node), 0, sizeof(node));
            ret |= (td_u32)memcpy_s(node.node_addr, sizeof(node.node_addr),
                                    meter_items[i].addr, sizeof(meter_items[i].addr));
            node.protocol = (meter_items[i].protocol & 0x7);
            ret |= (td_u32)memcpy_s(reply_content + MRS_1376_2_SEC_NODE_INFO_LEN + i * sizeof(mrs_afn_10_f2_node),
                                    reply_content_len - (MRS_1376_2_SEC_NODE_INFO_LEN + i * sizeof(mrs_afn_10_f2_node)),
                                    &node, sizeof(node));
            if (ret != EOK) {
                ret = EXT_ERR_FAILURE;
                break;
            }
        }
    } while (TD_FALSE);

    return ret;
}

td_u32 mrs_1376_2_afn_10_f2(mrs_fn_param *param)
{
    td_u32 ret = EXT_ERR_BAD_DATA;
    mrs_proto_1376_2_frame *frame = param->frame;
    td_u16 meters_total = mrs_archives_get_number(); /* < Totoal meters in the archives */
    td_u16 start_num, reply_content_len;                            /* < Start meter index */
    td_u16 data_len = frame->data_len;
    td_u8 meter_num; /* < Required meter number */
    td_u8 *content = frame->content;
    td_u8 *reply_content = 0;

    do {
        if (data_len < MRS_1376_2_SEC_NODE_INFO_LEN) {
            break;
        }

        if (meters_total == 0) {
            break;
        }

        /* Analyse content */
        start_num = uapi_make_u16(content[0], content[1]);
        meter_num = content[2]; /* index 2 means meter number */
        ret = mrs_afn_10_f2_get_numbers(&start_num, &meter_num, meters_total);
        if (ret != EXT_ERR_SUCCESS) {
            break;
        }
        if (meter_num > MRS_3762_SEC_NODE_MAX_NUM) {
            /* Out of range */
            meter_num = MRS_3762_SEC_NODE_MAX_NUM;
        }
        /* Length = total meter number(2 bytes) + meter number replied(1 byte) + meter info */
        reply_content_len = 2 + 1 + meter_num * sizeof(mrs_afn_10_f2_node); /* total meter 2B, number replied 1B */
        reply_content = mrs_malloc(reply_content_len);
        ret = mrs_afn_10_f2_fill_reply_content(reply_content, reply_content_len, start_num, meter_num);
        if (ret != EXT_ERR_SUCCESS) {
            break;
        }
    } while (0);

    if (ret != EXT_ERR_SUCCESS) {
        mrs_free(reply_content);
        meter_num = 0;
        reply_content_len = MRS_1376_2_SEC_NODE_INFO_LEN;
        reply_content = mrs_malloc(reply_content_len);
        if (reply_content == TD_NULL) {
            return EXT_ERR_MALLOC_FAILUE;
        }
    }

    if (reply_content != TD_NULL) {
        if (memcpy_s(reply_content, reply_content_len, &meters_total, sizeof(meters_total)) == EOK) {
            reply_content[2] = meter_num; /* index 2 means meter number */
            mrs_cco_1376_2_easy_encode(param, reply_content, reply_content_len);
        }
    }

    mrs_free(reply_content);

    return EXT_ERR_SUCCESS;
}

td_u32 mrs_1376_2_afn_10_f4(mrs_fn_param *param)
{
    mrs_cco_srv_ctx *cco = mrs_cco_get_srv_ctx();
    mrs_cco_evt_ctx *evt = mrs_cco_get_evt_ctx();
    td_u8 buffer[16] = { 0 }; /* buffer length 16 */
    td_u16 meter_num = mrs_archives_get_number();

    /*
     * bytes of running status
     * [D0:D0] route finished identification: 1: route study finished; 0:unfinished
     * [D1:D1] working identification: 1: in process of working; 0:working stoped [one more definition in beijing:
     * working includes transformer identify]
     * [D2:D2] report events identification: 1: events reported by slave nodes; 0: no events reported
     * [D3:D3] upgrade identification: 1: in process of upgrading; 0: upgrade stopped
     * [D7:D4] error correction code
 */
    buffer[0] = (td_u8)((evt->acd_flg << 2) | (cco->chl_status)); /* bit2: evt report flag */
    if (mrs_cco_upg_is_idle() != TD_TRUE) {
        buffer[0] |= 0x08;
    }

    buffer[1] = (td_u8)(meter_num & 0xff);
    buffer[2] = (td_u8)((meter_num >> 8) & 0xff); /* meter number 2bytes, index 2: high 8 bits */

    /*
     * working switch
     * [D0:D0] working status: 1:study; 0:read meter
     * [D1:D1] allowed register status: 1: allow; 0: not allow
     * [D7:D2] reserved */
    buffer[7] = 0x2; /* index 7: working switch 0x2 */

    buffer[13] = 0x08; /* index 13:working steps,equal to 08 */
    buffer[14] = 0x08; /* index 14:working steps,equal to 08 */
    buffer[15] = 0x08; /* index 15:working steps,equal to 08 */

    return mrs_cco_1376_2_easy_encode(param, buffer, sizeof(buffer));
}

td_void mrs_afn_10_f27_get_numbers(td_u16 *start_num, td_u8 *node_num)
{
    td_u16 topo_total = uapi_get_topo_node_num();
    td_u8 node_cnt = *node_num;

    if (node_cnt > MRS_AFN_QUERY_NODE_MAX_NUM) {
        node_cnt = MRS_AFN_QUERY_NODE_MAX_NUM;
    }

    if (*start_num == 0) {
        *start_num = 1;
        if (node_cnt > 0) {
            node_cnt--;
        }
    }

    if (topo_total < *start_num) {
        node_cnt = 0;
    } else {
        node_cnt = (td_u8)uapi_min(node_cnt, (topo_total - *start_num));
    }

    *node_num = node_cnt;
}

td_u16 mrs_afn_10_f27_get_start_index(const ext_mdm_nm_topo_node_info *topo_info, td_u16 start_num)
{
    td_u16 i = 0;
    td_u16 num = 0;
    for (; (i < EXT_MDM_NM_TOPO_NODE_NUM_MAX) && (num < start_num); i++) {
        if (topo_info[i].state != STATE_NOT_USED) {
            num++;
        }
    }
    return i;
}

td_void mrs_afn_10_f27_get_node_info(ext_mdm_nm_topo_node_info *topo_info, const td_u8 *phase_info,
                                     td_u16 i, mrs_afn_10_f27_node *node)
{
    td_u16 proxy_tei;
    td_u8 *parent_mac = TD_NULL;

    /* Get mac */
    (td_void) memcpy_s(node->node_addr, sizeof(node->node_addr), topo_info[i].mac, sizeof(topo_info[i].mac));
    mrs_hex_invert(node->node_addr, sizeof(node->node_addr));

    /* Get short address */
    node->node_tei = i + 1;

    /* Get parent address */
    proxy_tei = topo_info[i].proxy_tei;
    if (proxy_tei > 0) {
        proxy_tei--;
    }

    if (topo_info[proxy_tei].state != STATE_NOT_USED) {
        parent_mac = topo_info[proxy_tei].mac;
    } else {
        /* Use CCO address instead */
        parent_mac = topo_info[0].mac;
    }

    if (memcpy_s(node->node_parent_addr, sizeof(node->node_parent_addr),
                 parent_mac, MRS_METER_ADDR_LEN) != EOK) {
        return;
    }
    mrs_hex_invert(node->node_parent_addr, sizeof(node->node_parent_addr));

    /* Get signal qulity */
    node->signal = uapi_get_rcv_atten_by_tei(node->node_tei);

    /* Get network status */
    node->stat_nw = 1;
    if (uapi_topo_get_state_by_tei(node->node_tei) != EXT_TP_IN_USE) {
        node->stat_nw = 0;
    }

    /* Get location */
    node->longitude = 0xFFFFFFFF;
    node->latitude = 0xFFFFFFFF;

    /* Get pharse */
    if (phase_info == TD_NULL) {
        node->phase = 0;
    } else if (((phase_info[node->node_tei] & 0xC) >> 2) == EXT_MAC_PHASE_IDENTIFY_STATE_OVER) { /* bit 2&3:0xC */
        node->phase = phase_info[node->node_tei] & 0x3;
    } else {
        /* Skip the first invalid value in the phase results */
        node->phase = 0;
    }
}

td_u32 mrs_1376_2_afn_10_f27(mrs_fn_param *param)
{
    td_u32 ret = EXT_ERR_SUCCESS;
    ext_mdm_nm_topo_node_info *topo_info = TD_NULL;
    td_u8 *phase_info = uapi_get_phase_result_table();
    td_pbyte payload = TD_NULL;
    td_u16 i, num, start_num, payload_len;
    mrs_afn_10_f27_node node = { 0 };
    td_u8 node_num;

    if (param->frame->data_len != 3) { /* 3: start num 2B + node num 1B */
        return EXT_ERR_BAD_DATA;
    }

    start_num = uapi_make_u16(param->frame->content[0], param->frame->content[1]);
    node_num = param->frame->content[2]; /* index 2: node number */

    mrs_afn_10_f27_get_numbers(&start_num, &node_num);
    node_num = (node_num > MRS_AFN_QUERY_NODE_MAX_NUM) ? MRS_AFN_QUERY_NODE_MAX_NUM : node_num;

    do {
        td_u16 offset = 1;

        payload_len = 1 + node_num * (sizeof(mrs_afn_10_f27_node) - MRS_AFN_10_F27_NODE_PAD);
        payload = (td_u8 *)mrs_malloc(payload_len);
        if (payload == TD_NULL) {
            ret = EXT_ERR_FAILURE;
            break;
        }

        payload[0] = node_num;
        if (node_num == 0) {
            break;
        }

        uapi_get_topo_info(&topo_info);
        if (topo_info == TD_NULL) {
            ret = EXT_ERR_FAILURE;
            break;
        }

        /* Move to right start index in topo */
        i = mrs_afn_10_f27_get_start_index(topo_info, start_num);

        for (num = 0; (i < EXT_MDM_NM_TOPO_NODE_NUM_MAX) && (num < node_num); i++) {
            if (topo_info[i].state != STATE_NOT_USED) {
                mrs_afn_10_f27_get_node_info(topo_info, phase_info, i, &node);
                memcpy_s(payload + offset, payload_len - offset, &node, (sizeof(node) - MRS_AFN_10_F27_NODE_PAD));
                offset += (sizeof(node) - MRS_AFN_10_F27_NODE_PAD);
                num++;
            }
        }
    } while (0);

    if (ret != EXT_ERR_SUCCESS) {
        mrs_cco_1376_2_deny_frame(param, MRS_1376_2_DENY_METER_NOT_EXIST);
    } else {
        mrs_cco_1376_2_easy_encode(param, payload, payload_len);
    }

    mrs_free(payload);

    return EXT_ERR_SUCCESS;
}

td_u32 mrs_afn10_query_data_check(mrs_fn_param *afn_param, td_u8 *query_num, td_u16 *start_seq,
    td_bool is_create_deny)
{
    td_u8 *content = afn_param->frame->content;
    td_u16 content_len = afn_param->frame->data_len;
    td_u32 ret = EXT_ERR_SUCCESS;
    mrs_proto_1376_2_deny_code deny_data = MRS_1376_2_DENY_INVALID_DATA;

    do {
        if (content_len != MRS_CCO_AFN10_DOWN_DATA_LEN) {
            deny_data = MRS_1376_2_DENY_LEN_ERR;
            ret = EXT_ERR_BAD_DATA;
            break;
        }

        if (memcpy_s(start_seq, sizeof(td_u16), content, sizeof(td_u16)) != EOK) {
            return EXT_ERR_MEMCPY_FAIL;
        }
        if (!(*start_seq)) {
            ret = EXT_ERR_INVALID_PARAMETER;
            break;
        }

        *query_num = *(content + MRS_CCO_AFN10_QUERY_NUM_OFFSET);
        if (*query_num > MRS_AFN_QUERY_TOPO_NODE_NUM_MAX) {
            *query_num = MRS_AFN_QUERY_TOPO_NODE_NUM_MAX;
        }
    } while (0);

    if (is_create_deny == TD_TRUE && (ret == EXT_ERR_BAD_DATA || ret == EXT_ERR_INVALID_PARAMETER)) {
        ret = mrs_cco_1376_2_deny_frame(afn_param, deny_data);
        if (ret == EXT_ERR_SUCCESS) {
            ret = EXT_ERR_SKIP;
        }
    }

    return ret;
}

td_u32 mrs_query_topo_down_check(td_bool *start_flag, mrs_fn_param *param, td_u16 *start_seq, td_u8 *query_num)
{
    td_u32 ret;
    mrs_proto_1376_2_deny_code deny_data = MRS_1376_2_DENY_INVALID_DATA;

    do {
        ret = mrs_afn10_query_data_check(param, query_num, start_seq, TD_FALSE);
        if (ret != EXT_ERR_SUCCESS) {
            if (ret == EXT_ERR_BAD_DATA) {
                deny_data = MRS_1376_2_DENY_LEN_ERR;
            }
            break;
        }

        if (((*start_flag) == TD_FALSE) && (*start_seq != MRS_CCO_START_SEQ_ONE)) {
            ret = EXT_ERR_INVALID_PARAMETER;
            break;
        }

        if (*start_seq == MRS_CCO_START_SEQ_ONE) {
            (*start_flag) = TD_TRUE;
        }
    } while (0);

    if (ret == EXT_ERR_BAD_DATA || ret == EXT_ERR_INVALID_PARAMETER) {
        ret = mrs_cco_1376_2_deny_frame(param, deny_data);
        if (ret == EXT_ERR_SUCCESS) {
            ret = EXT_ERR_SKIP;
        }
    }

    return ret;
}

td_u32 mrs_afn_10_f230_fill_node_info(ext_mdm_nm_topo_node_info *topo_info, td_u8 *payload, td_u16 *data_offset,
    td_u16 my_tei)
{
    td_u8 role;
    td_u16 my_proxy_tei;
    td_u8 meter_address[EXT_PLC_MAC_ADDR_LEN] = { 0 };
    td_u16 current_offset = *data_offset;

    if (memcpy_s(meter_address, sizeof(meter_address), topo_info->mac, EXT_PLC_MAC_ADDR_LEN) != EOK) {
        return EXT_ERR_MEMCPY_FAIL;
    }
    mrs_hex_invert(meter_address, sizeof(meter_address));
    if (meter_address[0] == 0xFE) {
        meter_address[0] = 0x00;
    }
    if (memcpy_s(payload + current_offset, EXT_PLC_MAC_ADDR_LEN, meter_address, sizeof(meter_address)) != EOK) {
        return EXT_ERR_MEMCPY_FAIL;
    }

    current_offset += EXT_PLC_MAC_ADDR_LEN;
    if (memcpy_s(payload + current_offset, sizeof(td_u16), &my_tei, sizeof(my_tei)) != EOK) {
        return EXT_ERR_MEMCPY_FAIL;
    }
    current_offset += (td_u16)sizeof(td_u16);
    my_proxy_tei = topo_info->proxy_tei;
    if (memcpy_s(payload + current_offset, sizeof(td_u16), &my_proxy_tei, sizeof(my_proxy_tei)) != EOK) {
        return EXT_ERR_MEMCPY_FAIL;
    }
    current_offset += (td_u16)sizeof(td_u16);
    role = topo_info->ability;
    *(payload + current_offset) = (td_u8)((role << MRS_CCO_TOPO_ROLE_OFFSET) + topo_info->level);
    current_offset += (td_u16)sizeof(td_u8);

    *data_offset = current_offset;

    return EXT_ERR_SUCCESS;
}

td_u32 mrs_afn_10_f230_fill_up_data(td_u8 *payload, td_u16 start_seq, td_u8 need_node_num, td_u8 *actual_report_num)
{
    ext_mdm_nm_topo_node_info *topo_info = TD_NULL;
    td_u16 online_sta_num, find_index;
    td_u16 data_offset = 0;
    td_u8 report_num = 0;
    td_u16 max_tei = uapi_topo_get_max_tei();

    uapi_get_topo_info(&topo_info);
    if (topo_info == TD_NULL) {
        return EXT_ERR_FAILURE;
    }
    online_sta_num = uapi_get_topo_node_num();
    if (memcpy_s(payload, sizeof(td_u16), &online_sta_num, sizeof(online_sta_num)) != EOK) {
        return EXT_ERR_MEMCPY_FAIL;
    }
    data_offset += (td_u16)sizeof(td_u16);
    if (memcpy_s(payload + data_offset, sizeof(td_u16), &start_seq, sizeof(start_seq)) != EOK) {
        return EXT_ERR_MEMCPY_FAIL;
    }
    data_offset += (td_u16)sizeof(start_seq) + (td_u16)sizeof(report_num);
    for (find_index = start_seq; (online_sta_num != 0) && find_index < (need_node_num + start_seq); find_index++) {
        if (max_tei < find_index) {
            break;
        }
        if (topo_info[find_index - 1].state == STATE_NOT_USED) {
            continue;
        }
        if (mrs_afn_10_f230_fill_node_info(&topo_info[find_index - 1], payload, &data_offset,
            find_index) != EXT_ERR_SUCCESS) {
            return EXT_ERR_FAILURE;
        }

        report_num++;
    }

    *(payload + MRS_CCO_TOPO_REPORT_NUM_OFFSET) = report_num;
    *actual_report_num = report_num;

    return EXT_ERR_SUCCESS;
}
/* Querying Network Topology Information */
td_u32 mrs_1376_2_afn_10_f230(mrs_fn_param *param)
{
    td_u32 ret;
    td_u16 start_seq = 0;
    td_u8 need_query_num = 0;
    td_u8 actual_report_num = 0;
    td_u16 payload_len;
    td_u8 *payload = TD_NULL;
    EXT_PRV td_bool mrs_afn_10_f230_state = TD_FALSE;

    do {
        ret = mrs_query_topo_down_check(&mrs_afn_10_f230_state, param, &start_seq, &need_query_num);
        if (ret == EXT_ERR_SKIP) {
            return EXT_ERR_SUCCESS;
        } else if (ret != EXT_ERR_SUCCESS) {
            return ret;
        }
        payload_len = MRS_AFN_10_F230_DATA_FIX_LEN + need_query_num * MRS_AFN_10_F230_TOPO_INFO_LEN;
        payload = (td_u8 *)mrs_malloc(payload_len);
        if (payload == TD_NULL) {
            return EXT_ERR_NOT_ENOUGH_MEMORY;
        }
        (td_void)memset_s(payload, payload_len, 0, payload_len);
        ret = mrs_afn_10_f230_fill_up_data(payload, start_seq, need_query_num, &actual_report_num);
    } while (0);

    if (ret == EXT_ERR_SUCCESS) {
        td_u16 reply_len = (td_u16)(MRS_AFN_10_F230_DATA_FIX_LEN +
            actual_report_num * MRS_AFN_10_F230_TOPO_INFO_LEN);
        mrs_cco_1376_2_easy_encode(param, payload, reply_len);
    }

    mrs_free(payload);

    return ret;
}

/* AFN=11H route settings */
td_u32 mrs_1376_2_afn_11_f1(mrs_fn_param *param)
{
    td_u32 ret;
    td_u8 deny_code = MRS_1376_2_DENY_TIMEOUT;
    mrs_proto_1376_2_frame *frame = param->frame;
    td_u16 data_len = frame->data_len;
    td_u8 *content = frame->content;
    td_u8 add_meter_num = content[0];

    do {
        if (data_len < MRS_1376_2_SEC_NODE_NUM_LEN_MIN) {
            deny_code = MRS_1376_2_DENY_LEN_ERR;
            ret = EXT_ERR_BAD_DATA;
            break;
        }

        if (data_len != (add_meter_num * MRS_1376_2_SEC_NODE_ITEM_SIZE + 1)) {
            deny_code = MRS_1376_2_DENY_LEN_ERR;
            ret = EXT_ERR_BAD_DATA;
            break;
        }

        if (add_meter_num == 0) {
            deny_code = MRS_1376_2_DENY_INVALID_DATA;
            ret = EXT_ERR_BAD_DATA;
            break;
        }

        content += 1;
        ret = mrs_archives_append(content, add_meter_num);
        if (ret != EXT_ERR_SUCCESS) {
            if (ret == EXT_ERR_FULL) {
                deny_code = MRS_1376_2_DENY_TIMEOUT;
            } else {
                deny_code = MRS_1376_2_DENY_CCO_BUSY;
            }
        }
    } while (0);

    if (ret != EXT_ERR_SUCCESS) {
        mrs_cco_1376_2_deny_frame(param, deny_code);
    } else {
        mrs_cco_1376_2_ack_frame(param, 0);
    }

    return EXT_ERR_SUCCESS;
}

td_u32 mrs_1376_2_afn_11_f2(mrs_fn_param *param)
{
    td_u32 ret;
    td_u8 deny_code = MRS_1376_2_DENY_TIMEOUT;
    mrs_proto_1376_2_frame *frame = param->frame;
    td_u16 data_len = frame->data_len;
    td_u8 *content = frame->content;
    td_u8 del_meter_num = content[0];
    td_pbyte del_meter_list = content + 1;

    do {
        if (data_len < MRS_1376_2_SEC_NODE_ITEM_SIZE) {
            deny_code = MRS_1376_2_DENY_LEN_ERR;
            ret = EXT_ERR_BAD_DATA;
            break;
        }

        if (del_meter_num == 0) {
            /* Delete nothing */
            deny_code = MRS_1376_2_DENY_INVALID_DATA;
            ret = EXT_ERR_BAD_DATA;
            break;
        }

        if (data_len != (del_meter_num * ARCHIVES_METER_ADDR_SIZE + 1)) {
            deny_code = MRS_1376_2_DENY_LEN_ERR;
            ret = EXT_ERR_BAD_DATA;
            break;
        }

        ret = mrs_archives_delete(del_meter_list, del_meter_num);
        if (ret == EXT_ERR_NOT_FOUND) {
            deny_code = MRS_1376_2_DENY_METER_NOT_EXIST;
            break;
        }
    } while (0);

    if (ret != EXT_ERR_SUCCESS) {
        mrs_cco_1376_2_deny_frame(param, deny_code);
    } else {
        mrs_cco_1376_2_ack_frame(param, 0);
    }

    return EXT_ERR_SUCCESS;
}

td_u32 mrs_1376_2_afn_11_f5(mrs_fn_param *param)
{
    mrs_proto_1376_2_frame *frame = param->frame;
    td_u32 ret;
    td_pbyte data = frame->content;
    td_u16 data_len = frame->data_len;
    td_u8 deny = MRS_1376_2_DENY_TIMEOUT;

    do {
        td_u32 duration;

        if (data_len != 10) { /* note:length (10B) = time (6B) + duration (4B) */
            ret = EXT_ERR_BAD_DATA;
            deny = MRS_1376_2_DENY_LEN_ERR;
            break;
        }

        ret = mrs_1376_2_time_verify(data, 6); /* date time 6B */
        if (ret != EXT_ERR_SUCCESS) {
            ret = EXT_ERR_BAD_DATA;
            deny = MRS_1376_2_DENY_FORMAT_ERR;
            break;
        }

        duration = ext_make_u32(uapi_make_u16(data[6], data[7]), /* 6&7:low 16b */
                                uapi_make_u16(data[8], data[9]));                  /* 8&9:high 16b */
        ret = mrs_cco_start_tf_identify(duration);
        if (ret != EXT_ERR_SUCCESS) {
            deny = MRS_1376_2_DENY_TIMEOUT;
            break;
        }
    } while (0);

    if (ret == EXT_ERR_SUCCESS) {
        return mrs_cco_1376_2_ack_frame(param, 0);
    } else {
        return mrs_cco_1376_2_deny_frame(param, deny);
    }
}

td_u32 mrs_1376_2_afn_11_f6(mrs_fn_param *param)
{
    mrs_cco_stop_tf_identify();
    return mrs_cco_1376_2_ack_frame(param, 0);
}

/* AFN=12H route control */
td_u32 mrs_1376_2_afn_12_f1(mrs_fn_param *param)
{
    return mrs_cco_1376_2_ack_frame(param, 0);
}

td_u32 mrs_1376_2_afn_12_f2(mrs_fn_param *param)
{
    return mrs_cco_1376_2_ack_frame(param, 0);
}

td_u32 mrs_1376_2_afn_12_f3(mrs_fn_param *param)
{
    return mrs_cco_1376_2_ack_frame(param, 0);
}

/* AFN=13H data transmit by route */
td_u32 mrs_1376_2_afn_13_f1(mrs_fn_param *param)
{
    mrs_cco_upg_reset_file_rx_timeout(); /* Reset timeout calculator when CCO downloads file */
    return mrs_1376_2_mr_handle(param, MRS_XR_13H_F1);
}

td_u32 mrs_1376_2_afn_13_f255(mrs_fn_param *param)
{
    mrs_cco_upg_reset_file_rx_timeout(); /* Reset timeout calculator when CCO downloads file */
    return mrs_1376_2_mr_handle(param, MRS_XR_13H_F255);
}

/* AFN=F1H parallel read meter */
td_u32 mrs_1376_2_afn_f1_f1(mrs_fn_param *param)
{
    return mrs_1376_2_mr_handle(param, MRS_PR_F1H_F1);
}

td_u32 mrs_cco_get_mr_data_13_f1(mrs_proto_1376_2_frame *frame, td_pbyte *data, td_u16 *data_len, td_u8 *proto,
                                 td_u8 *deny)
{
    td_u8 sub_node_num;
    td_u8 length;

    if (frame->data_len < 4) { /* length must more than 4B */
        *deny = MRS_1376_2_DENY_LEN_ERR;
        return EXT_ERR_BAD_DATA;
    }

    *proto = frame->content[0];
    sub_node_num = frame->content[2]; /* index 2: node number */
    length = frame->content[3];       /* index 3: length */

    if (sub_node_num != 0) {
        *deny = MRS_1376_2_DENY_FORMAT_ERR;
        return EXT_ERR_NOT_SUPPORT;
    }

    if ((frame->data_len != length + 4) || (length == 0)) { /* data length:  4B + data length */
        *deny = MRS_1376_2_DENY_LEN_ERR;
        return EXT_ERR_BAD_DATA;
    }

    *data = frame->content + 4; /* data length:  4B + data length */
    *data_len = length;

    return EXT_ERR_SUCCESS;
}

td_u32 mrs_cco_get_mr_data_13_f255(mrs_proto_1376_2_frame *frame, td_pbyte *data, td_u16 *data_len, td_u8 *proto,
                                   td_u8 *deny)
{
    td_u8 sub_node_num;
    td_u16 length;

    if (frame->data_len < 5) { /* data length:  5B + data length */
        *deny = MRS_1376_2_DENY_LEN_ERR;
        return EXT_ERR_BAD_DATA;
    }

    *proto = frame->content[0];
    sub_node_num = frame->content[2];                          /* index 2: node number */
    length = uapi_make_u16(frame->content[3], frame->content[4]); /* index 3&4: length */

    if (sub_node_num != 0) {
        *deny = MRS_1376_2_DENY_FORMAT_ERR;
        return EXT_ERR_NOT_SUPPORT;
    }

    if ((frame->data_len != length + 5) || (length == 0)) { /* data length:  5B + data length */
        *deny = MRS_1376_2_DENY_LEN_ERR;
        return EXT_ERR_BAD_DATA;
    }

    *data = frame->content + 5; /* data length:  5B + data length */
    *data_len = length;

    return EXT_ERR_SUCCESS;
}

td_u32 mrs_cco_get_mr_data_02_f1(mrs_proto_1376_2_frame *frame, td_pbyte *data, td_u16 *data_len, td_u8 *proto,
                                 td_u8 *deny)
{
    td_u8 length = frame->content[1];

    if ((frame->data_len != length + 2) || (length == 0)) { /* data length:  2B + data length */
        *deny = MRS_1376_2_DENY_LEN_ERR;
        return EXT_ERR_BAD_DATA;
    }

    *proto = frame->content[0];
    *data = frame->content + 2; /* data length:  2B + data length */
    *data_len = length;

    return EXT_ERR_SUCCESS;
}

td_u32 mrs_cco_get_mr_data_02_f255(mrs_proto_1376_2_frame *frame, td_pbyte *data, td_u16 *data_len, td_u8 *proto,
                                   td_u8 *deny)
{
    td_u16 length = uapi_make_u16(frame->content[1], frame->content[2]); /* index 1&2: length */
    if ((frame->data_len != length + 3) || (length == 0)) { /* data length:  3B + data length */
        *deny = MRS_1376_2_DENY_LEN_ERR;
        return EXT_ERR_BAD_DATA;
    }

    *proto = frame->content[0];
    *data = frame->content + 3; /* data length:  3B + data length */
    *data_len = length;

    return EXT_ERR_SUCCESS;
}

typedef struct {
    td_u8 protocol;
    td_u8 rsv;
    td_u16 length;
    td_u8 data[0];
} mrs_1376_2_pr_dn;

td_u32 mrs_cco_get_mr_data_f1_f1(mrs_proto_1376_2_frame *frame, td_pbyte *data, td_u16 *data_len, td_u8 *proto,
                                 td_u8 *deny)
{
    mrs_1376_2_pr_dn *head = (mrs_1376_2_pr_dn *)frame->content;

    if ((frame->data_len != sizeof(mrs_1376_2_pr_dn) + head->length) || (head->length == 0)) {
        *deny = MRS_1376_2_DENY_LEN_ERR;
        return EXT_ERR_BAD_DATA;
    }

    *data = head->data;
    *data_len = head->length;
    *proto = head->protocol;

    return EXT_ERR_SUCCESS;
}

mrs_create_plc_frame mrs_1376_2_mr_handle_get_node_info(const mrs_proto_1376_2_frame *frame,
                                                        td_u8 option, mr_task_node *node)
{
    mrs_create_plc_frame create_plc_frame = TD_NULL;
    mrs_cco_srv_ctx *cco = mrs_cco_get_srv_ctx();

    (td_void) memset_s(node, sizeof(mr_task_node), 0, sizeof(mr_task_node));
    (td_void) memcpy_s(node->src_addr, sizeof(node->src_addr), frame->src_addr, sizeof(frame->src_addr));
    (td_void) memcpy_s(node->dst_addr, sizeof(node->dst_addr), frame->dst_addr, sizeof(frame->dst_addr));

    node->seq = frame->seq;
    node->plc_seq = ++cco->plc_seq;
    node->rx_time = uapi_get_seconds();
    node->send_uart_frame = mrs_uart_tx;
    node->option = option;

    if (option == MRS_PR_F1H_F1) {
        node->priority = MRS_MR_TASK_PRI_LO;
        node->type = MRS_CCO_MR_TYPE_PR;
        node->tx_max = MRS_CFG_PR_PLC_TRY_MAX;
        node->timeout = MRS_CFG_PR_PLC_TIMEOUT;
        node->create_uart_frame = mrs_cco_create_pr_1376_2_frame;
        create_plc_frame = mrs_cco_create_pr_plc_frame;
    } else {
        node->priority = MRS_MR_TASK_PRI_HI;
        node->type = MRS_CCO_MR_TYPE_XR;
        node->tx_max = MRS_CFG_XR_PLC_TRY_MAX;
        node->timeout = MRS_CFG_XR_PLC_TIMEOUT;
        node->create_uart_frame = mrs_cco_create_xr_1376_2_frame;
        create_plc_frame = mrs_cco_create_xr_plc_frame;
    }
    return create_plc_frame;
}
td_u32 mrs_1376_2_mr_handle(mrs_fn_param *param, td_u8 option)
{
    td_u32 ret;
    mrs_proto_1376_2_frame *frame = param->frame;
    mr_task_node *node = TD_NULL;
    mrs_create_plc_frame create_plc_frame;
    mrs_get_mr_data get_mr_data[] = { mrs_cco_get_mr_data_13_f1, mrs_cco_get_mr_data_13_f255, mrs_cco_get_mr_data_02_f1,
        mrs_cco_get_mr_data_02_f255, mrs_cco_get_mr_data_f1_f1, };
    td_u8 deny = MRS_1376_2_DENY_CCO_BUSY;

    if (option > MRS_PR_F1H_F1) {
        return EXT_ERR_INVALID_PARAMETER;
    }

    node = (mr_task_node *)mrs_malloc(sizeof(mr_task_node));
    if (node == TD_NULL) {
        return EXT_ERR_MALLOC_FAILUE;
    }

    create_plc_frame = mrs_1376_2_mr_handle_get_node_info(frame, option, node);

    do {
        ext_mdm_nm_topo_node_info entry;
        td_pbyte data = TD_NULL;
        td_u16 data_len = 0;

        ret = get_mr_data[option](frame, &data, &data_len, &node->protocol, &deny);
        if (ret != EXT_ERR_SUCCESS) {
            break;
        }

        ret = create_plc_frame(node, data, data_len);
        if (ret != EXT_ERR_SUCCESS) {
            break;
        }

        ret = uapi_query_mac_attr(node->plc_frame->addr, &entry);
        if (ret != EXT_ERR_SUCCESS) {
            deny = MRS_1376_2_DENY_STA_NOT_IN_TOPO;
            break;
        }

        ret = mrs_cco_mr_list_insert(node);
        if (ret != EXT_ERR_SUCCESS) {
            deny = MRS_1376_2_DENY_RM_POOL_FULL;
            break;
        }
    } while (0);

    if (ret != EXT_ERR_SUCCESS) {
        ret = mrs_cco_1376_2_deny_frame(param, deny);
        mrs_free(node->plc_frame);
        mrs_free(node);
    }

    return ret;
}

td_u32 mrs_1376_2_afn_15_f1(mrs_fn_param *param)
{
    /* Every frame size (contains segment size) must be less than MRS_DATA_BUFFER_SIZE */
    td_u32 ret = EXT_ERR_BAD_DATA;
    mrs_upg_param upg_param;
    td_u32 segment = 0;
    td_u16 data_len = param->frame->data_len;
    td_u8 *content = param->frame->content;

    do {
        /* Length check */
        if (data_len < MRS_CCO_UPG_FRAME_DATA_SIZE_MIN) {
            break;
        }

        upg_param.file_flag = content[MRS_CCO_UPG_FILE_FLAG_OFFSET];
        upg_param.file_property = content[MRS_CCO_UPG_FILE_PROPERTY_OFFSET];
        upg_param.file_ins = content[MRS_CCO_UPG_FILE_INS_OFFSET];
        upg_param.seg_totoal = uapi_make_u16(content[MRS_CCO_UPG_SEG_TOTOAL_OFFSET],
                                             content[MRS_CCO_UPG_SEG_TOTOAL_OFFSET + 1]);
        upg_param.seg_curr = uapi_make_identifier(content[MRS_CCO_UPG_SEG_CURRENT_OFFSET],
                                                  content[MRS_CCO_UPG_SEG_CURRENT_OFFSET + 1],
                                                  content[MRS_CCO_UPG_SEG_CURRENT_OFFSET + 2],  /* index 7:5+2 */
                                                  content[MRS_CCO_UPG_SEG_CURRENT_OFFSET + 3]); /* index 8:5+3 */
        upg_param.data_len = uapi_make_u16(content[MRS_CCO_UPG_SEG_DATA_LEN_OFFSET],
                                           content[MRS_CCO_UPG_SEG_DATA_LEN_OFFSET + 1]);

        if (data_len != upg_param.data_len + MRS_CCO_UPG_FRAME_DATA_SIZE_MIN) {
            break;
        }

        upg_param.data = (upg_param.data_len > 0) ? (content + MRS_CCO_UPG_SEG_DATA_OFFSET) : TD_NULL;

        /* Analyse content */
        switch (upg_param.file_flag) {
            case MRS_CCO_UPG_FILE_FLAG_CLR:
                mrs_cco_upg_set_frame(param->frame);
                ret = mrs_upg_clr();
                break;

            case MRS_CCO_UPG_FILE_FLAG_CCO:
            case MRS_CCO_UPG_FILE_FLAG_STA:
                ret = mrs_upg_file_rx(&upg_param, &segment);
                break;

            default:
                break;
        }
    } while (0);

    if (ret != EXT_ERR_CONTINUE) {
        if (ret != EXT_ERR_SUCCESS) {
            segment = MRS_CCO_UPG_ERROR_SEG;
        }
        mrs_cco_1376_2_easy_encode(param, (td_u8 *)&segment, (td_u16)sizeof(td_u32));
    }

    return EXT_ERR_SUCCESS;
}

#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_CCO) */


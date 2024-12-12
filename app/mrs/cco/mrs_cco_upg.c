/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: CCO upgrade implementation.
 */

#include <app_nv.h>
#include "mrs_msg.h"
#include "mrs_cco_1376_2.h"
#include "mrs_common_tools.h"
#include "mrs_common_uart.h"
#include "mrs_cco_srv.h"
#include "mrs_common_power_failure.h"
#include "mrs_cco_upg.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
/* Upgrade stages */
#define MRS_UPG_STAGE_IDLE       (0)
#define MRS_UPG_STAGE_INPROGRESS (1)
#define MRS_UPG_STAGE_DL         (2)

/* Upgrade state */
#define MRS_UPG_STAT_DONE       (0x00)
#define MRS_UPG_STAT_INPROGRESS (0x01)
#define MRS_UPG_STAT_UNCOMPLETE (0x02)

/* File property */
#define MRS_CCO_UPG_FILE_PROPERTY_START (0x00)
#define MRS_CCO_UPG_FILE_PROPERTY_MID   (MRS_CCO_UPG_FILE_PROPERTY_START)
#define MRS_CCO_UPG_FILE_PROPERTY_END   (0x01)
/* File instruction */
#define MRS_CCO_UPG_FILE_INS_MSG (0x00)

/* File types */
#define UPG_FILE_TYPE_LOCAL   (0) /* type: UNICAST */
#define UPG_FILE_OPTION_LOCAL (EXT_UPG_REQ_OPTION_UNICAST)

#define UPG_FILE_TYPE_REMOTE   (1) /* type: STA-->STA */
#define UPG_FILE_OPTION_REMOTE (EXT_UPG_REQ_OPTION_STA)

/* Switch value */
#define MRS_UPG_SWITCH_ON  (1)
#define MRS_UPG_SWITCH_OFF (0)

/* Timeout values */
#define MRS_UPG_RX_FILE_TIMEOUT       (600000)
#define MRS_UPG_WAIT_PLC_TIMEOUT      (10800000)
#define MRS_UPG_STATUS_QUERY_INTERVAL (600000)
#define MRS_UPG_STATUS_QUERY_TIMEOUT  (2000)
#define MRS_UPG_REMOTE_STOP_TIMEOUT   (800)

#define MRS_UPG_TRANS_SEGMENT_SIZE_MIN (36) /* the first frame includes upgrade file length */
#define MRS_UPG_STOP_RETRY_MAX         (3)

#define MRS_UPG_GET_SW_VER_645_LEN (15)
#define MRS_UPG_ENTRY_MAP_LEN   (125)

#define MRS_UPG_INVALID     0xFF

typedef struct {
    mrs_proto_1376_2_frame *frame; /* 1376.2 frame */
    td_u32 upg_file_len;           /* total length of upgrading file */
    td_u32 upg_cache_size;         /* upgrade cache size */
    td_u32 seg_total;              /* total quantity of segments */
    td_u32 crc;                    /* Crc */
    td_u32 seg_curr;               /* curent processing segment */
    td_u16 seg_len;                /* length of each segment */
    td_u8 upg_stage;               /* current stage of upgrading */
    td_u8 upg_type;                /* upgrade type */
    td_u8 upg_stop;                /* switch flag of stopping */
    td_u8 upg_status;              /* status */
    td_u8 remote_clr_flag;         /* clear download flag */
    td_u8 remote_clr_resp;         /* clear response */
    td_u8 wait_chl;                /* wait for PLC idle */
    td_u8 upg_stop_retry;          /* countor */
    td_u8 upg_query_retry;
    td_u8 rsv;
} mrs_upg_ctx;

typedef struct {
    td_u16 num; /* entry number */
    td_u16 cnt; /* valid entry number */
    td_u16 remain; /* ok and failed */
    td_u16 qry_idx[3]; /* 3 is enough */
    td_u8 ver[MRS_SW_VERSION_MAX]; /* Store the sw verion */
    td_u8 entry_map[MRS_UPG_ENTRY_MAP_LEN]; /* 125 * 8 = 1000 */
    td_u8 seq;
    td_u8 retry;
    td_u8 ok_addr_valid;
} mrs_upg_stat_ctx;

/* Initial upgrade service control context */
mrs_upg_ctx *mrs_upg_srv_obtain(td_void);
#ifdef PRODUCT_CFG_LOCAL_UPG_HEAD_CHECK
/* Receviced first file head check */
td_u32 mrs_upg_start_segment_file_head_check(td_u8 upg_type);
#endif
/* Receviced first file segment */
td_u32 mrs_upg_start_segment_handler(mrs_upg_param *upg_param);
/* Receviced file segments during download */
td_u32 mrs_upg_middle_segment_handler(EXT_CONST mrs_upg_param *upg_param);
/* Receviced last file segment */
td_u32 mrs_upg_last_segment_handler(EXT_CONST mrs_upg_param *upg_param);
/* Finish upgrade, clean up environment */
td_void mrs_upg_cleanup(td_void);
/* Get current segment number */
td_u32 mrs_cco_upg_get_seg_curr(td_void);
/* Start upgrade */
td_u32 mrs_upg_kick_start(td_void);
/* Upgrade process */
td_u32 mrs_upg_process(td_void);
/* Upgrade result check request */
td_u32 mrs_upg_status(td_u32 option);
/* Nodes upgrade stop */
td_u32 mrs_upg_remote_stop(td_void);
/* Check id of the message from mdm level */
td_bool mrs_upg_msg_id_valid(td_u32 msg_id);
/* Message handlers from mdm level */
td_u32 mrs_upg_msg_rx(td_u32 msg_id, td_pbyte packet, td_u16 packet_size);
/* Response from mdm level */
td_void mrs_upg_start_resp(td_pbyte resp, td_u16 resp_len);
td_void mrs_upg_stop_resp(td_pbyte resp, td_u16 resp_len);
td_void mrs_upg_status_bs_resp(td_pbyte resp, td_u16 resp_len);
td_void mrs_upg_status_resp(td_pbyte resp, td_u16 resp_len);
td_void mrs_upg_stat_info_resp(td_pbyte resp, td_u16 resp_len);
/* Send 376.2 answer frame by asynchronous */
td_void mrs_upg_remote_clr_reply(td_u32 reply_type);
/* Wait PLC available ready */
td_bool mrs_is_plc_available(td_void);
/* Report node upgrade results, frame follows 376.2 */
td_void mrs_upg_node_report(td_u32 result, td_u8 *sw_ver, td_u16 ver_len);
td_void mrs_upg_node_report_fail(EXT_CONST td_u8 *mac, td_u16 mac_len);
/* Store upgrade status */
td_void mrs_save_file_inf(td_u8 upg_type, td_u32 file_crc);
/* Upgrade checking after power on */
td_void poweron_active_query(td_void);
/* Send get sta version plc */
td_void mrs_upg_get_sw_version(EXT_CONST td_u16 idx);
td_void mrs_upg_get_sw_ver_ctx_cleanup(td_void);
td_void mrs_upg_set_map_bit(td_u16 idx);
td_bool mrs_upg_check_map_bit(td_u16 idx);
td_u16 mrs_upg_get_node_map_idx(EXT_CONST td_u8 *mac, EXT_CONST td_u16 mac_len);
td_u16 mrs_upg_get_map_idx(td_u16 start);

mrs_upg_ctx *g_mrs_upg_ctx = TD_NULL;
mrs_upg_stat_ctx *g_mrs_upg_stat_ctx = TD_NULL;

#define mrs_cco_upg_get_ctx() (mrs_upg_ctx *)(g_mrs_upg_ctx)

td_void mrs_msg_on_remote_upg(EXT_CONST mrs_queue_msg *msg)
{
    td_pbyte data = (td_pbyte)msg->param1;

    switch (msg->param2) {
        case EXT_UPG_STATUS_BS_IND:
            mrs_upg_status_bs_resp(data, msg->param0);
            break;

        case EXT_UPG_STATUS:
            mrs_upg_status_resp(data, msg->param0);
            break;

        case EXT_UPG_START:
            mrs_upg_start_resp(data, msg->param0);
            break;

        case EXT_UPG_STOP:
            mrs_upg_stop_resp(data, msg->param0);
            break;

        case EXT_UPG_STAT_INFO_IND:
            mrs_upg_stat_info_resp(data, msg->param0);
            break;

        default:
            break;
    }

    if (data != TD_NULL) {
        mrs_free(data);
        data = TD_NULL;
    }
}

td_void mrs_cco_upg_set_frame(const mrs_proto_1376_2_frame *frame)
{
    /*
    Remember the parameters for answer frame,
    because some frames would be sent by asynchronous.
    */
    mrs_upg_ctx *upg_ctx = mrs_cco_upg_get_ctx();

    if (upg_ctx == TD_NULL) {
        return;
    }

    if (upg_ctx->frame != TD_NULL) {
        mrs_free(upg_ctx->frame);
        upg_ctx->frame = TD_NULL;
    }

    upg_ctx->frame = (mrs_proto_1376_2_frame *)mrs_malloc(sizeof(mrs_proto_1376_2_frame));
    if (upg_ctx->frame != TD_NULL) {
        /* Remember the head only */
        (td_void) memcpy_s(upg_ctx->frame, sizeof(mrs_proto_1376_2_frame), frame, sizeof(mrs_proto_1376_2_frame));
    }
}

td_u32 mrs_cco_upg_get_seg_curr(td_void)
{
    mrs_upg_ctx *upg_ctx = mrs_cco_upg_get_ctx();

    if (upg_ctx == TD_NULL) {
        return 0;
    }

    return upg_ctx->seg_curr;
}

td_bool mrs_cco_upg_is_idle(td_void)
{
    mrs_upg_ctx *upg_ctx = mrs_cco_upg_get_ctx();

    if (upg_ctx == TD_NULL) {
        return TD_TRUE;
    }

    if (upg_ctx->upg_stage == MRS_UPG_STAGE_IDLE) {
        return TD_TRUE;
    }

    return TD_FALSE;
}

td_u32 mrs_upg_init(td_void)
{
    app_upg_inf nv;

    uapi_upg_register_msg_notify(EXT_UPG_START, mrs_upg_msg_rx);
    uapi_upg_register_msg_notify(EXT_UPG_STOP, mrs_upg_msg_rx);
    uapi_upg_register_msg_notify(EXT_UPG_STATUS_BS_IND, mrs_upg_msg_rx);
    uapi_upg_register_msg_notify(EXT_UPG_STATUS, mrs_upg_msg_rx);
    uapi_upg_register_msg_notify(EXT_UPG_STAT_INFO_IND, mrs_upg_msg_rx);

    (td_void) memset_s(&nv, sizeof(nv), 0, sizeof(nv));
    (td_void) uapi_nv_read(ID_NV_APP_UPG_INF, (EXT_CONST td_pvoid)&nv, sizeof(nv));

    if ((nv.filc_crc != 0) && (nv.file_attr == MRS_CCO_UPG_FILE_FLAG_STA)) {
        mrs_upg_ctx *upg_ctx = mrs_upg_srv_obtain();

        if (upg_ctx == TD_NULL) {
            return EXT_ERR_MALLOC_FAILUE;
        }

        upg_ctx->upg_stage = MRS_UPG_STAGE_INPROGRESS;
        upg_ctx->upg_status = MRS_UPG_STAT_INPROGRESS;
        upg_ctx->crc = nv.filc_crc;
        upg_ctx->upg_type = nv.file_attr;

        /* Checking node upgrade results after power on */
        poweron_active_query();
    }

    return EXT_ERR_SUCCESS;
}

mrs_upg_ctx *mrs_upg_srv_obtain(td_void)
{
    if (g_mrs_upg_ctx == TD_NULL) {
        g_mrs_upg_ctx = (mrs_upg_ctx *)mrs_malloc(sizeof(mrs_upg_ctx));
        if (g_mrs_upg_ctx == TD_NULL) {
            return TD_NULL;
        }
    }

    (td_void) memset_s(g_mrs_upg_ctx, sizeof(mrs_upg_ctx), 0, sizeof(mrs_upg_ctx));

    return g_mrs_upg_ctx;
}

td_u32 mrs_upg_file_rx(mrs_upg_param *upg_param, td_u32 *segment)
{
    td_u32 ret = EXT_ERR_SUCCESS;

    if (upg_param->file_property > MRS_CCO_UPG_FILE_PROPERTY_END) {
        return EXT_ERR_BAD_DATA;
    }

    if (upg_param->data_len == 0) {
        return EXT_ERR_BAD_DATA;
    }

    if ((upg_param->file_property == MRS_CCO_UPG_FILE_PROPERTY_START) && (upg_param->seg_curr == 0)) {
        /* The start segment */
        ret = mrs_upg_start_segment_handler(upg_param);
    } else if ((upg_param->file_property == MRS_CCO_UPG_FILE_PROPERTY_MID) && (upg_param->seg_curr != 0)) {
        /* The middle segment */
        ret = mrs_upg_middle_segment_handler(upg_param);
    } else if ((upg_param->file_property == MRS_CCO_UPG_FILE_PROPERTY_END) && (upg_param->seg_curr != 0)) {
        /* The last segment */
        ret = mrs_upg_last_segment_handler(upg_param);
    }

    if (ret == EXT_ERR_SUCCESS) {
        *segment = upg_param->seg_curr;
    }

    return ret;
}

td_u32 mrs_upg_clr(td_void)
{
    /* MRS_CCO_UPG_FILE_FLAG_CLR handler, passed length check */
    td_u32 ret;
    mrs_upg_ctx *upg_ctx = mrs_cco_upg_get_ctx();

    if (upg_ctx == TD_NULL) {
        return EXT_ERR_SUCCESS;
    }

    switch (upg_ctx->upg_stage) {
        case MRS_UPG_STAGE_IDLE:
            upg_ctx->upg_status = MRS_UPG_STAT_DONE;
            ret = EXT_ERR_SUCCESS;
            break;

        case MRS_UPG_STAGE_DL:
            upg_ctx->upg_status = MRS_UPG_STAT_UNCOMPLETE;
            mrs_upg_cleanup();
            ret = EXT_ERR_SUCCESS;
            break;

        case MRS_UPG_STAGE_INPROGRESS:
            if (upg_ctx->upg_type == MRS_CCO_UPG_FILE_FLAG_CCO) {
                upg_ctx->upg_status = MRS_UPG_STAT_UNCOMPLETE;
                mrs_upg_cleanup();
                ret = EXT_ERR_SUCCESS;
                break;
            }

            if (upg_ctx->upg_stop == MRS_UPG_SWITCH_OFF) {
                ret = EXT_ERR_FAILURE;
                break;
            }

            upg_ctx->remote_clr_flag = MRS_UPG_SWITCH_ON;
            upg_ctx->remote_clr_resp = MRS_UPG_SWITCH_ON;
            ret = mrs_upg_remote_stop();
            break;

        default:
            ret = EXT_ERR_FAILURE;
            break;
    }

    return ret;
}

td_void mrs_cco_upg_file_rx_on_timer(td_void)
{
    mrs_upg_cleanup();
}

td_void mrs_cco_upg_stop_on_timer(td_void)
{
    mrs_upg_ctx *upg_ctx = mrs_cco_upg_get_ctx();

    if (upg_ctx->upg_stop_retry < MRS_UPG_STOP_RETRY_MAX) {
        mrs_upg_remote_stop();
    } else {
        mrs_upg_remote_clr_reply(EXT_ERR_FAILURE);
    }
}

td_void mrs_cco_upg_stat_query_bs_on_timer(td_void)
{
    mrs_upg_ctx *upg_ctx = mrs_cco_upg_get_ctx();

    upg_ctx->upg_stop = MRS_UPG_SWITCH_ON;
    mrs_upg_status(EXT_UPG_STATUS_REQ_OPT_BS);
}

td_void mrs_cco_upg_stat_timeout(td_void)
{
    /* Querying results of the whole network */
    mrs_upg_status(EXT_UPG_STATUS_REQ_OPT_NA);
}

td_void mrs_cco_upg_get_sw_ver_timeout(td_void)
{
    td_u8 fail_ver[2] = { 0xFF, 0xFF }; /* ver : 2B */

    /* Report as failure if timeout */
    mrs_upg_node_report(EXT_ERR_FAILURE, fail_ver, sizeof(fail_ver));
    mrs_upg_get_sw_ver_ctx_cleanup();
    mrs_upg_cleanup();
}

td_void mrs_cco_upg_wait_resp_done_timeout(td_void)
{
    mrs_timer_stop(MRS_TIMER_ID_UPG_WAIT_RESP_DONE);
    /* Wait status response done, in case sw version response came before status response */
    if (g_mrs_upg_stat_ctx->cnt == g_mrs_upg_stat_ctx->num) {
        mrs_upg_node_report(EXT_ERR_SUCCESS, g_mrs_upg_stat_ctx->ver,
                            (td_u16)(strlen((const char *)g_mrs_upg_stat_ctx->ver)));
    } else {
        if (g_mrs_upg_stat_ctx->retry > 3) { /* 3 times */
            mrs_cco_upg_get_sw_ver_timeout(); /* Consider as timeout fail */
        }
        g_mrs_upg_stat_ctx->retry++;
        mrs_timer_start(MRS_TIMER_ID_UPG_WAIT_RESP_DONE, MRS_UPG_STATUS_QUERY_TIMEOUT, EXT_TIMER_TYPE_ONCE);
    }
}

td_void mrs_cco_upg_plc_wait_timeout(td_void)
{
    mrs_upg_cleanup();
}

td_void mrs_cco_upg_reset_file_rx_timeout(td_void)
{
    mrs_upg_ctx *upg_ctx = mrs_cco_upg_get_ctx();

    if (upg_ctx == TD_NULL) {
        return;
    }

    if (upg_ctx->upg_stage == MRS_UPG_STAGE_DL) {
        mrs_timer_start(MRS_TIMER_ID_TRANS_FILE_RX, MRS_UPG_RX_FILE_TIMEOUT, EXT_TIMER_TYPE_ONCE);
    }
}

td_u32 mrs_upg_stat_pre_check(mrs_upg_ctx **upg_ctx)
{
    if (*upg_ctx == TD_NULL) {
        *upg_ctx = mrs_upg_srv_obtain();
        if (*upg_ctx == TD_NULL) {
            return EXT_ERR_MALLOC_FAILUE;
        }
    } else if ((*upg_ctx)->upg_stage != MRS_UPG_STAGE_IDLE) {
        return EXT_ERR_BUSY;
    } else if ((*upg_ctx)->upg_status == MRS_UPG_STAT_INPROGRESS) { /* Got start segment before, return error */
        return EXT_ERR_BAD_DATA;
    }

    return EXT_ERR_SUCCESS;
}

td_u32 mrs_get_upg_cache_size(mrs_upg_ctx *upg_ctx, const mrs_upg_param *upg_param, td_u32 *file_type)
{
    td_u32 file_len = 0;

    switch (upg_ctx->upg_type) {
        case MRS_CCO_UPG_FILE_FLAG_CCO:
            *file_type = UPG_FILE_TYPE_LOCAL;
            /* Get file size from binary */
            if (memcpy_s(&file_len, sizeof(td_u32),
                &(((ext_21_upg_head *)upg_param->data)->file_len), sizeof(td_u32)) != EOK) {
                return EXT_ERR_INVALID_PARAMETER;
            }
            upg_ctx->upg_cache_size = file_len; /* Only accept right length for CCO upgrade */
            break;

        case MRS_CCO_UPG_FILE_FLAG_STA:
            *file_type = UPG_FILE_TYPE_REMOTE;
            upg_ctx->upg_cache_size = upg_ctx->seg_total * upg_ctx->seg_len;
            break;

        default:
            return EXT_ERR_INVALID_PARAMETER;
    }
    return EXT_ERR_SUCCESS;
}
#ifdef PRODUCT_CFG_LOCAL_UPG_HEAD_CHECK
td_u32 mrs_upg_start_segment_file_head_check(td_u8 upg_type)
{
    td_u32 ret = EXT_ERR_SUCCESS;
    if (upg_type == MRS_CCO_UPG_FILE_FLAG_CCO) {
        ret = uapi_local_upg_head_check_res();
        if (ret != EXT_ERR_SUCCESS) {
            mrs_upg_cleanup();
            return ret;
        }
    }
    return ret;
}
#endif
td_u32 mrs_upg_start_segment_handler(mrs_upg_param *upg_param)
{
    td_u32 ret;
    td_u32 file_type = UPG_FILE_TYPE_LOCAL;
    mrs_upg_ctx *upg_ctx = mrs_cco_upg_get_ctx();

    ret = mrs_upg_stat_pre_check(&upg_ctx);
    if (ret != EXT_ERR_SUCCESS) {
        return ret;
    }

    if (upg_param->data_len < MRS_UPG_TRANS_SEGMENT_SIZE_MIN) { /* Do NOT accept too small segment */
        return EXT_ERR_BAD_DATA;
    }

    upg_ctx->upg_stage = MRS_UPG_STAGE_DL;
    upg_ctx->seg_total = upg_param->seg_totoal;
    upg_ctx->upg_status = MRS_UPG_STAT_INPROGRESS;
    upg_ctx->seg_curr = upg_param->seg_curr;
    upg_ctx->upg_file_len = 0;
    upg_ctx->upg_type = upg_param->file_flag;
    upg_ctx->seg_len = upg_param->data_len;
    upg_ctx->crc = 0;

    ret = mrs_get_upg_cache_size(upg_ctx, upg_param, &file_type);
    if (ret != EXT_ERR_SUCCESS) {
        return ret;
    }

    ret = uapi_upg_obtain_cache(file_type, upg_ctx->upg_cache_size);
    if (ret != EXT_ERR_SUCCESS) {
        mrs_printf("[ERR: %s: %d]Obtain cache failed!\n", __FUNCTION__, __LINE__);
        return EXT_ERR_FAILURE;
    }

    /* Data in the start segment */
    ret = uapi_upg_mem_write(upg_ctx->upg_file_len, upg_param->data, upg_param->data_len);
    if (ret != EXT_ERR_SUCCESS) {
        mrs_printf("[ERR: %s: %d]Write cache failed!\n", __FUNCTION__, __LINE__);
        return ret;
    }

    upg_ctx->upg_file_len += upg_param->data_len;
    upg_ctx->crc = uapi_crc32(upg_ctx->crc, upg_param->data, (td_u32)upg_param->data_len);
#ifdef PRODUCT_CFG_LOCAL_UPG_HEAD_CHECK
    ret = mrs_upg_start_segment_file_head_check(upg_ctx->upg_type);
    if (ret != EXT_ERR_SUCCESS) {
        return ret;
    }
#endif
    if (upg_ctx->seg_total == 1) {
        /* One segment contains a whole file */
        ret = mrs_upg_kick_start();
    } else {
        mrs_timer_start(MRS_TIMER_ID_TRANS_FILE_RX, MRS_UPG_RX_FILE_TIMEOUT, EXT_TIMER_TYPE_ONCE);
    }

    return ret;
}

td_u32 mrs_upg_middle_segment_handler(EXT_CONST mrs_upg_param *upg_param)
{
    td_u32 ret = EXT_ERR_SUCCESS;
    mrs_upg_ctx *upg_ctx = mrs_cco_upg_get_ctx();

    if ((upg_ctx == TD_NULL) || (upg_ctx->upg_status != MRS_UPG_STAT_INPROGRESS)) {
        /* Expect start segment */
        return EXT_ERR_BAD_DATA;
    }

    do {
        if ((upg_ctx->upg_stage != MRS_UPG_STAGE_DL) || (upg_param->seg_totoal != upg_ctx->seg_total)
            || (upg_param->seg_curr != upg_ctx->seg_curr + 1) || (upg_param->data_len != upg_ctx->seg_len)
            || (upg_ctx->upg_type != upg_param->file_flag)) {
            ret = EXT_ERR_BAD_DATA;
            break;
        }

        /* Move data to cache */
        ret = uapi_upg_mem_write(upg_ctx->upg_file_len, upg_param->data, upg_param->data_len);
        if (ret == EXT_ERR_SUCCESS) {
            upg_ctx->seg_curr = upg_param->seg_curr;
            upg_ctx->upg_file_len += upg_param->data_len;
            upg_ctx->crc = uapi_crc32(upg_ctx->crc, upg_param->data, (td_u32)upg_param->data_len);
        }
    } while (0);

    mrs_timer_start(MRS_TIMER_ID_TRANS_FILE_RX, MRS_UPG_RX_FILE_TIMEOUT, EXT_TIMER_TYPE_ONCE);

    return ret;
}

td_u32 mrs_upg_last_segment_handler(EXT_CONST mrs_upg_param *upg_param)
{
    td_u32 ret = EXT_ERR_SUCCESS;
    mrs_upg_ctx *upg_ctx = mrs_cco_upg_get_ctx();

    if ((upg_ctx == TD_NULL) || (upg_ctx->upg_status != MRS_UPG_STAT_INPROGRESS)) {
        return EXT_ERR_BAD_DATA;
    }

    if ((upg_ctx->upg_stage != MRS_UPG_STAGE_DL) || (upg_param->seg_totoal != upg_ctx->seg_total)
        || (upg_param->seg_curr != upg_ctx->seg_curr + 1) || (upg_param->seg_curr != upg_ctx->seg_total - 1)
        || (upg_ctx->upg_type != upg_param->file_flag) || (upg_param->data_len > upg_ctx->seg_len)) {
        mrs_timer_start(MRS_TIMER_ID_TRANS_FILE_RX, MRS_UPG_RX_FILE_TIMEOUT, EXT_TIMER_TYPE_ONCE);
        return EXT_ERR_BAD_DATA;
    }

    do {
        if ((upg_ctx->upg_type == MRS_CCO_UPG_FILE_FLAG_CCO)
            && (upg_ctx->upg_cache_size != upg_ctx->upg_file_len + upg_param->data_len)) {
            /* Do NOT accept the file, because file size doesn't match cache size */
            ret = EXT_ERR_BAD_DATA;
            mrs_upg_cleanup();
            break;
        }

        /* Move data to cache */
        ret = uapi_upg_mem_write(upg_ctx->upg_file_len, upg_param->data, upg_param->data_len);
        if (ret == EXT_ERR_SUCCESS) {
            upg_ctx->seg_curr = upg_param->seg_curr;
            upg_ctx->upg_file_len += upg_param->data_len;
            upg_ctx->crc = uapi_crc32(upg_ctx->crc, upg_param->data, (td_u32)upg_param->data_len);

            mrs_timer_stop(MRS_TIMER_ID_TRANS_FILE_RX);

            /* File transfer done, kick upgrade */
            ret = mrs_upg_kick_start();
        }
    } while (0);

    return ret;
}

td_void mrs_upg_cleanup(td_void)
{
    uapi_upg_release_cache();

    mrs_timer_stop(MRS_TIMER_ID_TRANS_FILE_RX);
    mrs_timer_stop(MRS_TIMER_ID_UPG_STOP);
    mrs_timer_stop(MRS_TIMER_ID_UPG_BASE_STATUS_POLLING);
    mrs_timer_stop(MRS_TIMER_ID_UPG_NODE_STATUS_TIMEOUT);

    /* Clear stored upgrade results */
    mrs_save_file_inf(0, 0);

    if (g_mrs_upg_ctx != TD_NULL) {
        if (g_mrs_upg_ctx->frame != TD_NULL) {
            mrs_free(g_mrs_upg_ctx->frame);
        }

        mrs_free(g_mrs_upg_ctx);
        g_mrs_upg_ctx = TD_NULL;
    }
}

td_u32 mrs_upg_kick_start(td_void)
{
    td_u32 ret;
    mrs_upg_ctx *upg_ctx = mrs_cco_upg_get_ctx();

    upg_ctx->upg_stage = MRS_UPG_STAGE_INPROGRESS;

    upg_ctx->wait_chl = MRS_UPG_SWITCH_OFF;
    mrs_timer_stop(MRS_TIMER_ID_PLC_IS_AVAILABLE);

    if (upg_ctx->upg_type == MRS_CCO_UPG_FILE_FLAG_STA) {
        ret = uapi_upg_head_check_res();
        if (ret != EXT_ERR_SUCCESS) {
            mrs_upg_cleanup();
            return ret;
        }
    }

    if (!mrs_is_plc_available() && (upg_ctx->upg_type == MRS_CCO_UPG_FILE_FLAG_STA)) {
        upg_ctx->wait_chl = MRS_UPG_SWITCH_ON;
        mrs_timer_start(MRS_TIMER_ID_PLC_IS_AVAILABLE, MRS_UPG_WAIT_PLC_TIMEOUT, EXT_TIMER_TYPE_ONCE);
        return EXT_ERR_SUCCESS;
    }

    ret = mrs_upg_process();
    if (ret != EXT_ERR_SUCCESS) {
        mrs_upg_cleanup();
        return ret;
    }

    upg_ctx->upg_stop = MRS_UPG_SWITCH_OFF;
    (td_void) mrs_upg_status(EXT_UPG_STATUS_REQ_OPT_BS);
    mrs_timer_start(MRS_TIMER_ID_UPG_BASE_STATUS_POLLING, MRS_UPG_STATUS_QUERY_INTERVAL, EXT_TIMER_TYPE_PERIOD);

    return ret;
}

td_u32 mrs_upg_process(td_void)
{
    td_u32 ret;
    mrs_upg_ctx *upg_ctx = mrs_cco_upg_get_ctx();
    ext_upg_start_req *req = TD_NULL;
    td_u8 i;

    req = (ext_upg_start_req *)mrs_malloc(sizeof(ext_upg_start_req));
    if (req == TD_NULL) {
        return EXT_ERR_MALLOC_FAILUE;
    }

    (td_void) memset_s(req, sizeof(ext_upg_start_req), 0, sizeof(ext_upg_start_req));

    if (upg_ctx->upg_type == MRS_CCO_UPG_FILE_FLAG_CCO) {
        req->cfg.option = UPG_FILE_OPTION_LOCAL | EXT_UPG_VER_UPDATE_OPTION_START;
    } else {
        req->cfg.option = UPG_FILE_OPTION_REMOTE | EXT_UPG_VER_UPDATE_OPTION_START;
    }

    req->cfg.file_len = upg_ctx->upg_file_len;
    req->cfg.file_crc = upg_ctx->crc;
    req->cfg.upg_id = upg_ctx->crc;
    req->cfg.upg_time_window = 0; /* Default */
    req->cfg.blk_num = 1;         /* Fixed value: 1 */

    req->file.upg_id = req->cfg.upg_id;
    req->file.idx = 1;        /* Fixed value: 1 */
    for (i = 0; i < 2; i++) { /* send for 2times */
        ret = uapi_upg_msg_send(EXT_UPG_START, (td_pbyte)req, sizeof(ext_upg_start_req));
        if (ret == EXT_ERR_SUCCESS) {
            break;
        }
    }

    if (ret == EXT_ERR_SUCCESS) {
        mrs_save_file_inf(upg_ctx->upg_type, upg_ctx->crc);
    }

    return ret;
}

td_u32 mrs_upg_remote_stop(td_void)
{
    td_u32 ret;
    mrs_upg_ctx *upg_ctx = mrs_cco_upg_get_ctx();
    ext_upg_stop_req req;
    td_u8 i;

    (td_void) memset_s(&req, sizeof(req), 0, sizeof(req));
    req.stop_req.upg_id = upg_ctx->crc;

    for (i = 0; i < 2; i++) { /* send for 2times */
        ret = uapi_upg_msg_send(EXT_UPG_STOP, (td_pbyte)&req, sizeof(req));
        if (ret != EXT_ERR_SUCCESS) {
            continue;
        }

        upg_ctx->upg_stop_retry++;
        if (upg_ctx->remote_clr_flag == MRS_UPG_SWITCH_OFF) {
            break;
        }

        ret = EXT_ERR_CONTINUE;

        mrs_timer_start(MRS_TIMER_ID_UPG_STOP, MRS_UPG_REMOTE_STOP_TIMEOUT, EXT_TIMER_TYPE_ONCE);
        break;
    }

    return ret;
}

td_u32 mrs_upg_status(td_u32 option)
{
    ext_upg_status_req req;
    td_u32 ret;
    td_u8 i;

    (td_void) memset_s(&req, sizeof(req), 0, sizeof(req));
    req.status_req.option = option;

    for (i = 0; i < 2; i++) { /* send for 2times */
        ret = uapi_upg_msg_send(EXT_UPG_STATUS, (td_pbyte)&req, sizeof(req));
        if (ret == EXT_ERR_SUCCESS) {
            break;
        }
    }

    return ret;
}

td_bool mrs_upg_msg_id_valid(td_u32 msg_id)
{
    switch (msg_id) {
        case EXT_UPG_START:
        case EXT_UPG_STOP:
        case EXT_UPG_STATUS_BS_IND:
        case EXT_UPG_STATUS:
        case EXT_UPG_STAT_INFO_IND:
            return TD_TRUE;

        default:
            return TD_FALSE;
    }
}

td_u32 mrs_upg_msg_rx(td_u32 msg_id, td_pbyte packet, td_u16 packet_size)
{
    td_u32 ret;
    td_pbyte payload;

    if ((packet == TD_NULL) || (packet_size == 0)) {
        return EXT_ERR_INVALID_PARAMETER;
    }

    if (mrs_upg_msg_id_valid(msg_id) != TD_TRUE) {
        return EXT_ERR_INVALID_ID;
    }

    payload = (td_pbyte)mrs_malloc(packet_size);
    if (payload == TD_NULL) {
        return EXT_ERR_MALLOC_FAILUE;
    }

    (td_void) memcpy_s(payload, packet_size, packet, packet_size);

    ret = mrs_msg_queue_send(MRS_MSG_ID_REMOTE_UPG, packet_size, (uintptr_t)payload, (uintptr_t)msg_id);
    if (ret != EXT_ERR_SUCCESS) {
        mrs_free(payload);
    }

    return ret;
}

td_bool mrs_is_plc_available(td_void)
{
    mrs_cco_srv_ctx *srv_ctx = mrs_cco_get_srv_ctx();
    return (srv_ctx->chl_status);
}

/* Report result of clearing files */
td_void mrs_upg_remote_clr_reply(td_u32 reply_type)
{
    td_u32 ret;
    td_u32 segment = MRS_CCO_UPG_ERROR_SEG;
    mrs_fn_param param;
    td_pbyte out_buf = TD_NULL;
    td_u16 out_len = 0;
    mrs_upg_ctx *upg_ctx = mrs_cco_upg_get_ctx();

    if (reply_type == EXT_ERR_SUCCESS) {
        segment = upg_ctx->seg_curr;
    }

    (td_void) memset_s(&param, sizeof(mrs_fn_param), 0, sizeof(mrs_fn_param));
    param.frame = upg_ctx->frame;
    param.out_buf = &out_buf;
    param.out_len = &out_len;
    param.afn = 0x15;
    param.fn = 0x01;

    ret = mrs_cco_1376_2_easy_encode(&param, (td_u8 *)&segment, (td_u16)sizeof(td_u32));
    if ((ret == EXT_ERR_SUCCESS) && (out_buf != TD_NULL)) {
        mrs_uart_tx(out_buf, out_len);
    }

    mrs_free(out_buf);
    mrs_free(upg_ctx->frame);
}
td_void mrs_upg_get_node_ver(td_u8 *data, td_u16 data_len, td_u32 *pos, const td_u8 *sw_ver, td_u16 ver_len)
{
    td_u32 idx = *pos;
    /* Version number */
    data[idx] = (td_u8)ver_len;
    if (ver_len > 0) {
        idx++;

        /* Get version number */
        if (memcpy_s(&data[idx], data_len - idx, sw_ver, ver_len) != EOK) {}
    }

    *pos = idx;
}

td_u16 mrs_upg_get_map_idx(td_u16 start)
{
    for (td_u16 i = start; i < MRS_UPG_ENTRY_MAP_LEN * BIT_SIZE; i++) {
        if (mrs_upg_check_map_bit(i) == TD_TRUE) {
            return i;
        }
    }
    return 0;
}

static td_u32 mrs_upg_fill_report_param(mrs_fn_param *param, td_pbyte *out_buf, td_u16 *out_len)
{
    (td_void) memset_s(param, sizeof(mrs_fn_param), 0, sizeof(mrs_fn_param));
    param->frame = (mrs_proto_1376_2_frame *)mrs_malloc(sizeof(mrs_proto_1376_2_frame));
    if (param->frame == TD_NULL) {
        return EXT_ERR_FAILURE;
    }

    (td_void) memset_s(param->frame, sizeof(mrs_proto_1376_2_frame), 0, sizeof(mrs_proto_1376_2_frame));

    param->out_buf = out_buf;
    param->out_len = out_len;

    /* Report 06H-F7 */
    param->afn = 6; /* AFN 6 */
    param->fn = 7;  /* fn 7 */

    return EXT_ERR_SUCCESS;
}

static td_u32 mrs_upg_fill_data(td_u32 result, td_u8 *data, td_u16 data_len,
    td_u16 idx, td_u32 *pos)
{
    td_u32 tmp_pos;
    ext_mdm_nm_topo_node_info *topo_info = TD_NULL;

    uapi_get_topo_info(&topo_info);
    if (topo_info == TD_NULL) {
        return EXT_ERR_FAILURE;
    }
    if (memcpy_s(data, data_len, topo_info[idx].mac, MRS_METER_ADDR_LEN) != EOK) {
        return EXT_ERR_FAILURE;
    }
    mrs_hex_invert(data, MRS_METER_ADDR_LEN);

    tmp_pos = MRS_METER_ADDR_LEN;
    if (result == EXT_ERR_SUCCESS) {
        data[tmp_pos++] = 1; /* success 1 */
    } else {
        data[tmp_pos++] = 0;
    }
    *pos = tmp_pos;

    return EXT_ERR_SUCCESS;
}
/* Report upgrade results of all nodes in the network */
td_void mrs_upg_node_report(td_u32 result, td_u8 *sw_ver, td_u16 ver_len)
{
    td_u32 ret, pos, num;
    td_u16 out_len = 0;
    td_u16 data_len;
    td_u16 idx = 0;
    mrs_fn_param param;
    td_pbyte out_buf = TD_NULL;
    td_u8 *data = TD_NULL;

    if ((g_mrs_upg_stat_ctx == TD_NULL) || (sw_ver == TD_NULL)) {
        return;
    }

    if (mrs_upg_fill_report_param(&param, &out_buf, &out_len) != EXT_ERR_SUCCESS) {
        return;
    }
    data_len = MRS_METER_ADDR_LEN + 2 + ver_len; /* 2: upgrade success flag 1B, new version length 1B */
    data = (td_u8 *)mrs_malloc(data_len);
    if (data == TD_NULL) {
        mrs_free(param.frame);
        return;
    }

    mrs_timer_stop(MRS_TIMER_ID_UPG_GET_SW_VER);

    for (pos = 0, num = 0; num < g_mrs_upg_stat_ctx->num; num++) {
        ++idx;
        idx = mrs_upg_get_map_idx(idx);
        if (idx == 0) {
            continue; /* failed */
        }
        if (mrs_upg_fill_data(result, data, data_len, idx, &pos) != EXT_ERR_SUCCESS) {
            mrs_free(data);
            mrs_free(param.frame);
            return;
        }
        mrs_upg_get_node_ver(data, data_len, &pos, sw_ver, ver_len);
        ret = mrs_cco_1376_2_easy_encode(&param, data, data_len);
        if ((ret == EXT_ERR_SUCCESS) && (out_buf != TD_NULL)) {
            mrs_uart_tx(out_buf, out_len);
        }

        mrs_free(out_buf);
        g_mrs_upg_stat_ctx->remain--;
        if (g_mrs_upg_stat_ctx->remain == 0) {
            break;
        }
    }

    mrs_free(param.frame);
    mrs_upg_get_sw_ver_ctx_cleanup();
    mrs_upg_cleanup();
}

td_void mrs_upg_node_report_fail(EXT_CONST td_u8 *mac, td_u16 mac_len)
{
    td_u32 ret;
    td_u32 pos;
    td_u16 out_len = 0;
    td_u16 data_len;
    mrs_fn_param param;
    td_pbyte out_buf = TD_NULL;
    td_u8 fail_ver[2] = { 0xFF, 0xFF }; /* version: 2 */
    td_u8 *data = TD_NULL;

    (td_void) memset_s(&param, sizeof(mrs_fn_param), 0, sizeof(mrs_fn_param));

    param.frame = (mrs_proto_1376_2_frame *)mrs_malloc(sizeof(mrs_proto_1376_2_frame));
    if (param.frame == TD_NULL) {
        return;
    }

    (td_void) memset_s(param.frame, sizeof(mrs_proto_1376_2_frame), 0, sizeof(mrs_proto_1376_2_frame));

    param.out_buf = &out_buf;
    param.out_len = &out_len;

    /* Report 06H-F7 */
    param.afn = 6; /* afn 6 */
    param.fn = 7;  /* fn 7 */

    data_len = MRS_METER_ADDR_LEN + 2 + sizeof(fail_ver); /* 2: upgrade success flag 1B, new version length 1B */
    data = (td_u8 *)mrs_malloc(data_len);
    if (data == TD_NULL) {
        mrs_free(param.frame);
        return;
    }

    if (memcpy_s(data, data_len, mac, mac_len) != EOK) {
        mrs_free(data);
        mrs_free(param.frame);
        return;
    }
    mrs_hex_invert(data, MRS_METER_ADDR_LEN);

    pos = mac_len;
    /* Failed */
    data[pos] = 0;
    pos++;
    /* Version number */
    data[pos] = (td_u8)sizeof(fail_ver);
    pos++;

    /* Get version number */
    if (memcpy_s(&data[pos], data_len - pos, fail_ver, sizeof(fail_ver)) != EOK) {}

    ret = mrs_cco_1376_2_easy_encode(&param, data, data_len);
    if ((ret == EXT_ERR_SUCCESS) && (out_buf != TD_NULL)) {
        mrs_uart_tx(out_buf, out_len);
    }

    mrs_free(out_buf);
    mrs_free(param.frame);
}

/* Got response of starting upgrade */
td_void mrs_upg_start_resp(td_pbyte resp, td_u16 resp_len)
{
    ext_upg_start_ind *ind = (ext_upg_start_ind *)resp;

    if ((resp == TD_NULL) || (resp_len != sizeof(ext_upg_start_ind))) {
        return;
    }

    if (ind->rc != EXT_ERR_SUCCESS) {
        mrs_upg_cleanup();
    }
}

/* Got response of stopping upgrade */
td_void mrs_upg_stop_resp(td_pbyte resp, td_u16 resp_len)
{
    ext_upg_stop_ind *ind = (ext_upg_stop_ind *)resp;
    mrs_upg_ctx *upg_ctx = mrs_cco_upg_get_ctx();

    if ((upg_ctx == TD_NULL) || (resp_len != sizeof(ext_upg_stop_ind))) {
        return;
    }

    mrs_timer_stop(MRS_TIMER_ID_UPG_STOP);
    upg_ctx->upg_stop_retry = 0;

    if (upg_ctx->remote_clr_flag != MRS_UPG_SWITCH_ON) {
        if (ind->stop_ind.upg_id == 0) {
            mrs_upg_cleanup();
        }
        return;
    }

    if (ind->stop_ind.upg_id == 0) {
        return;
    }

    if ((ind->stop_ind.rc == EXT_ERR_SUCCESS) || (ind->stop_ind.rc == EXT_ERR_SUM_INVALID_UPG_ID)) {
        mrs_upg_remote_clr_reply(EXT_ERR_SUCCESS);
        mrs_upg_cleanup();
    } else if (upg_ctx->remote_clr_resp == MRS_UPG_SWITCH_OFF) {
        upg_ctx->remote_clr_flag = MRS_UPG_SWITCH_OFF;
        upg_ctx->remote_clr_resp = MRS_UPG_SWITCH_ON;
        mrs_upg_remote_clr_reply(EXT_ERR_FAILURE);
        mrs_upg_cleanup();
    }
}

/* Got basic infomation response of upgrading */
td_void mrs_upg_status_bs_resp(td_pbyte resp, td_u16 resp_len)
{
    mrs_upg_ctx *upg_ctx = mrs_cco_upg_get_ctx();
    ext_upg_status_bs_ind *ind = (ext_upg_status_bs_ind *)resp;

    if ((resp_len != sizeof(ext_upg_status_bs_ind)) || (ind == TD_NULL)
        || (upg_ctx == TD_NULL)) {
        return;
    }

    upg_ctx->upg_stop = MRS_UPG_SWITCH_ON;

    if (ind->upg_status == EXT_UPG_STATUS_OK) {
        mrs_timer_stop(MRS_TIMER_ID_UPG_BASE_STATUS_POLLING);
        if (upg_ctx->upg_type == MRS_CCO_UPG_FILE_FLAG_STA) {
            /* Query all info */
            mrs_upg_status(EXT_UPG_STATUS_REQ_OPT_NA);
            mrs_timer_start(MRS_TIMER_ID_UPG_NODE_STATUS_TIMEOUT,
                            MRS_UPG_STATUS_QUERY_TIMEOUT * 15, EXT_TIMER_TYPE_ONCE); /* 15 * 2000 */
        }

        return;
    }

    if ((ind->upg_status == EXT_UPG_STATUS_END) || (ind->upg_status == EXT_UPG_STATUS_PROCESS)) {
        if (ind->upg_id == upg_ctx->crc) {
            upg_ctx->upg_stage = MRS_UPG_STAGE_INPROGRESS;
            upg_ctx->upg_status = MRS_UPG_STAT_INPROGRESS;
            if (upg_ctx->upg_type == MRS_CCO_UPG_FILE_FLAG_STA) {
                /* Waiting for network ok */
                mrs_timer_start(MRS_TIMER_ID_UPG_BASE_STATUS_POLLING,
                                MRS_UPG_STATUS_QUERY_TIMEOUT * 30, EXT_TIMER_TYPE_ONCE); /* 30 * 2000 */
            }

            return;
        }

        if (upg_ctx->upg_stage == MRS_UPG_STAGE_INPROGRESS) {
            upg_ctx->upg_status = MRS_UPG_STAT_UNCOMPLETE;
            mrs_upg_cleanup();
        }
    }

    if ((ind->upg_status == EXT_UPG_STATUS_FAIL) || (ind->upg_status == EXT_UPG_STATUS_UNKNOWN)) {
        upg_ctx->upg_status = MRS_UPG_STAT_UNCOMPLETE;
        mrs_upg_cleanup();
    }
}

mrs_upg_stat_ctx *mrs_upg_get_stat_ctx(td_void)
{
    if (g_mrs_upg_stat_ctx == TD_NULL) {
        g_mrs_upg_stat_ctx = (mrs_upg_stat_ctx *)mrs_malloc(sizeof(mrs_upg_stat_ctx));
        if (g_mrs_upg_stat_ctx == TD_NULL) {
            return TD_NULL;
        }
    }

    return g_mrs_upg_stat_ctx;
}

td_void mrs_upg_stat_info_resp(td_pbyte resp, td_u16 resp_len)
{
    ext_upg_stat_info_ind *ind = (ext_upg_stat_info_ind *)resp;

    if ((resp == TD_NULL) || (resp_len != sizeof(ext_upg_stat_info_ind))) {
        return;
    }

    if (ind->stat_info.ok_num == 0) {
        return;
    }

    if (mrs_upg_get_stat_ctx() == TD_NULL) {
        return;
    }

    (td_void) memset_s(g_mrs_upg_stat_ctx, sizeof(mrs_upg_stat_ctx), 0, sizeof(mrs_upg_stat_ctx));

    g_mrs_upg_stat_ctx->seq = (td_u8)uapi_get_random_num32(1, 255); /* random number: min 1, max 255 */
    g_mrs_upg_stat_ctx->remain = ind->stat_info.ok_num + ind->stat_info.fail_num - 1; /* remove cco result */
    g_mrs_upg_stat_ctx->num = ind->stat_info.ok_num - 1; /* remove cco result */
}

td_void mrs_upg_set_map_bit(td_u16 idx)
{
    mrs_power_failure_bit_set(g_mrs_upg_stat_ctx->entry_map, idx);
}

td_u16 mrs_upg_get_node_map_idx(EXT_CONST td_u8 *mac, EXT_CONST td_u16 mac_len)
{
    /* return value: 0:failed, others:tei */
    ext_mdm_nm_topo_node_info *topo_info = TD_NULL;
    td_u16 num = uapi_get_topo_node_num();

    if (mac_len != EXT_METER_ADDR_LEN) {
        return 0;
    }

    uapi_get_topo_info(&topo_info);
    if (topo_info == TD_NULL) {
        return 0;
    }
    for (td_u16 i = 1; i < num; i++) {
        if (memcmp(mac, topo_info[i].mac, mac_len) == 0) {
            return i;
        }
    }
    return 0;
}

td_bool mrs_upg_check_map_bit(td_u16 idx)
{
    return mrs_power_failure_bit_check(g_mrs_upg_stat_ctx->entry_map, idx);
}

/* Got response of querying all network information */
td_void mrs_upg_status_resp(td_pbyte resp, td_u16 resp_len)
{
    ext_upg_status_ind *ind = (ext_upg_status_ind *)resp;
    upg_status_ind_entry *entry = TD_NULL;
    td_u32 num = 0;
    td_u8 mac[MRS_METER_ADDR_LEN] = { 0 };

    if ((resp == TD_NULL) || (resp_len < sizeof(ext_upg_status_ind))) {
        return;
    }

    mrs_timer_stop(MRS_TIMER_ID_UPG_NODE_STATUS_TIMEOUT);
    uapi_get_my_mac(mac, sizeof(mac));
    if ((ind->status_ind.num == 1) && (memcmp(mac, ind->status_ind.entry->mac, MRS_METER_ADDR_LEN) == 0)) {
        /* Skip CCO upgrade status */
        return;
    }

    if (g_mrs_upg_stat_ctx == TD_NULL) { /* ctx has init at mrs_upg_stat_info_resp  */
        return;
    }

    for (num = 0; num < ind->status_ind.num; num++) {
        entry = ind->status_ind.entry + num;
        if (entry->upg_status == EXT_UPG_STATUS_OK) {
            if (g_mrs_upg_stat_ctx->num < g_mrs_upg_stat_ctx->cnt + 1) {
                continue;
            }
            td_u16 idx = mrs_upg_get_node_map_idx(entry->mac, sizeof(entry->mac));
            if (idx == 0) {
                continue;
            }

            mrs_upg_set_map_bit(idx);
            g_mrs_upg_stat_ctx->cnt++;

            if (g_mrs_upg_stat_ctx->ok_addr_valid < 3) { /* 3 is enough */
                mrs_upg_get_sw_version(idx);
                g_mrs_upg_stat_ctx->ok_addr_valid++;
            }
        } else { /* upgrade failed */
            mrs_upg_node_report_fail(entry->mac, sizeof(entry->mac));
            if (g_mrs_upg_stat_ctx->remain != 0) {
                g_mrs_upg_stat_ctx->remain--;
            }
        }
    }

    if (g_mrs_upg_stat_ctx->remain == 0) { /* No more to report */
        mrs_upg_cleanup();
        mrs_upg_get_sw_ver_ctx_cleanup();
    }
}

td_void mrs_save_file_inf(td_u8 upg_type, td_u32 file_crc)
{
    app_upg_inf nv;

    (td_void) memset_s(&nv, sizeof(nv), 0, sizeof(nv));
    (td_void) uapi_nv_read(ID_NV_APP_UPG_INF, (EXT_CONST td_pvoid)&nv, sizeof(nv));
    nv.filc_crc = file_crc;
    nv.file_attr = upg_type;
    (td_void) uapi_nv_write(ID_NV_APP_UPG_INF, (EXT_CONST td_pvoid)&nv, sizeof(nv));
}

td_void poweron_active_query(td_void)
{
    (td_void) mrs_upg_status(EXT_UPG_STATUS_REQ_OPT_BS);
    mrs_timer_start(MRS_TIMER_ID_UPG_BASE_STATUS_POLLING, MRS_UPG_STATUS_QUERY_TIMEOUT, EXT_TIMER_TYPE_ONCE);
}

td_void mrs_upg_get_sw_version(EXT_CONST td_u16 idx)
{
    mrs_plc_frame_data plc;
    mrs_query_sta_inf *inf = (mrs_query_sta_inf *)mrs_malloc(sizeof(mrs_query_sta_inf) + 1);
    ext_mdm_nm_topo_node_info *topo_info = TD_NULL;
    td_u8 *addr = TD_NULL;
    errno_t err;

    if (inf == TD_NULL) {
        return;
    }

    inf->stru_ver = 1;
    inf->stru_size = sizeof(mrs_query_sta_inf);
    inf->seq = g_mrs_upg_stat_ctx->seq++;
    inf->element_cnt = 1;
    inf->element[0] = MRS_QUERY_SW_VERSION;

    plc.id = PLC_CMD_ID_QRY_STA_INF;
    uapi_get_topo_info(&topo_info);
    if (topo_info == TD_NULL) {
        mrs_free(inf);
        return;
    }

    g_mrs_upg_stat_ctx->qry_idx[g_mrs_upg_stat_ctx->ok_addr_valid] = idx;
    addr = topo_info[idx].mac;
    err = memcpy_s(plc.addr, sizeof(plc.addr), addr, MRS_METER_ADDR_LEN);
    if (err != EOK) {
        mrs_free(inf);
        return;
    }

    plc.bc_flag = 0;
    plc.option = 0;
    plc.payload = (td_u8 *)inf;
    plc.payload_len = sizeof(mrs_query_sta_inf) + 1;

    mrs_plc_frame_send(&plc);
    mrs_free(inf);

    mrs_timer_start(MRS_TIMER_ID_UPG_GET_SW_VER, MRS_UPG_STATUS_QUERY_INTERVAL, EXT_TIMER_TYPE_ONCE);
}

td_void mrs_upg_get_sw_ver_ctx_cleanup(td_void)
{
    if (g_mrs_upg_stat_ctx != TD_NULL) {
        mrs_free(g_mrs_upg_stat_ctx);
        g_mrs_upg_stat_ctx = TD_NULL;
    }
}

td_void mrs_cco_upg_get_sw_ver_rx(const td_u8 addr[MRS_METER_ADDR_LEN], td_u8 *ver_str, td_u16 ver_len)
{
    td_bool found = TD_FALSE;

    if ((g_mrs_upg_stat_ctx == TD_NULL) || (ver_str == TD_NULL) || (ver_len == 0)) {
        return;
    }

    if (g_mrs_upg_stat_ctx->ok_addr_valid == MRS_UPG_INVALID) { /* Got version before */
        return;
    }

    if (ver_len > MRS_SW_VERSION_MAX) {
        ver_len = MRS_SW_VERSION_MAX;
    }

    if (g_mrs_upg_stat_ctx->ok_addr_valid > 0) {
        ext_mdm_nm_topo_node_info *topo_info = TD_NULL;
        uapi_get_topo_info(&topo_info);
        if (topo_info == TD_NULL) {
            return;
        }
        for (td_u16 i = 0; i < g_mrs_upg_stat_ctx->ok_addr_valid; i++) {
            if (g_mrs_upg_stat_ctx->qry_idx[i] == 0) {
                continue;
            }
            if (memcmp(topo_info[g_mrs_upg_stat_ctx->qry_idx[i]].mac, addr, MRS_METER_ADDR_LEN) == 0) {
                mrs_timer_stop(MRS_TIMER_ID_UPG_GET_SW_VER);
                found = TD_TRUE;
                g_mrs_upg_stat_ctx->ok_addr_valid = MRS_UPG_INVALID;
            }
        }

        if (found != TD_TRUE) { /* Response doesn't belong to last time upgrade */
            return;
        }

        if (memcpy_s(g_mrs_upg_stat_ctx->ver, MRS_SW_VERSION_MAX, ver_str, ver_len) != EOK) {
            return;
        }

        if (g_mrs_upg_stat_ctx->cnt == g_mrs_upg_stat_ctx->num) {
            mrs_upg_node_report(EXT_ERR_SUCCESS, g_mrs_upg_stat_ctx->ver, ver_len);
        } else {
            mrs_timer_start(MRS_TIMER_ID_UPG_WAIT_RESP_DONE, MRS_UPG_STATUS_QUERY_TIMEOUT, EXT_TIMER_TYPE_ONCE);
        }
    }
}

#endif

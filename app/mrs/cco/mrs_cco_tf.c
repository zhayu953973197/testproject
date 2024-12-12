/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS CCO Transfomer Identify handle.
 */

#include "mrs_cco_tf.h"
#include "mrs_common_tools.h"
#include "mrs_cco_srv.h"
#include "mrs_cco_queue.h"
#include "mrs_proto_1376_2.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)

#define MRS_TF_SEARCH_METER_REPORT_NUM 8
#define MRS_TF_METER_NODE_SIZE         12

#define MRS_TF_PROCESS_FINISHED 2
#define MRS_TF_REPORT_CHECK_CNT 5 /* Number of cycles that meet the reporting conditions but are not reported */

/* STA's identify result */
typedef struct {
    td_u8 addr[METER_ADDR_LEN]; /* Address to be reported to the concentrator. If the address is a meter, the address
                                    is a meter address. If the address is a repeater, the MAC address of the repeater is
                                    directly reported. */
    td_u8 mac[EXT_PLC_MAC_ADDR_LEN];

    td_u16 tei : 12;
    td_u16 period_num : 4; /* Number of periods when the node meets the reporting condition. If the value is greater
                               than or equal to 5, the node needs to report the number of cycles immediately. */
    td_u8 protocol : 4;
    td_u8 type : 4;
    td_u8 pad;

    td_bool is_ack; /* is receive ack */
    td_u8 result;   /* Transformer district identification result */
    td_u8 query_cnt;
    td_bool is_report; /* Whether it has been reported to the concentrator */
} mrs_cco_tf_sta_result;

/* CCO transformer district identification context */
typedef struct {
    td_u8 status;              /* identify state */
    td_bool last_white_switch; /* Indicates whether to enable the whitelist function before the identification. */
    td_u16 plc_seq;            /* PLC packet sequence number */

    td_u32 begin_time;        /* unit:s */
    td_u32 identify_duration; /* unit:s */
    td_u32 report_duration;   /* Identification completion, reporting timeÂ£Â¬unit:s */

    td_u16 sta_num;
    td_u16 query_idx;

    td_u16 report_idx;      /* Indicates the reported STA index. */
    td_u16 afn06_f4_length; /* Indicates the reported 06F4 frame length. */

    td_u16 p1376_seq;       /* Indicates the sequence number of the packet sent to the collector. */
    td_u8 afn06_f4_buf[97]; /* 97 bytes = (1+ MRS_TF_SEARCH_METER_REPORT_NUM x MRS_TF_METER_NODE_SIZE) */
    td_bool is_show_dfx_log;

    td_s8 query_pend_cnt;
    td_u8 pad[3]; /* reserved 3B */

    mrs_cco_tf_sta_result sta_results[0];
} mrs_cco_tf_ctx;

td_void mrs_cco_clear_sta_tf_result(td_void);
td_void mrs_cco_delete_invalid_sta_record(ext_mdm_nm_topo_node_info *topo_infos);
td_void mrs_cco_realign_sta_record(td_void);
td_void mrs_cco_add_new_sta_record(ext_mdm_nm_topo_node_info *topo_infos);
td_u32 mrs_cco_send_tf_query_cmd(td_u8 sta_mac[METER_ADDR_LEN]);
td_bool mrs_cco_tf_is_notify_complete(td_void);
td_void mrs_cco_tf_result_report(td_pvoid param);
td_void mrs_cco_tf_report_06_f4(td_void);
td_void mrs_cco_tf_report_06_f3(td_u8 status);
td_u16 mrs_cco_not_report_num(EXT_OUT td_bool *need_report);

mrs_cco_tf_ctx *g_mrs_cco_tf_ctx_hdl = TD_NULL;
mrs_cco_tf_inf g_mrs_tf_conf;

mrs_cco_tf_ctx *mrs_cco_get_tf_ctx(td_void)
{
    return g_mrs_cco_tf_ctx_hdl;
}

mrs_cco_tf_inf *mrs_cco_get_tf_conf(td_void)
{
    return &g_mrs_tf_conf;
}

td_u32 mrs_cco_tf_refresh_nv_para(td_void)
{
    mrs_cco_tf_inf *tf_conf = mrs_cco_get_tf_conf();
    mrs_cco_tf_inf nv_data;
    td_u32 ret1;
    td_u32 ret2;

    if (tf_conf == TD_NULL) {
        return EXT_ERR_NO_INITILIZATION;
    }

    ret1 = uapi_nv_read(ID_NV_APP_MRS_CCO_TF, (td_pvoid)&nv_data, sizeof(nv_data));
    ret2 = (td_u32)memcpy_s(tf_conf, sizeof(mrs_cco_tf_inf), &nv_data, sizeof(mrs_cco_tf_inf));
    if ((ret1 != EXT_ERR_SUCCESS) || (ret2 != EXT_ERR_SUCCESS)) {
        tf_conf->identify_duration = IDENTIFY_DURATION_DEFAULT;
        tf_conf->identify_duration_rate = IDENTIFY_DURATION_RATE;
        tf_conf->report_duration = REPORT_DURATION_DEFAULT;
        tf_conf->start_cmd_interval = START_CMD_SEND_INTERVAL;
        tf_conf->stop_cmd_interval = SEND_STOP_CMD_INTERVAL;
        tf_conf->query_sta_interval = QUERY_STA_INTERVAL;
        tf_conf->refrash_topo_interval = REFRESH_STA_LIST_INTERVAL;
        tf_conf->query_sta_timeout = QUERY_STA_TIMEOUT;
        tf_conf->query_max_cnt = QUERY_MAX_CNT;
        tf_conf->delay_white_list_duration = DELAY_WHITE_LIST_DEFAULT;
        uapi_nv_write(ID_NV_APP_MRS_CCO_TF, (td_pvoid)tf_conf, sizeof(mrs_cco_tf_inf));
    }
    return EXT_ERR_SUCCESS;
}

td_void mrs_cco_tf_dfx_report_message(EXT_IN td_u16 id, EXT_IN td_pvoid packet, EXT_IN td_u16 packet_size)
{
    mrs_cco_tf_ctx *tf_ctx = mrs_cco_get_tf_ctx();
    if (tf_ctx != TD_NULL) {
        if (tf_ctx->is_show_dfx_log == TD_FALSE) {
            return;
        }
        ext_diag_layer_msg layer_msg;
        layer_msg.id = id;
        layer_msg.src_mod_id = EXT_MOD_ID_APP_COMMON;
        layer_msg.dest_mod_id = EXT_MOD_ID_APP_COMMON;
        layer_msg.data = packet;
        layer_msg.data_size = packet_size;
        (td_void) diag_report_sys_msg(layer_msg, EXT_MSG_SYS_L3);
    }
}

td_void mrs_cco_reset_all_timer(td_void)
{
    mrs_timer_stop(MRS_TIMER_ID_TF_SEND_START_CMD);
    mrs_timer_stop(MRS_TIMER_ID_TF_SEND_STOP_CMD);
    mrs_timer_stop(MRS_TIMER_ID_TF_REFRESH_STA_LIST);
    mrs_timer_stop(MRS_TIMER_ID_TF_IDENTIFY_PROC);
    mrs_timer_stop(MRS_TIMER_ID_TF_REPORT_PROC);
    mrs_timer_stop(MRS_TIMER_ID_TF_QUERY_INTERVAL);
    mrs_timer_stop(MRS_TIMER_ID_TF_COMPLETE);
    mrs_timer_stop(MRS_TIMER_ID_TF_QUERY_TIMEOUT);
}

/* Starts the transformer district identification. It is invoked when the concentrator AFN11-F5 is received or enabled
   through the APP. */
td_u32 mrs_cco_start_tf_identify(td_u32 duration)
{
    mrs_cco_srv_ctx *cco = mrs_cco_get_srv_ctx();
    mrs_cco_tf_inf *tf_conf = mrs_cco_get_tf_conf();
    td_u32 len = sizeof(mrs_cco_tf_ctx) + sizeof(mrs_cco_tf_sta_result) * EXT_MDM_NM_TOPO_NODE_NUM_MAX;
    mrs_cco_tf_ctx *tf_ctx = TD_NULL;

    if (g_mrs_cco_tf_ctx_hdl == TD_NULL) {
        g_mrs_cco_tf_ctx_hdl = (mrs_cco_tf_ctx *)uapi_malloc(EXT_MOD_ID_APP_COMMON, len);
        if (g_mrs_cco_tf_ctx_hdl == TD_NULL) {
            return EXT_ERR_MALLOC_FAILURE;
        }
        (td_void) memset_s(g_mrs_cco_tf_ctx_hdl, len, 0, len);

        if (mrs_cco_tf_refresh_nv_para() != EXT_ERR_SUCCESS) {
            uapi_free(EXT_MOD_ID_APP_COMMON, g_mrs_cco_tf_ctx_hdl);
            g_mrs_cco_tf_ctx_hdl = TD_NULL;
            return EXT_ERR_FAILURE;
        }
    }
    tf_ctx = g_mrs_cco_tf_ctx_hdl;
    tf_ctx->is_show_dfx_log = TD_TRUE;

    tf_ctx->identify_duration = ext_minute_2_sec(duration) * tf_conf->identify_duration_rate / 100; /* 100 ms */
    tf_ctx->report_duration = ext_minute_2_sec(duration) - tf_ctx->identify_duration;

    if (tf_ctx->identify_duration == 0) {
        tf_ctx->identify_duration = ext_minute_2_sec(tf_conf->identify_duration);
        tf_ctx->report_duration = ext_minute_2_sec(tf_conf->report_duration);
    }

    if (tf_ctx->status != MRS_TF_STATUE_IDENTIFYING && tf_ctx->status != MRS_TF_STATUS_REPORT_RESULT) {
        if (tf_ctx->last_white_switch != TD_TRUE) {
            (td_void) uapi_get_white_list_switch(&tf_ctx->last_white_switch);
        }
        (td_void) uapi_set_white_list_switch(TD_FALSE, TD_FALSE, EXT_MAC_WHITE_LIST_TF_IDENTIFY);

        mrs_cco_clear_sta_tf_result();
        mrs_cco_refresh_sta_tf_record();
    }
    tf_ctx->begin_time = uapi_get_seconds();
    tf_ctx->status = MRS_TF_STATUE_IDENTIFYING;

    mrs_cco_send_tf_start_cmd();
    mrs_cco_reset_all_timer();

    cco->status = MRS_CCO_STATUS_SEARCH_METER;

    mrs_timer_start(MRS_TIMER_ID_TF_IDENTIFY_PROC, uapi_sec_2_ms(tf_ctx->identify_duration), EXT_TIMER_TYPE_ONCE);
    mrs_timer_start(MRS_TIMER_ID_TF_SEND_START_CMD, uapi_sec_2_ms(tf_conf->start_cmd_interval), EXT_TIMER_TYPE_PERIOD);
    mrs_timer_start(MRS_TIMER_ID_TF_REFRESH_STA_LIST, uapi_sec_2_ms(tf_conf->refrash_topo_interval),
                    EXT_TIMER_TYPE_PERIOD);

    return EXT_ERR_SUCCESS;
}

/* Query the registration result of the slave node. */
td_void mrs_cco_query_tf_identify(td_void)
{
    mrs_cco_tf_ctx *tf_ctx = mrs_cco_get_tf_ctx();
    mrs_cco_tf_inf *tf_conf = mrs_cco_get_tf_conf();

    if (tf_ctx == TD_NULL) {
        return;
    }
    mrs_timer_stop(MRS_TIMER_ID_TF_SEND_START_CMD); /* Stop sending the START command periodically. */

    tf_ctx->status = MRS_TF_STATUS_REPORT_RESULT;
    mrs_timer_start(MRS_TIMER_ID_TF_REPORT_PROC, uapi_sec_2_ms(tf_ctx->report_duration), EXT_TIMER_TYPE_ONCE);

    mrs_timer_start(MRS_TIMER_ID_TF_QUERY_INTERVAL, uapi_sec_2_ms(tf_conf->query_sta_interval), EXT_TIMER_TYPE_ONCE);

    tf_ctx->query_idx = 0;
}

td_void mrs_cco_stop_tf_identify(td_void)
{
    td_u16 cnt;
    mrs_cco_tf_inf *tf_conf = mrs_cco_get_tf_conf();
    mrs_cco_tf_ctx *tf_ctx = mrs_cco_get_tf_ctx();
    mrs_cco_srv_ctx *cco = mrs_cco_get_srv_ctx();
    td_bool need_report = TD_FALSE;

    mrs_cco_send_tf_stop_cmd(); /* Send the command for stopping the slave node registration. */
    if (tf_ctx == TD_NULL) {
        return;
    }
    cnt = mrs_cco_not_report_num(&need_report);
    if (cnt > 0) {
        mrs_cco_tf_result_report(TD_NULL);
    }

    tf_ctx->status = MRS_TF_STATUS_COMPLETED;
    mrs_cco_tf_report_06_f3(MRS_TF_PROCESS_FINISHED);

    cco->status = MRS_CCO_STATUS_IDLE;
    tf_ctx->p1376_seq = 0;
    tf_ctx->query_idx = 0;
    tf_ctx->query_pend_cnt = 0;

    mrs_cco_reset_all_timer();

    mrs_timer_start(MRS_TIMER_ID_TF_SEND_STOP_CMD, uapi_sec_2_ms(tf_conf->stop_cmd_interval), EXT_TIMER_TYPE_PERIOD);
    mrs_timer_start(MRS_TIMER_ID_TF_COMPLETE, uapi_sec_2_ms(tf_conf->delay_white_list_duration), EXT_TIMER_TYPE_ONCE);
}

/* Clear the identification result. */
td_void mrs_cco_clear_sta_tf_result(td_void)
{
    mrs_cco_tf_ctx *tf_ctx = mrs_cco_get_tf_ctx();
    td_u32 clear_size;

    if (tf_ctx == TD_NULL) {
        return;
    }
    tf_ctx->sta_num = 0;
    clear_size = sizeof(mrs_cco_tf_sta_result) * EXT_MDM_NM_TOPO_NODE_NUM_MAX;
    (td_void) memset_s(tf_ctx->sta_results, clear_size, 0, clear_size);
}

/* Update the identification result list based on the latest topology. */
td_void mrs_cco_refresh_sta_tf_record(td_void)
{
    ext_mdm_nm_topo_node_info *topo_infos = TD_NULL;

    uapi_get_topo_info(&topo_infos);
    if (topo_infos == TD_NULL) {
        return;
    }
    mrs_cco_delete_invalid_sta_record(topo_infos);
    mrs_cco_realign_sta_record();
    mrs_cco_add_new_sta_record(topo_infos);
}

/* Delete the stations that are not in the topology from the identification result list. */
td_void mrs_cco_delete_invalid_sta_record(ext_mdm_nm_topo_node_info *topo_infos)
{
    mrs_cco_tf_ctx *tf_ctx = mrs_cco_get_tf_ctx();
    td_u16 i;
    td_u16 j;

    if (tf_ctx == TD_NULL) {
        return;
    }

    for (i = 0; i < tf_ctx->sta_num; i++) {
        for (j = 0; j < EXT_MDM_NM_TOPO_NODE_NUM_MAX; j++) {
            if (memcmp(tf_ctx->sta_results[i].mac, topo_infos[j].mac, EXT_PLC_MAC_ADDR_LEN) == 0 &&
                topo_infos[j].state != STATE_NOT_USED) {
                break;
            }
        }
        if (j >= EXT_MDM_NM_TOPO_NODE_NUM_MAX) {
            (td_void) memset_s(&tf_ctx->sta_results[i], sizeof(tf_ctx->sta_results[i]),
                               0, sizeof(tf_ctx->sta_results[i]));
        }
    }
}

/* Move the valid records in the identification result list to the top. */
td_void mrs_cco_realign_sta_record(td_void)
{
    mrs_cco_tf_ctx *tf_ctx = mrs_cco_get_tf_ctx();
    td_u16 i;
    td_u16 j = 0;
    errno_t ret;

    /* Find the first invalid node. */
    while (tf_ctx->sta_results[j].tei > 0 && j < tf_ctx->sta_num) {
        j++;
    }

    /* Move the valid records forward. */
    for (i = j + 1; i < tf_ctx->sta_num; i++) {
        if (tf_ctx->sta_results[i].tei == 0) {
            continue;
        }

        ret = memcpy_s(&tf_ctx->sta_results[j], sizeof(tf_ctx->sta_results[j]), &tf_ctx->sta_results[i],
                       sizeof(tf_ctx->sta_results[i]));
        if (ret == EXT_ERR_SUCCESS) {
            j++;
        }
    }
    tf_ctx->sta_num = j;
}

/* Add the points that are not included in the identification result list in the topology view. */
td_void mrs_cco_add_new_sta_record(ext_mdm_nm_topo_node_info *topo_infos)
{
    mrs_cco_tf_ctx *tf_ctx = mrs_cco_get_tf_ctx();
    td_u16 topo_idx;
    td_u16 record_idx;
    mrs_cco_tf_sta_result *sta_list = TD_NULL;

    if (tf_ctx == TD_NULL) {
        return;
    }
    sta_list = tf_ctx->sta_results;

    for (topo_idx = 0; topo_idx < EXT_MDM_NM_TOPO_NODE_NUM_MAX; topo_idx++) {
        if (topo_infos[topo_idx].state == STATE_NOT_USED || topo_idx + 1 <= 1) {
            continue;
        }

        for (record_idx = 0; record_idx < tf_ctx->sta_num; record_idx++) {
            if (memcmp(topo_infos[topo_idx].mac, sta_list[record_idx].mac, EXT_PLC_MAC_ADDR_LEN) == 0) {
                break;
            }
        }

        if (record_idx >= tf_ctx->sta_num && tf_ctx->sta_num < EXT_MDM_NM_TOPO_NODE_NUM_MAX) {
            (td_void) memset_s(&sta_list[tf_ctx->sta_num], sizeof(sta_list[tf_ctx->sta_num]), 0,
                               sizeof(sta_list[tf_ctx->sta_num]));
            sta_list[tf_ctx->sta_num].tei = topo_idx + 1;
            if (memcpy_s(sta_list[tf_ctx->sta_num].mac, sizeof(sta_list[tf_ctx->sta_num].mac),
                         topo_infos[topo_idx].mac, EXT_PLC_MAC_ADDR_LEN) != EXT_ERR_SUCCESS) {
                continue;
            }
            tf_ctx->sta_num++;
        }
    }
}

/* The CCO sends a command to the STA to identify the transformer district. */
td_void mrs_cco_send_tf_start_cmd(td_void)
{
    mrs_cco_tf_ctx *tf_ctx = mrs_cco_get_tf_ctx();
    mrs_plc_frame_data plc;
    mrs_plc_start_search_meter tf_cmd;

    if (tf_ctx == TD_NULL) {
        return;
    }
    tf_ctx->plc_seq++;

    (td_void) memset_s(&tf_cmd, sizeof(tf_cmd), 0, sizeof(tf_cmd));
    tf_cmd.interface_ver = MRS_PLC_PROTO_VERSION;
    tf_cmd.stru_len = sizeof(tf_cmd);
    tf_cmd.force_resp = 0;
    tf_cmd.option = MRS_SM_START_SEARCH;
    tf_cmd.seq = tf_ctx->plc_seq;

    (td_void) memset_s(&plc, sizeof(plc), 0, sizeof(plc));
    plc.id = PLC_CMD_ID_START_SEARCH;
    plc.payload_len = sizeof(tf_cmd);
    plc.payload = (td_pbyte)&tf_cmd;
    plc.bc_flag = TD_TRUE;
    (td_void) memset_s(plc.addr, sizeof(plc.addr), 0xFF, EXT_PLC_MAC_ADDR_LEN);

    mrs_cco_tf_dfx_report_message(EXT_DSID_APP_MRS_TF_CCO_PLC_START_SEND, (td_pvoid)&tf_cmd, sizeof(tf_cmd));

    mrs_plc_frame_send(&plc);
}

/* Query the number of nodes that report conditions but are not reported to the concentrator. If a point meets the
   reporting condition and is not reported for five times, it needs to be reported immediately. */
td_u16 mrs_cco_not_report_num(EXT_OUT td_bool *need_report)
{
    td_u16 num = 0;
    td_u16 i;
    mrs_cco_tf_ctx *tf_ctx = mrs_cco_get_tf_ctx();
    mrs_cco_tf_sta_result *sta_list = tf_ctx->sta_results;

    for (i = 0; i < tf_ctx->sta_num; i++) {
        if (sta_list[i].query_cnt == 0) {
            return num;
        }

        if (sta_list[i].is_ack == TD_TRUE && sta_list[i].is_report == TD_FALSE &&
            sta_list[i].result == MRS_TF_RESULT_BELONG) {
            if (num == 0) {
                tf_ctx->report_idx = i; /* Indicates the start position of the report. */
            }
            if (sta_list[i].period_num >= MRS_TF_REPORT_CHECK_CNT) {
                *need_report = TD_TRUE;
            }
            sta_list[i].period_num++;
            num++;
        }
    }
    return num;
}

/* Querying the Slave Node Registration Process */
td_void mrs_cco_query_tf_proc(td_void)
{
    mrs_cco_tf_ctx *tf_ctx = mrs_cco_get_tf_ctx();
    mrs_cco_tf_inf *tf_conf = mrs_cco_get_tf_conf();
    td_u8 query_cnt = 0;
    td_u16 i;
    td_u16 cnt;
    td_bool need_report = TD_FALSE;
    mrs_cco_tf_sta_result *sta_list = TD_NULL;
    if (tf_ctx == TD_NULL) {
        return;
    }

    sta_list = tf_ctx->sta_results;
    for (i = tf_ctx->query_idx; i < tf_ctx->sta_num && query_cnt < tf_conf->query_sta_max_way; i++) {
        if (sta_list[i].is_ack == TD_TRUE || sta_list[i].query_cnt > tf_conf->query_max_cnt) {
            continue;
        }

        if (mrs_cco_send_tf_query_cmd(sta_list[i].mac) != EXT_ERR_SUCCESS) {
            break;
        }
        sta_list[i].query_cnt++;
        query_cnt++;

        tf_ctx->query_pend_cnt++;
    }

    if (i >= tf_ctx->sta_num) {
        tf_ctx->query_idx = 0;
    } else {
        tf_ctx->query_idx = i;
    }

    cnt = mrs_cco_not_report_num(&need_report);
    if (cnt >= MRS_TF_SEARCH_METER_REPORT_NUM || need_report == TD_TRUE) {
        mrs_cco_tf_result_report(TD_NULL);
    }

    if (tf_ctx->status == MRS_TF_STATUS_COMPLETED) {
        return;
    }

    if (tf_ctx->query_pend_cnt > 0) {
        mrs_timer_start(MRS_TIMER_ID_TF_QUERY_TIMEOUT, uapi_sec_2_ms(tf_conf->query_sta_timeout), EXT_TIMER_TYPE_ONCE);
    } else {
        mrs_timer_stop(MRS_TIMER_ID_TF_QUERY_INTERVAL);
        mrs_timer_start(MRS_TIMER_ID_TF_QUERY_INTERVAL, uapi_sec_2_ms(tf_conf->query_sta_interval),
            EXT_TIMER_TYPE_ONCE);
    }
}

td_u32 mrs_cco_send_tf_query_cmd(td_u8 sta_mac[EXT_PLC_MAC_ADDR_LEN])
{
    mrs_cco_tf_ctx *tf_ctx = mrs_cco_get_tf_ctx();
    mrs_plc_frame_data plc;
    mrs_plc_meterlist_dl tf_cmd;
    td_u8 cco_mac[EXT_PLC_MAC_ADDR_LEN] = { 0 };

    if (tf_ctx == TD_NULL) {
        return EXT_ERR_FAILURE;
    }
    tf_ctx->plc_seq++;
    (td_void) memset_s(&tf_cmd, sizeof(tf_cmd), 0, sizeof(tf_cmd));
    tf_cmd.interface_ver = MRS_PLC_PROTO_VERSION;
    tf_cmd.stru_len = sizeof(tf_cmd);
    tf_cmd.option = MRS_SM_SEARCH_RESULT;
    tf_cmd.enable = MRS_TF_DISABLE;
    tf_cmd.seq = tf_ctx->plc_seq;
    uapi_get_my_mac(cco_mac, sizeof(cco_mac));
    (td_void) memcpy_s(tf_cmd.src, sizeof(tf_cmd.src), cco_mac, EXT_PLC_MAC_ADDR_LEN);
    (td_void) memcpy_s(tf_cmd.dst, sizeof(tf_cmd.dst), sta_mac, EXT_PLC_MAC_ADDR_LEN);

    (td_void) memset_s(&plc, sizeof(plc), 0, sizeof(plc));
    plc.id = PLC_CMD_ID_METER_LIST;
    plc.payload_len = sizeof(tf_cmd);
    plc.payload = (td_pbyte)&tf_cmd;

    if (memcpy_s(plc.addr, sizeof(plc.addr), sta_mac, EXT_PLC_MAC_ADDR_LEN) != EXT_ERR_SUCCESS) {
        return EXT_ERR_FAILURE;
    }

    mrs_cco_tf_dfx_report_message(EXT_DSID_APP_MRS_TF_CCO_PLC_QUERY_SEND, (td_pvoid *)&tf_cmd, sizeof(tf_cmd));
    return mrs_plc_frame_send(&plc);
}

/* The CCO sends a stop command to the STA. */
td_void mrs_cco_send_tf_stop_cmd(td_void)
{
    mrs_cco_tf_ctx *tf_ctx = mrs_cco_get_tf_ctx();
    mrs_plc_frame_data plc;
    mrs_plc_stop_search_meter tf_cmd;

    if (tf_ctx == TD_NULL) {
        return;
    }
    tf_ctx->plc_seq++;

    (td_void) memset_s(&tf_cmd, sizeof(tf_cmd), 0, sizeof(tf_cmd));
    tf_cmd.interface_ver = MRS_PLC_PROTO_VERSION;
    tf_cmd.stru_len = sizeof(tf_cmd);
    tf_cmd.seq = tf_ctx->plc_seq;

    (td_void) memset_s(&plc, sizeof(plc), 0, sizeof(plc));
    plc.id = PLC_CMD_ID_STOP_SEARCH;
    plc.payload_len = sizeof(mrs_plc_stop_search_meter);
    plc.payload = (td_pbyte)&tf_cmd;
    plc.bc_flag = TD_TRUE;
    (td_void) memset_s(plc.addr, sizeof(plc.addr), 0xFF, EXT_PLC_MAC_ADDR_LEN);

    mrs_cco_tf_dfx_report_message(EXT_DSID_APP_MRS_TF_CCO_PLC_STOP_SEND, (td_pvoid *)&tf_cmd, sizeof(tf_cmd));
    mrs_plc_frame_send(&plc);
}

td_void mrs_cco_get_tf_report_data(td_void)
{
    mrs_cco_tf_ctx *tf_ctx = mrs_cco_get_tf_ctx();
    td_pbyte buf;
    td_u16 offset = 1;
    td_u8 num = 0;

    buf = tf_ctx->afn06_f4_buf;
    while ((tf_ctx->report_idx < tf_ctx->sta_num) && (num < MRS_TF_SEARCH_METER_REPORT_NUM)) {
        if (tf_ctx->sta_results[tf_ctx->report_idx].result == MRS_TF_RESULT_BELONG &&
            tf_ctx->sta_results[tf_ctx->report_idx].is_report == TD_FALSE &&
            !mrs_invalid_meter(tf_ctx->sta_results[tf_ctx->report_idx].addr)) {
            num++;
            tf_ctx->p1376_seq++;
            /* addr */
            buf[offset++] = tf_ctx->sta_results[tf_ctx->report_idx].addr[0]; /* addr 0 byte */
            buf[offset++] = tf_ctx->sta_results[tf_ctx->report_idx].addr[1]; /* addr 1 byte */
            buf[offset++] = tf_ctx->sta_results[tf_ctx->report_idx].addr[2]; /* addr 2 byte */
            buf[offset++] = tf_ctx->sta_results[tf_ctx->report_idx].addr[3]; /* addr 3 byte */
            buf[offset++] = tf_ctx->sta_results[tf_ctx->report_idx].addr[4]; /* addr 4 byte */
            buf[offset++] = tf_ctx->sta_results[tf_ctx->report_idx].addr[5]; /* addr 5 byte */

            buf[offset++] = tf_ctx->sta_results[tf_ctx->report_idx].protocol; /* protocol */
            buf[offset++] = (td_u8)(tf_ctx->p1376_seq & 0xff);                /* seq: Lo */
            buf[offset++] = (td_u8)((tf_ctx->p1376_seq >> 8) & 0xff);         /* seq: Hi ,left move 8 bits */
            buf[offset++] = tf_ctx->sta_results[tf_ctx->report_idx].type;     /* device type */
            buf[offset++] = 0;                                                /* sub node num */
            buf[offset++] = 0;                                                /* sub node num of this frame */

            tf_ctx->sta_results[tf_ctx->report_idx].is_report = TD_TRUE;
        }

        tf_ctx->report_idx++;
    }

    if (num == 0) {
        tf_ctx->afn06_f4_length = 0;
        return;
    }
    buf[0] = num;
    tf_ctx->afn06_f4_length = 1 + MRS_TF_METER_NODE_SIZE * num;
}

td_void mrs_cco_tf_report_06_f3(td_u8 status)
{
    td_u32 ret;
    mrs_proto_1376_2_encode encode;
    mrs_cco_srv_ctx *cco = mrs_cco_get_srv_ctx();
    mrs_cco_queue_item *item = TD_NULL;
    td_pbyte payload = TD_NULL;
    td_u16 payload_len = 0;
    td_u8 data = status;

    memset_s(&encode, sizeof(encode), 0, sizeof(encode));
    encode.afn = 0x06; /* afn is 0x06 */
    encode.fn = 0x03;  /* fn is 0x03 */
    encode.data = &data;
    encode.length = sizeof(data);
    encode.prm = 1;
    encode.seq = ++cco->seq;
    ret = mrs_proto_1376_2_create_frame(&encode, &payload, &payload_len);
    if (ret != EXT_ERR_SUCCESS) {
        return;
    }

    item = (mrs_cco_queue_item *)mrs_malloc(sizeof(mrs_cco_queue_item) + payload_len);
    if (item == TD_NULL) {
        mrs_free(payload);
        return;
    }

    memset_s(item, sizeof(mrs_cco_queue_item) + payload_len, 0, sizeof(mrs_cco_queue_item) + payload_len);
    item->valid = TD_TRUE;
    item->afn = 0x06;
    item->fn = 0x03;
    item->retry_max = MRS_CCO_TF_REPORT_RETRY_MAX;
    item->timeout = MRS_CCO_TF_REPORT_TIMEOUT;
    item->data_len = payload_len;
    if (memcpy_s(item->data, item->data_len, payload, payload_len) != EOK) {
        mrs_free(payload);
        mrs_free(item);
        return;
    }
    mrs_cco_queue_enqueue(item);
    mrs_cco_queue_active();
    mrs_cco_tf_dfx_report_message(EXT_DSID_APP_MRS_TF_CCO_UART_SEND, (td_pvoid)payload, payload_len);
    mrs_free(payload);
}

td_void mrs_cco_tf_report_06_f4(td_void)
{
    td_u32 ret;
    mrs_cco_tf_ctx *tf_ctx = mrs_cco_get_tf_ctx();
    mrs_cco_srv_ctx *cco = mrs_cco_get_srv_ctx();
    mrs_proto_1376_2_encode encode;
    mrs_cco_queue_item *item = TD_NULL;
    td_pbyte payload = TD_NULL;
    td_u16 payload_len = 0;

    (td_void) memset_s(&encode, sizeof(encode), 0, sizeof(encode));
    encode.afn = 0x06;
    encode.fn = 0x04;
    encode.data = tf_ctx->afn06_f4_buf;
    encode.length = tf_ctx->afn06_f4_length;
    encode.prm = 1;
    encode.seq = ++cco->seq;
    ret = mrs_proto_1376_2_create_frame(&encode, &payload, &payload_len);
    if (ret != EXT_ERR_SUCCESS) {
        return;
    }

    item = (mrs_cco_queue_item *)mrs_malloc(sizeof(mrs_cco_queue_item) + payload_len);
    if (item == TD_NULL) {
        mrs_free(payload);
        return;
    }

    (td_void) memset_s(item, sizeof(mrs_cco_queue_item) + payload_len, 0, sizeof(mrs_cco_queue_item) + payload_len);
    item->valid = TD_TRUE;
    item->afn = 0x06; /* 0x06 is afn num */
    item->fn = 0x04;  /* 0x04 is fn num */
    item->retry_max = MRS_CCO_TF_REPORT_RETRY_MAX;
    item->timeout = MRS_CCO_TF_REPORT_TIMEOUT;
    item->data_len = payload_len;
    if (memcpy_s(item->data, item->data_len, payload, payload_len) != EOK) {
        mrs_free(payload);
        mrs_free(item);
        return;
    }
    item->rx_handle = mrs_cco_tf_result_report; /* This command is invoked when the cco receives the 00-f1 or 00-f2
                                                    from the concentrator. */
    mrs_cco_queue_enqueue(item);
    mrs_cco_queue_active();
    mrs_cco_tf_dfx_report_message(EXT_DSID_APP_MRS_TF_CCO_UART_SEND, (td_pvoid)payload, payload_len);
    mrs_free(payload);
}

td_void mrs_cco_tf_finished(td_void)
{
    mrs_cco_tf_ctx *tf_ctx = mrs_cco_get_tf_ctx();
    mrs_timer_stop(MRS_TIMER_ID_TF_SEND_STOP_CMD);
    if (tf_ctx == TD_NULL) {
        return;
    }
    (td_void) uapi_set_white_list_switch(tf_ctx->last_white_switch, TD_FALSE, EXT_MAC_WHITE_LIST_TF_IDENTIFY);
}

td_void mrs_cco_tf_query_timeout(td_void)
{
    mrs_cco_tf_ctx *tf_ctx = mrs_cco_get_tf_ctx();
    mrs_cco_tf_inf *tf_conf = mrs_cco_get_tf_conf();
    if (tf_ctx == TD_NULL) {
        return;
    }
    tf_ctx->query_pend_cnt = 0;
    mrs_timer_stop(MRS_TIMER_ID_TF_QUERY_INTERVAL);
    mrs_timer_start(MRS_TIMER_ID_TF_QUERY_INTERVAL, uapi_sec_2_ms(tf_conf->query_sta_interval), EXT_TIMER_TYPE_ONCE);
}

mrs_cco_tf_sta_result *mrs_cco_get_sta_tf_result(td_u8 sta_mac[EXT_PLC_MAC_ADDR_LEN])
{
    mrs_cco_tf_ctx *tf_ctx = mrs_cco_get_tf_ctx();
    td_u16 i;

    if (tf_ctx == TD_NULL) {
        return TD_NULL;
    }

    for (i = 0; i < tf_ctx->sta_num; i++) {
        if (memcmp(sta_mac, tf_ctx->sta_results[i].mac, EXT_PLC_MAC_ADDR_LEN) == 0) {
            return &tf_ctx->sta_results[i];
        }
    }
    return TD_NULL;
}

td_void mrs_cco_tf_plc_frame_rx(mrs_plc_frame_data *plc)
{
    mrs_plc_meterlist_up *tf_sta_response = TD_NULL;
    mrs_cco_tf_sta_result *sta_result = TD_NULL;
    mrs_cmd_meterlist_item *meter_item = TD_NULL;
    mrs_cco_tf_ctx *tf_ctx = mrs_cco_get_tf_ctx();
    mrs_cco_tf_inf *tf_conf = mrs_cco_get_tf_conf();
    if (tf_ctx == TD_NULL) {
        return;
    }

    tf_ctx->query_pend_cnt--;
    if (tf_ctx->query_pend_cnt <= 0) {
        tf_ctx->query_pend_cnt = 0;
        mrs_timer_stop(MRS_TIMER_ID_TF_QUERY_TIMEOUT);

        mrs_timer_stop(MRS_TIMER_ID_TF_QUERY_INTERVAL);
        mrs_timer_start(MRS_TIMER_ID_TF_QUERY_INTERVAL, uapi_sec_2_ms(tf_conf->query_sta_interval),
            EXT_TIMER_TYPE_ONCE);
    }

    if (plc == TD_NULL || plc->payload == TD_NULL ||
        plc->payload_len < sizeof(mrs_plc_meterlist_up) + sizeof(mrs_cmd_meterlist_item)) {
        return;
    }
    mrs_cco_tf_dfx_report_message(EXT_DSID_APP_MRS_TF_CCO_PLC_RECV, (td_pvoid *)plc->payload, plc->payload_len);

    tf_sta_response = (mrs_plc_meterlist_up *)plc->payload;
    meter_item = (mrs_cmd_meterlist_item *)tf_sta_response->meter_list;

    sta_result = mrs_cco_get_sta_tf_result(tf_sta_response->src);
    if (sta_result == TD_NULL) {
        return;
    }

    sta_result->result = tf_sta_response->result;
    (td_void) memcpy_s(sta_result->addr, MRS_METER_ADDR_LEN, tf_sta_response->asset, MRS_METER_ADDR_LEN);
    if (tf_sta_response->type == MRS_DEVICE_TYPE_PLC_METER) {
        sta_result->type = MRS_DEVICE_TYPE_1376_2_METER;
        if (mrs_invalid_meter(sta_result->addr)) {
            (td_void) memcpy_s(sta_result->addr, EXT_PLC_MAC_ADDR_LEN, tf_sta_response->id, EXT_PLC_MAC_ADDR_LEN);
        }
    } else if (tf_sta_response->type == MRS_DEVICE_TYPE_PLC_RELAY) {
        sta_result->type = MRS_DEVICE_TYPE_1376_2_RELAY;
    }
    sta_result->protocol = meter_item->protocol;
    sta_result->is_ack = TD_TRUE;
}

td_void mrs_cco_tf_result_report(td_pvoid param)
{
    mrs_cco_tf_ctx *tf_ctx = mrs_cco_get_tf_ctx();
    ext_unref_param(param);

    if (tf_ctx == TD_NULL) {
        return;
    }
    mrs_cco_get_tf_report_data();

    if (tf_ctx->afn06_f4_length > 0) {
        mrs_cco_tf_report_06_f4();
    }
}

td_u32 mrs_dfx_tf_ctrl(diag_cmd_tf_ctrl_req *req, td_u16 cmd_size, td_u8 option)
{
    mrs_cco_tf_ctx *tf_ctx;
    diag_cmd_tf_ctrl_ind ind;
    td_u32 pass_duration;

    if (req == TD_NULL || cmd_size < sizeof(diag_cmd_tf_ctrl_req)) {
        return EXT_ERR_INVALID_PARAMETER;
    }

    if (req->cmd_type == TF_CMD_TYPE_START_IDENTIFY) {
        mrs_cco_start_tf_identify(req->duration);
    } else if (req->cmd_type == TF_CMD_TYPE_STOP_IDENTIFY) {
        mrs_cco_stop_tf_identify();
    } else {
    }
    (td_void) memset_s(&ind, sizeof(ind), 0, sizeof(ind));

    tf_ctx = mrs_cco_get_tf_ctx();
    ind.ret = EXT_ERR_SUCCESS;
    if (tf_ctx == TD_NULL) {
        ind.tf_status = 0;
        ind.last_duration = 0;
    } else {
        ind.tf_status = tf_ctx->status;
        pass_duration = mrs_time_sub_duration(uapi_get_seconds(), tf_ctx->begin_time);
        if (tf_ctx->status == MRS_TF_STATUE_IDENTIFYING) {
            ind.last_duration = tf_ctx->identify_duration > pass_duration ?
                                (tf_ctx->identify_duration - pass_duration) : 0;
        } else if (tf_ctx->status == MRS_TF_STATUS_REPORT_RESULT) {
            ind.last_duration = (tf_ctx->identify_duration + tf_ctx->report_duration) > pass_duration ?
                                (tf_ctx->identify_duration + tf_ctx->report_duration - pass_duration) : 0;
        } else {
            ind.last_duration = 0;
        }
    }
    return uapi_diag_report_packet(ID_DIAG_CMD_TF_CTRL, option, (td_pbyte)&ind, sizeof(ind), TD_FALSE);
}

td_u32 mrs_dfx_tf_result_query(const diag_cmd_tf_result_req *req, td_u16 cmd_size, td_u8 option)
{
    mrs_cco_tf_ctx *tf_ctx = mrs_cco_get_tf_ctx();
    diag_cmd_tf_result_ind ind;
    td_u32 i;

    (td_void) memset_s(&ind, sizeof(ind), 0, sizeof(ind));
    if (tf_ctx == TD_NULL || req == TD_NULL || cmd_size < sizeof(diag_cmd_tf_result_req)) {
        return uapi_diag_report_packet(ID_DIAG_CMD_TF_RESULT_QUERY, option, (td_pbyte)&ind, sizeof(ind), TD_FALSE);
    }

    ind.total_num = tf_ctx->sta_num;
    ind.report_begin_idx = (td_u16)req->begin_idx;

    for (i = req->begin_idx; i < tf_ctx->sta_num; i++) {
        (td_void) memcpy_s(ind.results[i - req->begin_idx].sta_mac, EXT_PLC_MAC_ADDR_LEN,
                           tf_ctx->sta_results[i].mac, EXT_PLC_MAC_ADDR_LEN);
        ind.results[i - req->begin_idx].tei = tf_ctx->sta_results[i].tei;
        ind.results[i - req->begin_idx].result = tf_ctx->sta_results[i].result;

        ind.report_num++;
        if (ind.report_num >= DIAG_TF_RESULT_REPORT_NUM) {
            break;
        }
    }
    return uapi_diag_report_packet(ID_DIAG_CMD_TF_RESULT_QUERY, option, (td_pbyte)&ind, sizeof(ind), TD_FALSE);
}

#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_CCO) */


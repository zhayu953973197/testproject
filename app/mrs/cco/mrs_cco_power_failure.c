/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS CCO POWER FAILURE handle.
 */

#include "soc_mdm_mac.h"
#include "mrs_cco_evt.h"
#include "mrs_proto_1376_2.h"
#include "mrs_common_tools.h"
#include "mrs_cco_srv.h"
#include "mrs_cco_queue.h"
#include "mrs_cco_power_failure.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
diag_cmd_pwf_result_query g_power_failure_mrs_dfx_info = { 0 };

td_bool mrs_cco_not_reply_ack(td_u8 rcv_mac[EXT_PLC_MAC_ADDR_LEN])
{
    td_u16 i;
    td_u16 check_tei = 0;
    mrs_power_failure_ctrl_st *power_failure_info = mrs_get_power_failure_ctrl();
    ext_mdm_nm_topo_node_info *topo_info = TD_NULL;

    if (power_failure_info->power_failure_bitmap == TD_NULL) {
        return TD_TRUE;
    }

    uapi_get_topo_info(&topo_info);

    for (i = 0; i < EXT_MDM_NM_TOPO_NODE_NUM_MAX; i++) {
        if (memcmp(topo_info[i].mac, rcv_mac, EXT_PLC_MAC_ADDR_LEN) == 0) {
            check_tei = i + 1;
            break;
        }
    }

    return mrs_power_failure_bit_check(power_failure_info->power_failure_bitmap, check_tei);
}

td_u32 mrs_cco_evt_report(td_void)
{
    mrs_power_failure_ctrl_st *power_failure_info = mrs_get_power_failure_ctrl();
    power_failure_info->is_ready_to_send_power_failure = TD_TRUE;

    return mrs_timer_start(MRS_TIMER_ID_CCO_POWER_FAILURE, power_failure_info->converge_time * MRS_US_TO_S_MASK,
        EXT_TIMER_TYPE_ONCE);
}

/* cco timeout processing for response event report timer */
td_void mrs_cco_rsp_power_failure_evt(td_void)
{
    mrs_power_failure_ctrl_st *power_failure_info = mrs_get_power_failure_ctrl();
    if (mrs_cco_create_rsp_power_frame() != EXT_ERR_SUCCESS) { }
    power_failure_info->is_ready_to_send_power_failure = TD_FALSE;

    if (power_failure_info->cco_diff_bitmap == TD_NULL) {
        return;
    }

    if (memset_s(power_failure_info->cco_diff_bitmap, POWER_FAILURE_BITMAP_LEN, 0,
                 POWER_FAILURE_BITMAP_LEN) != EXT_ERR_SUCCESS) {
        return;
    }

    if (power_failure_info->cco_erease_bitmap_flag == TD_FALSE) {
        /* erase data 4 minutes later(set time according to reality) */
        mrs_timer_start(MRS_TIMER_ID_CCO_ERASE_DATA,
                        power_failure_info->converge_time * MRS_US_TO_S_MASK * CCO_ERASE_BITMAP, EXT_TIMER_TYPE_ONCE);
        power_failure_info->cco_erease_bitmap_flag = TD_TRUE;
    }
}

td_u32 mrs_cco_report_rsp_power_info(td_u32 pk_num, td_u16 *start_tei)
{
    td_u32 i;
    td_u32 ret;
    td_u16 data_length = 0;
    mrs_cco_evt_item *item = TD_NULL;
    mrs_cco_evt_ctx *evt = mrs_cco_get_evt_ctx();
    td_u8 *data = TD_NULL;

    for (i = 0; i < pk_num; i++) {
        ret = mrs_cco_create_report_frame(&data, &data_length, start_tei);
        if (ret != EXT_ERR_SUCCESS) {
            break;
        }

        item = (mrs_cco_evt_item *)mrs_malloc(sizeof(mrs_cco_evt_item) + data_length);
        if (item == TD_NULL) {
            mrs_free(data);
            mrs_free(item);
            return EXT_ERR_NO_MORE_MEMORY;
        }

        if (memset_s(item, sizeof(mrs_cco_evt_item) + data_length, 0,
                     sizeof(mrs_cco_evt_item) + data_length) != EXT_ERR_SUCCESS) {
            mrs_free(data);
            mrs_free(item);
            return EXT_ERR_FAILURE;
        }

        item->data_len = data_length;
        item->fn = 9; /* fn 9 */
        if (memcpy_s(item->data, item->data_len, data, item->data_len) != EOK) {
            mrs_free(data);
            mrs_free(item);
            return EXT_ERR_MEMCPY_FAIL;
        }
        mrs_queue_enqueue(&evt->evt_q, &item->link);
        evt->evt_q_num++;
        mrs_free(data);

        evt->acd_flg = TD_TRUE;
        /* function of reporting by self.(fn = 9, power down report) */
        mrs_cco_evt_report_active();
    }
    return EXT_ERR_SUCCESS;
}

td_u32 mrs_cco_create_rsp_power_frame(td_void)
{
    td_u32 ret;
    mrs_power_failure_ctrl_st *power_failure_info = mrs_get_power_failure_ctrl();
    td_u16 bitmap_max_tei;
    td_u16 bitmap_min_tei;
    td_u32 bitmap_tei_cnt;
    td_u32 pk_num;
    td_u16 start_tei;

    /* get bitmap to be sent */
    mrs_cco_report_bitmap();
    if (power_failure_info->cco_diff_bitmap == TD_NULL) {
        return EXT_ERR_FAILURE;
    }

    bitmap_min_tei = mrs_calc_min_tei(power_failure_info->cco_diff_bitmap);
    bitmap_max_tei = mrs_calc_max_tei(power_failure_info->cco_diff_bitmap);

    /* calc quantity of power down modules */
    bitmap_tei_cnt = mrs_cco_get_bitmap_tei_cnt(bitmap_min_tei, bitmap_max_tei, power_failure_info->cco_diff_bitmap);
    start_tei = bitmap_min_tei;

    pk_num = (bitmap_tei_cnt + MAX_REPORT_TEI_CNT - 1) / MAX_REPORT_TEI_CNT;

    /* create report frame */
    ret = mrs_cco_report_rsp_power_info(pk_num, &start_tei);
    if (ret != EXT_ERR_SUCCESS) {
        return ret;
    }

    /* maintainable info */
    mrs_cco_power_failure_dfx_info();

    return ret;
}

/* create report frame (376.2 treaty:data_len= 1 + 6xN bytes / supplementary content copy to buffer */
td_u32 mrs_cco_create_report_frame(td_pbyte *buf, td_u16 *data_len, td_u16 *start_tei_pos)
{
    td_u32 ret = EXT_ERR_SUCCESS;
    mrs_proto_1376_2_encode encode;
    mrs_cco_srv_ctx *cco = mrs_cco_get_srv_ctx();
    mrs_power_failure_ctrl_st *power_failure_info = mrs_get_power_failure_ctrl();
    td_u16 bitmap_max_tei;
    td_u32 bitmap_tei_cnt;
    td_u16 report_data_len;
    td_byte *pf_data = TD_NULL;
    td_u8 report_tei_cnt;
    td_u16 next_tei_pos = 0;
    td_u8 *bitmap = power_failure_info->cco_diff_bitmap;

    bitmap_max_tei = mrs_calc_max_tei(bitmap);

    bitmap_tei_cnt = mrs_cco_get_bitmap_tei_cnt(*start_tei_pos, bitmap_max_tei, bitmap);
    if (bitmap_tei_cnt > MAX_REPORT_TEI_CNT) {
        report_data_len = MAX_REPORT_TEI_CNT * EXT_PLC_MAC_ADDR_LEN;
        report_tei_cnt = MAX_REPORT_TEI_CNT;
    } else {
        report_data_len = (td_u16)(bitmap_tei_cnt * EXT_PLC_MAC_ADDR_LEN);
        report_tei_cnt = (td_u8)bitmap_tei_cnt;
    }

    do {
        /* fill data unit */
        pf_data = mrs_malloc(report_data_len + 1);
        if (pf_data == TD_NULL) {
            return EXT_ERR_MALLOC_FAILUE;
        }

        pf_data[0] = report_tei_cnt; /* report quantity of slave nodes */
        /* fill address of power down module */
        mrs_cco_report_mac_addr(bitmap, *start_tei_pos, report_data_len, pf_data + 1, &next_tei_pos);

        *start_tei_pos = next_tei_pos;

        /* fill 3762 frame stucture */
        if (memset_s(&encode, sizeof(mrs_proto_1376_2_encode), 0, sizeof(mrs_proto_1376_2_encode)) != EXT_ERR_SUCCESS) {
            mrs_free(pf_data);
            return EXT_ERR_FAILURE;
        }

        encode.data = pf_data;
        encode.length = report_data_len + 1;
        encode.prm = 1;
        encode.seq = ++cco->seq;
        encode.afn = 6; /* afn 6 */
        encode.fn = 9;  /* fn 9 */
        if (memcpy_s(encode.dst_addr, sizeof(encode.dst_addr), cco->main_node,
            EXT_PLC_MAC_ADDR_LEN) != EXT_ERR_SUCCESS) {
            break;
        }

        mrs_hex_invert(encode.dst_addr, EXT_PLC_MAC_ADDR_LEN);

        ret = mrs_proto_1376_2_create_frame(&encode, buf, data_len);
        mrs_free(encode.data);
    } while (0);

    mrs_free(pf_data);

    return ret;
}

/* get MAC address of power down tei */
td_void mrs_cco_report_mac_addr(td_u8 *diff_bitmap, td_u16 start_pos, td_u16 report_len, td_u8 *mac_addr,
                                td_u16 *next_pos)
{
    td_u16 i;
    td_u16 j;
    td_u16 bitmap_max_tei;
    td_u16 query_index = 0;
    ext_mdm_nm_topo_node_info *topo_info = TD_NULL;
    td_u8 mac_cnt = 0;
    /* get max tei */
    bitmap_max_tei = mrs_calc_max_tei(diff_bitmap);
    uapi_get_topo_info(&topo_info);

    for (i = start_pos; i <= bitmap_max_tei && mac_cnt < report_len; i++) {
        /* station power down test */
        if (mrs_power_failure_bit_check(diff_bitmap, i) == TD_FALSE) {
            continue;
        }

        /* fill mac address to power-down data unit searched from power-down stations in topo */
        for (j = query_index; j < EXT_MDM_NM_TOPO_NODE_NUM_MAX; j++) {
            if (j + 1 != i) {
                continue;
            }

            if (memcpy_s(mac_addr + mac_cnt, EXT_PLC_MAC_ADDR_LEN, topo_info[j].mac, EXT_PLC_MAC_ADDR_LEN) != EOK) {
                continue;
            }

            query_index = j + 1;
            mrs_hex_invert(mac_addr + mac_cnt, EXT_PLC_MAC_ADDR_LEN);
            mac_cnt += EXT_PLC_MAC_ADDR_LEN;
            *next_pos = i;
            break;
        }
    }

    /* update start index for next reporting */
    *next_pos += 1;
}

/* get bitmap which not sent before */
td_void mrs_cco_report_bitmap(td_void)
{
    td_u16 i;
    mrs_power_failure_ctrl_st *power_failure_info = mrs_get_power_failure_ctrl();
    td_u8 *bitmap_diff = power_failure_info->cco_diff_bitmap;
    td_u8 *bitmap_cur = power_failure_info->power_failure_bitmap;
    td_u8 *bitmap_last_send = power_failure_info->cco_last_send_bitmap;

    if (bitmap_diff == TD_NULL) {
        return;
    }

    if (bitmap_cur == TD_NULL) {
        return;
    }

    if (bitmap_last_send == TD_NULL) {
        return;
    }

    for (i = 0; i < POWER_FAILURE_BITMAP_LEN * BIT_SIZE; i++) {
        /* update the difference if the bit of the station is power down present and power on last time */
        if (mrs_power_failure_bit_check(bitmap_cur, i) == TD_TRUE &&
            mrs_power_failure_bit_check(bitmap_last_send, i) == TD_FALSE) {
            mrs_power_failure_bit_set(bitmap_diff, i);
        }
    }

    if (memcpy_s(bitmap_last_send, POWER_FAILURE_BITMAP_LEN, bitmap_cur, POWER_FAILURE_BITMAP_LEN) != EXT_ERR_SUCCESS) {
        return;
    }
}

td_u32 mrs_cco_get_bitmap_tei_cnt(td_u16 bitmap_min_tei, td_u16 bitmap_max_tei, td_u8 *bitmap)
{
    td_u32 count = 0;
    td_u16 i;
    td_u16 j;
    td_u16 query_index = 0;
    ext_mdm_nm_topo_node_info *topo_info = TD_NULL;

    uapi_get_topo_info(&topo_info);

    if (topo_info == TD_NULL) {
        return count;
    }

    for (i = bitmap_min_tei; i <= bitmap_max_tei; i++) {
        /* station power down test */
        if (mrs_power_failure_bit_check(bitmap, i) == TD_FALSE) {
            continue;
        }

        for (j = query_index; j < EXT_MDM_NM_TOPO_NODE_NUM_MAX; j++) {
            if (topo_info[j].state == STATE_NOT_USED) {
                continue;
            }
            /* verify if station in topo and validity of address */
            if ((j + 1 == i) && (!mrs_invalid_meter(topo_info[j].mac))) {
                count++;
                query_index = j + 1;
                break;
            }
        }
    }

    return count;
}

/* CCO erase data */
td_void mrs_cco_erase_bitmap(td_void)
{
    mrs_power_failure_ctrl_st *power_failure_info = mrs_get_power_failure_ctrl();
    td_u8 *cco_temp_diff_bitmap = TD_NULL;

    cco_temp_diff_bitmap = (td_u8 *)mrs_malloc(POWER_FAILURE_BITMAP_LEN);
    if (cco_temp_diff_bitmap == TD_NULL) {
        return;
    }

    /* get bitmap to be sent */
    mrs_cco_report_bitmap();
    do {
        if (power_failure_info->cco_diff_bitmap != TD_NULL) {
            if (memcpy_s(cco_temp_diff_bitmap, POWER_FAILURE_BITMAP_LEN, power_failure_info->cco_diff_bitmap,
                         POWER_FAILURE_BITMAP_LEN) != EXT_ERR_SUCCESS) {
                break;
            }
            if (memset_s(power_failure_info->cco_diff_bitmap, POWER_FAILURE_BITMAP_LEN, 0,
                         POWER_FAILURE_BITMAP_LEN) != EXT_ERR_SUCCESS) {
                break;
            }
        }

        if (mrs_cco_erase_common_bitmap(power_failure_info) != EXT_ERR_SUCCESS) {
            break;
        }

        if (power_failure_info->cco_diff_bitmap != TD_NULL) {
            if (memcpy_s(power_failure_info->cco_diff_bitmap, POWER_FAILURE_BITMAP_LEN, cco_temp_diff_bitmap,
                         POWER_FAILURE_BITMAP_LEN) != EXT_ERR_SUCCESS) {
                break;
            }
        }

        if (power_failure_info->power_failure_bitmap != TD_NULL) {
            if (memcpy_s(power_failure_info->power_failure_bitmap, POWER_FAILURE_BITMAP_LEN, cco_temp_diff_bitmap,
                         POWER_FAILURE_BITMAP_LEN) != EXT_ERR_SUCCESS) {
                break;
            }
        }
    } while (0);

    mrs_free(cco_temp_diff_bitmap);
}

td_u32 mrs_cco_erase_common_bitmap(mrs_power_failure_ctrl_st *power_failure_info)
{
    if (power_failure_info->power_failure_bitmap != TD_NULL &&
        memset_s(power_failure_info->power_failure_bitmap, POWER_FAILURE_BITMAP_LEN, 0,
                 POWER_FAILURE_BITMAP_LEN) != EXT_ERR_SUCCESS) {
        return EXT_ERR_FAILURE;
    }

    if (power_failure_info->cco_last_send_bitmap != TD_NULL &&
        memset_s(power_failure_info->cco_last_send_bitmap, POWER_FAILURE_BITMAP_LEN, 0,
                 POWER_FAILURE_BITMAP_LEN) != EXT_ERR_SUCCESS) {
        return EXT_ERR_FAILURE;
    }

    if (memset_s(&g_power_failure_mrs_dfx_info, sizeof(diag_cmd_pwf_result_query), 0,
                 sizeof(diag_cmd_pwf_result_query)) != EXT_ERR_SUCCESS) {
        return EXT_ERR_FAILURE;
    }

    power_failure_info->cco_erease_bitmap_flag = TD_FALSE;

    return EXT_ERR_SUCCESS;
}

/* function for get maintainable info */
td_void mrs_cco_power_failure_dfx_info(td_void)
{
    td_u16 i;
    mrs_power_failure_ctrl_st *power_failure_info = mrs_get_power_failure_ctrl();
    diag_cmd_pwf_result_query *ind = &g_power_failure_mrs_dfx_info;

    for (i = 0; i < POWER_FAILURE_BITMAP_LEN * BIT_SIZE; i++) {
        if (mrs_power_failure_bit_check(power_failure_info->cco_diff_bitmap, i) == TD_TRUE) {
            ind->pwf_tei_cnt++;
        }
    }

    ind->rcv_pwf_frame_time = uapi_get_seconds();
}

td_u32 mrs_cmd_pwf(td_u16 id, td_pvoid cmd, td_u16 cmd_size, td_u8 option)
{
    diag_cmd_pwf_result_query *ind = &g_power_failure_mrs_dfx_info;

    ext_unref_param(cmd_size);
    ext_unref_param(cmd);

    return uapi_diag_report_packet(id, option, (td_pbyte)ind, sizeof(diag_cmd_pwf_result_query), TD_FALSE);
}
#endif

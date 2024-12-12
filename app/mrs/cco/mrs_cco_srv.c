/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS cco service.
 */

#include <app_nv.h>
#include "mrs_config.h"
#include "mrs_msg.h"
#include "mrs_common_tools.h"
#include "mrs_cco_io.h"
#include "mrs_cco_archives.h"
#include "mrs_cco_1376_2.h"
#include "mrs_cco_evt.h"
#include "mrs_cco_queue.h"
#include "mrs_cco_upg.h"
#include "mrs_cco_broadcast.h"
#include "mrs_cco_tf.h"
#include "mrs_cco_power_failure.h"
#include "mrs_common_capture.h"
#include "mrs_cco_srv.h"


#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)

static mrs_cco_srv_ctx *g_cco_srv_ctx = TD_NULL;

td_void mrs_cco_init_mac_addr(td_void);

mrs_msg_timer_func_map g_cco_msg_timer_func_map[] = {
    { MRS_TIMER_ID_UART,                    mrs_proto_1376_2_on_timer },
    { MRS_TIMER_ID_MR_QUEUE_CHECK,          mrs_mr_queue_check_on_timer },
    { MRS_TIMER_ID_PARALLEL_CHECK,          mrs_parallel_check_on_timer },
    { MRS_TIMER_ID_METER_ARCHIVES,          mrs_archives_on_timer },
    { MRS_TIMER_ID_CCO_TO_HOST,             mrs_cco_queue_on_timer },
    { MRS_TIMER_ID_CCO_INIT,                mrs_cco_report_afn_03f10 },
    { MRS_TIMER_ID_LED_PHASE_A,             mrs_plc_tx_phase_a_off },
    { MRS_TIMER_ID_LED_PHASE_B,             mrs_plc_tx_phase_b_off },
    { MRS_TIMER_ID_LED_PHASE_C,             mrs_plc_tx_phase_c_off },
    { MRS_TIMER_ID_BROADCAST_RESEND,        mrs_cco_broadcast_send_timeout },
    { MRS_TIMER_ID_TRANS_FILE_RX,           mrs_cco_upg_file_rx_on_timer },
    { MRS_TIMER_ID_UPG_STOP,                mrs_cco_upg_stop_on_timer },
    { MRS_TIMER_ID_UPG_BASE_STATUS_POLLING, mrs_cco_upg_stat_query_bs_on_timer },
    { MRS_TIMER_ID_UPG_NODE_STATUS_TIMEOUT, mrs_cco_upg_stat_timeout },
    { MRS_TIMER_ID_UPG_GET_SW_VER,          mrs_cco_upg_get_sw_ver_timeout },
    { MRS_TIMER_ID_UPG_WAIT_RESP_DONE,      mrs_cco_upg_wait_resp_done_timeout },
    { MRS_TIMER_ID_PLC_IS_AVAILABLE,        mrs_cco_upg_plc_wait_timeout },
    { MRS_TIMER_ID_TF_SEND_START_CMD,       mrs_cco_send_tf_start_cmd },
    { MRS_TIMER_ID_TF_SEND_STOP_CMD,        mrs_cco_send_tf_stop_cmd },
    { MRS_TIMER_ID_TF_REFRESH_STA_LIST,     mrs_cco_refresh_sta_tf_record },
    { MRS_TIMER_ID_TF_IDENTIFY_PROC,        mrs_cco_query_tf_identify },
    { MRS_TIMER_ID_TF_REPORT_PROC,          mrs_cco_stop_tf_identify },
    { MRS_TIMER_ID_TF_QUERY_INTERVAL,       mrs_cco_query_tf_proc },
    { MRS_TIMER_ID_TF_COMPLETE,             mrs_cco_tf_finished },
    { MRS_TIMER_ID_TF_QUERY_TIMEOUT,        mrs_cco_tf_query_timeout },
    { MRS_TIMER_ID_CCO_POWER_FAILURE,       mrs_cco_rsp_power_failure_evt },
    { MRS_TIMER_ID_CCO_ERASE_DATA,          mrs_cco_erase_bitmap },
    { MRS_TIMER_ID_CCO_ENTER_TESTMODE,      mrs_enter_test_mode },
    { MRS_TIMER_ID_CCO_PHY_TRANS_TESTMODE,  mrs_exit_phy_tans_test_mode },
    { MRS_TIMER_ID_CCO_PHY_LOOP_TESTMODE,   mrs_exit_phy_loop_test_mode },
    { MRS_TIMER_ID_CCO_MAC_MSDU_TESTMODE,   mrs_exit_mac_msdu_test_mode },
};

mrs_msg_timer_func mrs_find_msg_on_timer_func(uintptr_t id)
{
    td_u32 count = ext_array_count(g_cco_msg_timer_func_map);
    td_u32 i = 0;

    if (id > MRS_TIMER_ID_MAX) {
        return TD_NULL;
    }

    for (; i < count; i++) {
        if (id == g_cco_msg_timer_func_map[i].id) {
            return g_cco_msg_timer_func_map[i].func;
        }
    }
    return TD_NULL;
}

td_void mrs_msg_on_timer(EXT_CONST mrs_queue_msg *msg)
{
    mrs_msg_timer_func func = mrs_find_msg_on_timer_func(msg->param1);
    if (func != TD_NULL) {
        func();
        return;
    }

    switch (msg->param1) {
        case MRS_TIMER_ID_CCO_REBOOT:
            uapi_usr_reboot(EXT_SYS_REBOOT_CAUSE_USR1);
            break;

        case MRS_TIMER_ID_RST_IO:
            uapi_usr_reboot(EXT_SYS_REBOOT_CAUSE_USR0);
            break;

        case MRS_TIMER_ID_PLC_LED_RX:
            mrs_plc_rx_led_off();
            break;

        case MRS_TIMER_ID_PLC_LED_TX:
            mrs_plc_tx_led_off();
            break;

        default:
            break;
    }
}

mrs_cco_srv_ctx *mrs_cco_get_srv_ctx(td_void)
{
    return g_cco_srv_ctx;
}

td_void mrs_cco_init_mac_addr(td_void)
{
    mrs_cco_srv_ctx *cco = mrs_cco_get_srv_ctx();
    app_dev_addr_inf nv = { { 0 }, { 0 } };
    td_u32 ret;
    td_u8 mac[EXT_METER_ADDR_LEN] = { 0 };

    ret = uapi_nv_read(ID_NV_APP_DEV_ADDR_INF, &nv, sizeof(nv));
    mrs_hex_invert(nv.dev_addr, (td_u16)sizeof(nv.dev_addr));
    mrs_convert_meter_to_mac(nv.dev_addr, sizeof(nv.dev_addr), mac, sizeof(mac));
    if (mrs_is_invalid_mac_addr(mac)) {
        uapi_get_local_mac_addr(mac, sizeof(mac));
    }

    uapi_set_cco_mac_addr(mac);

    if (ret == EXT_ERR_SUCCESS) {
        errno_t err = memcpy_s(cco->main_node, sizeof(cco->main_node), nv.dev_addr, sizeof(nv.dev_addr));
        if (err != EOK) {
            return;
        }
    }
}

td_u32 mrs_cco_srv_init(td_void)
{
    if (g_cco_srv_ctx != TD_NULL) {
        return EXT_ERR_INITILIZATION_ALREADY;
    }

    g_cco_srv_ctx = (mrs_cco_srv_ctx *)mrs_malloc(sizeof(mrs_cco_srv_ctx));
    if (g_cco_srv_ctx == TD_NULL) {
        return EXT_ERR_MALLOC_FAILUE;
    }

    mrs_timer_start(MRS_TIMER_ID_CCO_INIT, MRS_CCO_INIT_TIME, EXT_TIMER_TYPE_ONCE);

    (td_void) memset_s(g_cco_srv_ctx, sizeof(mrs_cco_srv_ctx), 0, sizeof(mrs_cco_srv_ctx));
    g_cco_srv_ctx->plc_seq = (td_u16)uapi_get_random_num32(1, 65535); /* random number: min 1,max 65535 */
    mrs_cco_mr_list_init(&g_cco_srv_ctx->mr_list_head);
    mrs_cco_mr_pool_init(&g_cco_srv_ctx->mr_pool);
    mrs_cco_event_init();
    mrs_cco_queue_init();
    mrs_proto_1376_2_init();
    mrs_cco_setup_whitelist();
    mrs_io_init();
    mrs_upg_init();

    mrs_cco_init_mac_addr();

    return EXT_ERR_SUCCESS;
}

td_void mrs_cco_simu_ctrl(td_bool enable)
{
    mrs_cco_srv_ctx *cco = mrs_cco_get_srv_ctx();
    if (cco != TD_NULL) {
        cco->simu_ch_status = enable;
    }
}

td_bool mrs_cco_simu_status(td_void)
{
    mrs_cco_srv_ctx *cco = mrs_cco_get_srv_ctx();
    if (cco != TD_NULL) {
        return cco->simu_ch_status;
    }

    return TD_FALSE;
}

td_void mrs_cco_setup_whitelist(td_void)
{
    ext_mac_valid_mac_list *mac_list = TD_NULL;
    ext_mac_entry *entry = TD_NULL;
    meter_archives *meter_inf = mrs_archives_get();
    meter_item *item = TD_NULL;
    td_u16 i, max_tei;
    td_u32 data_size;

    if (meter_inf == TD_NULL) {
        return;
    }

    max_tei = uapi_topo_get_max_tei();
    data_size = uapi_min(meter_inf->head.num, max_tei);
    data_size = data_size * sizeof(ext_mac_entry) + sizeof(ext_mac_valid_mac_list);

    mac_list = (ext_mac_valid_mac_list *)mrs_malloc(data_size);
    if (mac_list == TD_NULL) {
        return;
    }

    (td_void) memset_s(mac_list, data_size, 0, data_size);
    mac_list->list_num = uapi_min(meter_inf->head.num, max_tei);

    entry = mac_list->mac_list;
    item = meter_inf->items;
    for (i = 0; i < mac_list->list_num; i++, entry++, item++) {
        mrs_convert_meter_to_mac(item->addr, sizeof(item->addr), entry->mac, sizeof(entry->mac));
    }

    uapi_set_valid_mac_list(mac_list, EXT_MAC_WHITE_LIST_CHANGE_REASON_APP_INIT);
    mrs_free(mac_list);
}

#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_CCO) */


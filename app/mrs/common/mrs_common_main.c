/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS main entry.
 */

#include "mrs_common_main.h"
#include "mrs_proto.h"
#include "mrs_msg.h"
#include "mrs_common_power_manage.h"
#include "mrs_common_tools.h"
#include "mrs_common_diag.h"
#include "mrs_cco_srv.h"
#include "mrs_cco_io.h"
#include "mrs_sta_srv.h"
#include "mrs_ndm_srv.h"
#include "mrs_dfx.h"
#include "soc_mdm_adc_control.h"
#include "soc_mdm_adc.h"
#include "mrs_common_power_failure.h"
#include "mrs_sta_power_failure.h"
#include "soc_mdm_pi.h"
#include "soc_mdm_tl.h"
#include "mrs_common_capture.h"
#include "dfx_app.h"
#include "soc_mdm_tf.h"

static mrs_main_srv_context *g_mrs_main_srv = TD_NULL;

td_void mrs_main_init(td_void);
td_u32 mrs_main_context_init(td_void);
td_u32 mrs_main_res_init(EXT_CONST mrs_main_srv_context *ctx);
td_void mrs_main_task_body(uintptr_t param);
td_void mrs_msg_handle(EXT_CONST mrs_queue_msg *msg);
td_void mrs_main_init_mac_addr(td_void);
td_u32 app_query_mrs_version_info(EXT_OUT td_pbyte ver_info, EXT_IN EXT_CONST td_u16 ver_info_size);

__hot mrs_main_srv_context *mrs_get_main_srv_ctx(td_void)
{
    return g_mrs_main_srv;
}

#if defined(PRODUCT_CFG_DEBUG_CRASH_MRS)
__isr td_void led_flash_when_exc(td_void)
{
    uapi_watchdog_disable();
#ifdef PRODUCT_CFG_PRODUCT_TYPE_STA
    uapi_io_set_func(EXT_GPIO_IDX_6, 0);
    uapi_io_set_dir(EXT_GPIO_IDX_6, EXT_GPIO_DIRECTION_OUT);
    uapi_io_set_ouput_val(EXT_GPIO_IDX_6, EXT_GPIO_VALUE0);
    while (1) {
        for (uintptr_t i = 0x02000000; i < 0x02038000; i += 16) { /* shifts 16bits */
            if ((i & 0x3ff) == 0 && (i & 0x7ff) != 0) {
                uapi_io_set_ouput_val(EXT_GPIO_IDX_6, EXT_GPIO_VALUE1);
                uapi_led_on(EXT_LED_IDX_4);
                uapi_led_on(EXT_LED_IDX_1);
                uapi_led_on(EXT_LED_IDX_0);
            } else if ((i & 0x3ff) == 0) {
                uapi_led_off(EXT_LED_IDX_4);
                uapi_led_off(EXT_LED_IDX_1);
                uapi_led_off(EXT_LED_IDX_0);
                uapi_io_set_ouput_val(EXT_GPIO_IDX_6, EXT_GPIO_VALUE0);
            }
            printf(" 0x%08x: 0x%08x 0x%08x 0x%08x 0x%08x \n",
                   i, *(td_u32 *)i, *(td_u32 *)(i + 4), *(td_u32 *)(i + 8), *(td_u32 *)(i + 12)); /* shifts 4/8/12b */
        }
    }
#else
    while (1) {
        for (uintptr_t i = 0x02000000; i < 0x02038000; i += 16) { /* shifts 16bits */
            if ((i & 0x3ff) == 0 && (i & 0x7ff) != 0) {
                uapi_led_on(EXT_LED_IDX_4);
                uapi_led_on(EXT_LED_IDX_1);
                uapi_led_on(EXT_LED_IDX_0);
            } else if ((i & 0x3ff) == 0) {
                uapi_led_off(EXT_LED_IDX_4);
                uapi_led_off(EXT_LED_IDX_1);
                uapi_led_off(EXT_LED_IDX_0);
            }
            printf(" 0x%08x: 0x%08x 0x%08x 0x%08x 0x%08x \n",
                   i, *(td_u32 *)i, *(td_u32 *)(i + 4), *(td_u32 *)(i + 8), *(td_u32 *)(i + 12)); /* shifts 4/8/12b */
        }
    }
#endif
}

#endif

td_void mrs_adc_ctrl_pi_init(td_void)
{
#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO) || defined(PRODUCT_CFG_PRODUCT_TYPE_STA)
    ext_adc_channel_index_en channel[AD_COLLECT_ENTRY_NUM];
#endif

    /* set data aquisition channel and data aquisition self checking parameter */
#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
    channel[0] = EN_CHANNEL_6;
    uapi_adc_set_channel(channel, 1);
#endif
#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)
    channel[0] = EN_CHANNEL_6;
    uapi_adc_set_channel(channel, 1);
#endif
    uapi_adc_set_self_check_para(AD_SELF_CHECK_COLLECT_CYCLE, AD_SELF_CHECK_COLLECT_DURATION, AD_POWER_FAILURE_DIFF_TH);
    /* initialization for data aquisition control module */
    uapi_adc_control_init();
    uapi_sar_adc_noiseshaping_cfg(EN_NOISE_SHAPING_OFF);

#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA) || defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
    /* loading phase identification module */
    uapi_load_pi();
#endif
}

td_void app_main(td_void)
{
#if defined(PRODUCT_CFG_DEBUG_CRASH_MRS)
    uapi_syserr_exc_register((ext_syserr_exc_callback)led_flash_when_exc);
#endif
    (td_void) uapi_board_init();
    (td_void) uapi_io_int_init();
    (td_void) uapi_aes_init();
    uapi_phy_init();
#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
    mrs_cco_phase_init();
#endif
    uapi_mac_cp_init();
    uapi_load_sg_and_1901_protocol();
    /* loading transport-layer */
    uapi_load_tl(APP_TL_QUEUE_SIZE);

#if !defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
    uapi_load_p2p_component_p1901();
#endif
#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA) || defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
    uapi_load_config_belong_network_component();
#endif
    mrs_adc_ctrl_pi_init();

#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)
    /* loading transformer identify module */
    uapi_load_tf_snr_component();
#if defined(PRODUCT_CFG_SUPPORT_UPG_FROM_DBK)
    uapi_upg_sta_cmd_init();
#endif
#endif

    (td_void) uapi_mac_init();
    uapi_phase_evaluate_enable();

    mrs_common_power_manage_init();
    (td_void) uapi_diag_init();
    (td_void) uapi_mid_init();
    /* set partition info for list upgrading. By default, using user data partition: [0x7B000,0x7D000). */
#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
    ext_flash_partition_table *partition = uapi_get_partition_table();
    if (partition->table[EXT_FLASH_PARTITON_USER_INFO_1].size_blk >= 2) { /* more than 2blocks */
        uapi_upg_list_set_cache_info(partition->table[EXT_FLASH_PARTITON_USER_INFO_1].addr_blk,
            2); /* Cache size: 2 blks */
    }
#endif
    uapi_upg_init();
    mrs_dfx_init();
    mrs_main_init();

    (td_void) uapi_diag_register_query_app_ver_cmd_callback((ext_mdm_query_app_ver)app_query_mrs_version_info);
}

td_void mrs_main_init(td_void)
{
    td_u32 ret;
    mrs_main_srv_context *ctx = TD_NULL;

    ret = mrs_main_context_init();
    if (ret != EXT_ERR_SUCCESS) {
        return;
    }

    ctx = mrs_get_main_srv_ctx();

    ret = mrs_uart_init();
    if (ret != EXT_ERR_SUCCESS) {
        return;
    }

    ret = mrs_proto_init();
    if (ret != EXT_ERR_SUCCESS) {
        return;
    }

    ret = mrs_main_res_init(ctx);
    if (ret != EXT_ERR_SUCCESS) {
        return;
    }

    ret = mrs_plc_chl_init();
    if (ret != EXT_ERR_SUCCESS) {
        return;
    }

    mrs_main_init_mac_addr();
    /* power down detection feature(download settings) */
    mrs_common_power_failure_init();
#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)
    mrs_pwf_init();
#endif
#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA) || defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
    mrs_common_capture_init();
    uapi_register_power_failure_rejoin_event_report(mrs_handle_power_failure_rejoin_event);
#endif
}

td_u32 mrs_main_context_init(td_void)
{
    if (g_mrs_main_srv != TD_NULL) {
        return EXT_ERR_INITILIZATION_ALREADY;
    }

    g_mrs_main_srv = (mrs_main_srv_context *)mrs_malloc(sizeof(mrs_main_srv_context));
    if (g_mrs_main_srv == TD_NULL) {
        return EXT_ERR_MALLOC_FAILUE;
    }

    (td_void) memset_s(g_mrs_main_srv, sizeof(mrs_main_srv_context), 0, sizeof(mrs_main_srv_context));

    return EXT_ERR_SUCCESS;
}

td_u32 mrs_main_res_init(EXT_CONST mrs_main_srv_context *ctx)
{
    td_u32 ret;
    td_u32 task_id = EXT_ERR_INVALID_ID;
    td_s8 i;

    ret = uapi_msg_queue_create((td_u32 *)&ctx->queue_id, "mrsq", MRS_MAIN_QUEUE_SIZE, sizeof(mrs_queue_msg));
    if (ret != EXT_ERR_SUCCESS) {
        return ret;
    }

    for (i = 0; i < MRS_SEM_ID_MAX; i++) {
        ret = uapi_sem_bcreate((td_u32 *)&ctx->sem_id[i], "sem_mrs", 1);
        if (ret != EXT_ERR_SUCCESS) {
            return ret;
        }
    }

    ret = uapi_task_create(&task_id, "srv_mrs", mrs_main_task_body, ctx->queue_id,
                         MRS_MAIN_TASK_STACK_SIZE, MRS_MAIN_TASK_PRIORITY);
    if (ret != EXT_ERR_SUCCESS) {
        return ret;
    }

    ret = uapi_task_create(&task_id, "uart_m", mrs_uart_task_body, 0, MRS_UART_TASK_STACK_SIZE,
                         MRS_UART_TASK_PRIORITY);
    if (ret != EXT_ERR_SUCCESS) {
        return ret;
    }

    return ret;
}

td_void mrs_main_task_body(uintptr_t param)
{
    td_u32 ret;
    mrs_queue_msg msg = { 0 };
    td_u32 queue_id = (td_u32)param;

    mrs_srv_init();
    mrs_diag_init();

    for (;;) {
        ret = uapi_msg_queue_wait(queue_id, &msg, EXT_SYS_WAIT_FOREVER, sizeof(mrs_queue_msg));
        if (ret == EXT_ERR_SUCCESS) {
            mrs_msg_handle(&msg);
            continue;
        }

        uapi_sleep(1000); /* sleep time: 1000ms */
    }
}

td_void mrs_msg_handle(EXT_CONST mrs_queue_msg *msg)
{
    mrs_msg_handler_fct handler[] = MRS_MSG_HANDLER_MAP;

    if (msg->id >= MRS_MSG_ID_MAX) {
        return;
    }

    if (handler[msg->id] != TD_NULL) {
        handler[msg->id](msg);
    }
}

td_void mrs_main_init_mac_addr(td_void)
{
    td_u32 ret;
    errno_t err;
    td_u8 mac[EXT_PLC_MAC_ADDR_LEN] = { 0 };
    td_u32 result;

    ret = uapi_get_local_mac_addr(mac, sizeof(mac));
    if (ret != EXT_ERR_SUCCESS) {
        return;
    }

    if (mrs_is_invalid_mac_addr(mac)) {
        td_u32 trng_result[2] = { 0 }; /* note: 2*4 = 8bytes */
        ext_nv_ftm_product_id nv = { EXT_FTM_PRODUCT_TYPE_UNKOWN, { 0 }, { 0 } };

        ret = uapi_factory_nv_read(EXT_NV_FTM_PRODUCT_ID, &nv, sizeof(nv));
        if (ret != EXT_ERR_SUCCESS) {
            return;
        }

        result = 0;
        (td_void) uapi_trng_getrandom(&result);
        trng_result[0] = result;
        (td_void) uapi_trng_getrandom(&result);
        trng_result[1] = result;

        err = memcpy_s(nv.plc_mac, sizeof(nv.plc_mac), trng_result, EXT_PLC_MAC_ADDR_LEN);
        if (err != EOK) {
            return;
        }

        ret = uapi_factory_nv_write(EXT_NV_FTM_PRODUCT_ID, &nv, sizeof(nv));
        if (ret != EXT_ERR_SUCCESS) {
            return;
        }
    }
}

td_u32 app_query_mrs_version_info(EXT_OUT td_pbyte ver_info, EXT_IN EXT_CONST td_u16 ver_info_size)
{
    td_char app_name[] = {"MRS"};
    ext_app_ver_cmd_ind_info *cur_ver_info = TD_NULL;

    if (ver_info == TD_NULL || ver_info_size != sizeof(ext_app_ver_cmd_ind_info)) {
        return EXT_ERR_FAILURE;
    }

    cur_ver_info = (ext_app_ver_cmd_ind_info *)ver_info;
    if (strcpy_s((td_char *)cur_ver_info->app_name, sizeof(app_name), app_name) != EOK) {
        return EXT_ERR_FAILURE;
    }

    return EXT_ERR_SUCCESS;
}

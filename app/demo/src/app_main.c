/*
 * Copyright (c) CompanyNameMagicTag 2018-2020. All rights reserved.
 * Description: APP main entry.
 */

#include <dw21_platform.h>
#include <soc_mdm_clock.h>
#include <soc_mdm_timecnt.h>
#include <soc_mid.h>
#include <soc_mdm_mac.h>
#include <soc_mdm_phy.h>
#include <soc_mdm_tl.h>
#include "app_common.h"
#include "app_uart.h"
#include "app_cmd_msg.h"
#include "app_cco_whitelist.h"
#include "dfx_app.h"
#include "app_sta_dut.h"
#include "app_diag_cmd.h"
#include "app_power_manager.h"
#include "soc_mdm_star_net.h"
#include "soc_mdm_upg.h"
#include "app_main.h"
#if defined(PRODUCT_CFG_PLC_DEMO_TEST)
#include "app_plc_test.h"
#endif

app_srv_context *g_app_srv_context = TD_NULL;

td_u32 app_ctx_init(td_void);
td_void app_uart_buf_init(app_data_buf *buf);
td_u32 app_res_init(EXT_CONST app_res_cfg *res);
static td_void app_srv_task_body(td_u32 param);
static td_void app_main_msg_proc(EXT_CONST ext_sys_queue_msg *msg);
static td_void app_uart_rx_timeout(td_void);
static td_void app_send_plc_frame(EXT_CONST td_u8 *data, td_u16 data_len);
td_void app_send_uart_frame(td_u8 *data, td_u16 data_len);
static td_u32 app_plc_chl_rx(td_u16 port, EXT_CONST ext_dms_chl_rx *chl_rx);
static td_void app_on_timer(td_u32 timer_id);
td_void app_on_init_alarm(td_void);
static td_void app_uart_frame_proc(EXT_CONST td_pbyte frame, td_u16 length, td_u16 max_frame_len);
static td_u32 app_query_demo_version_info(EXT_OUT td_pbyte ver_info, EXT_IN EXT_CONST td_u16 ver_info_size);
/* convenient for testing, and customer can delete the relevant
references of the NV according to the actual situation */
#define NV_ID_TEST 0x75 /* test parameters: including UART configuration */

typedef struct {
    td_u8 uart_cfg : 1; /* 0:app uart0¡¢diag:uart1; 1:app uart1¡¢diag:uart0 */
    td_u8 pad1 : 7;
    td_u8 pad2;
    td_u16 pad3;
    td_u32 pad4;
} ext_nv_test;

#if defined(PRODUCT_CFG_DEBUG_CRASH_DEMO)
__isr td_void led_flash_when_exc(td_void)
{
    uapi_watchdog_disable();
    uapi_io_set_func(EXT_GPIO_IDX_6, 0);
    uapi_io_set_dir(EXT_GPIO_IDX_6, EXT_GPIO_DIRECTION_OUT);
    uapi_io_set_ouput_val(EXT_GPIO_IDX_6, EXT_GPIO_VALUE0);

    while (1) {
        for (uintptr_t i = 0x02000000; i < 0x02038000; i += APP_LED_STEP) {
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
                   i, *(td_u32 *)i, *(td_u32 *)(i + APP_LED_STEP1),
                   *(td_u32 *)(i + APP_LED_STEP2), *(td_u32 *)(i + APP_LED_STEP3));
        }
    }
}
#endif

/* turn off the red light after sta joins the networksta */
__isr td_void led_off_sta_join_network(td_u32 event, td_pvoid event_info)
{
    /* set LED */
#if defined(PRODUCT_CFG_DEBUG_JTAG_LED_DEMO)
    uapi_io_set_func(EXT_GPIO_IDX_6, 0);
    uapi_io_set_dir(EXT_GPIO_IDX_6, EXT_GPIO_DIRECTION_OUT);
    uapi_io_set_ouput_val(EXT_GPIO_IDX_6, EXT_GPIO_VALUE0);
    uapi_io_set_func(EXT_GPIO_IDX_1, 1);
    uapi_led_init(EXT_LED_MODE_HIGH);
    uapi_io_set_func(EXT_GPIO_IDX_2, 1);
    uapi_io_set_func(EXT_GPIO_IDX_3, 1);
    uapi_led_init(EXT_LED_MODE_HIGH);
    uapi_led_off(EXT_LED_IDX_1);
#else
    uapi_io_set_func(EXT_GPIO_IDX_2, 1);
    uapi_io_set_func(EXT_GPIO_IDX_3, 1);
    uapi_led_init(EXT_LED_MODE_HIGH);
    uapi_led_off(EXT_LED_IDX_1);
#endif
    ext_unref_param(event);
    ext_unref_param(event_info);
}
__isr td_void led_on_sta_join_network(td_u32 event, td_pvoid event_info)
{
    /* set LED */
#if defined(PRODUCT_CFG_DEBUG_JTAG_LED_DEMO)
    uapi_io_set_func(EXT_GPIO_IDX_6, 0);
    uapi_io_set_dir(EXT_GPIO_IDX_6, EXT_GPIO_DIRECTION_OUT);
    uapi_io_set_ouput_val(EXT_GPIO_IDX_6, EXT_GPIO_VALUE1);
    uapi_io_set_func(EXT_GPIO_IDX_1, 1);
    uapi_led_init(EXT_LED_MODE_HIGH);
    uapi_io_set_func(EXT_GPIO_IDX_2, 1);
    uapi_io_set_func(EXT_GPIO_IDX_3, 1);
    uapi_led_init(EXT_LED_MODE_HIGH);
    uapi_led_on(EXT_LED_IDX_1);
#else
    uapi_io_set_func(EXT_GPIO_IDX_2, 1);
    uapi_io_set_func(EXT_GPIO_IDX_3, 1);
    uapi_led_init(EXT_LED_MODE_HIGH);
    uapi_led_on(EXT_LED_IDX_1);
#endif
    ext_unref_param(event);
    ext_unref_param(event_info);
}
td_void app_led_init(td_void)
{
    uapi_register_mac_event_call_back(MAC_STA_NETWORK_FORMED, led_off_sta_join_network);
    uapi_register_mac_event_call_back(MAC_STA_OFFLINE, led_on_sta_join_network);
}

/* test codes of uart0 and uart1 are switched dynamically,
 * which is convenient to connect port to the upper computer for debugging software when testing in large environment.
 */
td_u32 uart_test_nv_change_notify(td_u8 nv_id)
{
    td_u32 ret;

    if (nv_id != NV_ID_TEST) {
        return EXT_ERR_FAILURE;
    }

    ext_nv_test nv_test;
    ret = uapi_nv_read(NV_ID_TEST, (td_pvoid)&nv_test, sizeof(ext_nv_test));
    if (ret == EXT_ERR_SUCCESS) {
        /* Note: only applicable to the scenario with the same baud rate configuration of uart0 and uart1 */
        ext_uart_init_type uart_cfg = { 115200, 8, 1, 2 };

        if (nv_test.uart_cfg == 1) {
            /* app: uart1; diag: uart0 */
            app_uart_set_param(EXT_UART_1, uart_cfg);
            uapi_diag_set_uart_param(EXT_UART_0, uart_cfg);
        } else {
            /* app: uart0; diag: uart1 */
            app_uart_set_param(EXT_UART_0, uart_cfg);
            uapi_diag_set_uart_param(EXT_UART_1, uart_cfg);
        }
    }

    return ret;
}

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
EXT_CONST ext_diag_cmd_reg_obj g_app_diag_cmd[] = {
    { ID_DIAG_CMD_WHITELIST_CLEAR, ID_DIAG_CMD_APP_END, app_diag_cmd_proc },
};
#endif

td_void app_main(td_void)
{
    ext_nv_test nv_test;

    (td_void) uapi_board_init();
    (td_void) uapi_aes_init();
    uapi_mac_cp_init();
#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA) || defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
    uapi_load_spc_identify_component();
#endif

    uapi_phy_init();
    uapi_load_sg_and_1901_protocol();
    uapi_load_tl(APP_TL_QUEUE_SIZE);
#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)
    uapi_mdm_register_process_p2p_fc_function();
#endif
#if defined(PRODUCT_CFG_PRODUCT_TYPE_NDM)
    uapi_load_p2p_component_p1901();
#endif
    (td_void) uapi_mac_init();

    /* temporary configuration, test UART use */
    td_u32 ret = uapi_nv_read(NV_ID_TEST, (td_pvoid)&nv_test, sizeof(ext_nv_test));
    if (ret == EXT_ERR_SUCCESS && nv_test.uart_cfg == 1) {
        /* default is uart1, which is changed to UART0 here */
        ext_uart_init_type diag_uart_cfg = { 115200, 8, 1, 2 };
        uapi_diag_set_uart_param(EXT_UART_0, diag_uart_cfg);
    }
    app_pm_init();

    (td_void) uapi_diag_init();

#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)
    app_led_init();
#if defined(PRODUCT_CFG_SUPPORT_UPG_FROM_DBK)
    uapi_upg_sta_cmd_init();
#endif
#endif

#if defined(PRODUCT_CFG_DEBUG_CRASH_DEMO)
    uapi_syserr_exc_register((ext_syserr_exc_callback)led_flash_when_exc);
#endif

    uapi_mid_init();

    /* set the list upgrade partition information, and use the user data area by default:[0x7B000,0x7C000). */
#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
    ext_flash_partition_table *partition = uapi_get_partition_table();
    if (partition->table[EXT_FLASH_PARTITON_USER_INFO_1].size_blk >= 1) {
        uapi_upg_list_set_cache_info(partition->table[EXT_FLASH_PARTITON_USER_INFO_1].addr_blk, 1);
    }
#endif
    uapi_upg_init();
    app_srv_init();
}

td_u32 app_srv_init(td_void)
{
    td_u32 ret;
    app_srv_context *ctx = TD_NULL;
    ext_dms_chl_port_obj mac_chl_obj = { TD_NULL, app_plc_chl_rx, TD_NULL };

    ret = app_ctx_init();
    if (ret != EXT_ERR_SUCCESS) {
        return ret;
    }

    ctx = app_get_srv_context();
    app_uart_buf_init(&(ctx->buffer));

    ret = app_res_init(&ctx->res);
    if (ret != EXT_ERR_SUCCESS) {
        return ret;
    }

    (td_void) uapi_dms_chl_setup(&mac_chl_obj, EXT_DMS_CHL_MAC_PORT_APP);
    (td_void) uapi_diag_register_query_app_ver_cmd_callback((ext_mdm_query_app_ver)app_query_demo_version_info);
    /* uart configuration temporary use */
    ext_nv_test nv_test;
    ret = uapi_nv_read(NV_ID_TEST, (td_pvoid)&nv_test, sizeof(ext_nv_test));
    if (ret == EXT_ERR_SUCCESS && nv_test.uart_cfg == 1) {
        /* default is uart0, which is changed to uart1 here */
        ext_uart_init_type app_uart_cfg = { 115200, 8, 1, 2 };
        app_uart_set_param(EXT_UART_1, app_uart_cfg);
    }
    uapi_nv_register_change_nofity_proc(NV_ID_TEST, NV_ID_TEST, uart_test_nv_change_notify);

    return app_uart_init();
}

__hot app_srv_context *app_get_srv_context(td_void)
{
    return g_app_srv_context;
}

td_u32 app_ctx_init(td_void)
{
    if (g_app_srv_context != TD_NULL) {
        return EXT_ERR_INITILIZATION_ALREADY;
    }

    g_app_srv_context = (app_srv_context *)uapi_malloc(EXT_MOD_ID_APP_COMMON, sizeof(app_srv_context));
    if (g_app_srv_context == TD_NULL) {
        return EXT_ERR_MALLOC_FAILUE;
    }

    (td_void) memset_s(g_app_srv_context, sizeof(app_srv_context), 0, sizeof(app_srv_context));

    return EXT_ERR_SUCCESS;
}

td_void app_uart_buf_init(app_data_buf *uart_buf)
{
    if (uart_buf != TD_NULL) {
        uart_buf->data_th = APP_FRAME_DATA_MAX;
    }
}

td_u32 app_res_init(EXT_CONST app_res_cfg *res)
{
    td_u32 ret;
    td_s8 i;

    if (res == TD_NULL) {
        return EXT_ERR_FAILURE;
    }
    ret = uapi_msg_queue_create((td_u32 *)&res->queue_id, "appq", APP_QUEUE_SIZE, sizeof(ext_sys_queue_msg));
    if (ret != EXT_ERR_SUCCESS) {
        return EXT_ERR_FAILURE;
    }

    for (i = 0; i < APP_SEM_ID_MAX; i++) {
        ret = uapi_sem_bcreate((td_u32 *)&res->sem_id[i], "SEM_APP", 1);
        if (ret != EXT_ERR_SUCCESS) {
            return EXT_ERR_FAILURE;
        }
    }

    ret = uapi_task_create((td_u32 *)&res->task_id, "SRV_APP", app_srv_task_body, res->queue_id,
                         APP_SRV_TASK_STACK_SIZE, APP_SRV_TASK_PRIORITY);
    if (ret != EXT_ERR_SUCCESS) {
        return EXT_ERR_FAILURE;
    }

    return EXT_ERR_SUCCESS;
}

td_u32 g_startup_time = APP_STARTUP_TIME;
static td_void app_srv_task_body(td_u32 param)
{
    td_u32 ret;
    ext_sys_queue_msg msg = { 0 };
    td_u32 queue_id = param;

    uapi_time_counter_getms(&g_startup_time);
    printf("ENTER app_srv_task_body(). startup_time: %d\n", g_startup_time);

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
    uapi_diag_register_cmd(g_app_diag_cmd, ext_array_count(g_app_diag_cmd));
    app_whitelist_setup();
#endif

#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)
    (td_void) app_timer_start(APP_TIMER_ID_INIT, APP_TIMER_INIT, EXT_TIMER_TYPE_ONCE);
#endif

#if defined(PRODUCT_CFG_PLC_DEMO_TEST) && \
    (defined(PRODUCT_CFG_PRODUCT_TYPE_CCO) || defined(PRODUCT_CFG_PRODUCT_TYPE_STA))
    app_send_msg_setup((app_send_msg_func)app_send_message);
#endif
    for (;;) {
        ret = uapi_msg_queue_wait(queue_id, &msg, EXT_SYS_WAIT_FOREVER, sizeof(msg));
        if (ret == EXT_ERR_SUCCESS) {
#if defined(PRODUCT_CFG_PLC_DEMO_TEST)
#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
            app_handle_plc_test_event(&msg);
#endif
#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)
            app_handle_plc_test_sta_event(&msg);
#endif
#endif
            app_main_msg_proc(&msg);
        } else {
            uapi_sleep(APP_SLEEP_DURTION_MS);
        }
    }
}

static td_void app_main_msg_proc(EXT_CONST ext_sys_queue_msg *msg)
{
    if (msg == TD_NULL) {
        return;
    }
    switch (msg->msg_id) {
        case APP_MSG_ID_TIMER:
            app_on_timer(msg->param[0]);
            break;

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
        case APP_MSG_ID_DIAG:
            app_on_msg_diag(msg);
            break;
#endif

        default:
            break;
    }
}

static td_void app_on_timer(td_u32 timer_id)
{
    switch (timer_id) {
        case APP_TIMER_ID_UART:
            app_uart_rx_timeout();
            break;

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
        case APP_TIMER_ID_WHITELIST:
            app_whitelist_timeout_notify_mac();
            break;
        case APP_TIMER_ID_WRITE_WHITELIST_TO_FLASH:
            app_whitelist_timeout();
            break;
#endif

#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)
        case APP_TIMER_ID_INIT:
            app_on_init_alarm();
            break;
#endif

        default:
            break;
    }
}

td_void app_uart_rx(td_u8 ch)
{
    app_srv_context *ctx = app_get_srv_context();
    app_data_buf *uart_buf = &ctx->buffer;

    (td_void) app_semaphore_wait(APP_SEM_ID_UART, EXT_SYS_WAIT_FOREVER);
    (td_void) app_timer_start(APP_TIMER_ID_UART, APP_TIME_UART, EXT_TIMER_TYPE_ONCE);

    uart_buf->buffer[uart_buf->rx_len] = ch;
    uart_buf->rx_len++;
    if (uart_buf->data_th > APP_UART_BUF_SIZE) {
        (td_void) app_semaphore_signal(APP_SEM_ID_UART);
        return;
    }
    if (uart_buf->rx_len == uart_buf->data_th) {
        app_uart_frame_proc(uart_buf->buffer, uart_buf->data_th, sizeof(uart_buf->buffer));
        uart_buf->rx_len = 0;
    }

    (td_void) app_semaphore_signal(APP_SEM_ID_UART);
}

static td_void app_uart_rx_timeout(td_void)
{
    app_srv_context *ctx = app_get_srv_context();
    app_data_buf *uart_buf = &ctx->buffer;

    (td_void) app_semaphore_wait(APP_SEM_ID_UART, EXT_SYS_WAIT_FOREVER);
    if (uart_buf->rx_len > APP_UART_BUF_SIZE) {
        (td_void) app_semaphore_signal(APP_SEM_ID_UART);
        return;
    }
    if (uart_buf->rx_len > 0) {
        app_uart_frame_proc(uart_buf->buffer, uart_buf->rx_len, sizeof(uart_buf->buffer));
        uart_buf->rx_len = 0;
    }

    (td_void) app_semaphore_signal(APP_SEM_ID_UART);
}

static td_void app_fill_plc_ctrl(ext_dms_chl_tx_ctrl *tx_ctrl)
{
    if (tx_ctrl == TD_NULL) {
        return;
    }
    tx_ctrl->ctrl = EXT_DMS_CHL_TX_CTRL_FORCE;
    tx_ctrl->type = EXT_DMS_CHL_TX_TYPE_MR;
    tx_ctrl->prio = APP_SEND_PLC_PRIO;
    tx_ctrl->id = APP_PLC_ID_DATA_TRANSMIT;
}

static td_void app_send_plc_frame(EXT_CONST td_u8 *data, td_u16 data_len)
{
    ext_dms_chl_tx_data *tx_data = TD_NULL;
    ext_dms_chl_tx_ctrl *tx_ctrl = TD_NULL;
    app_data_transmit_frame *trans_frame = TD_NULL;
    td_u32 frame_length;

    if (data == TD_NULL) {
        return;
    }

    tx_ctrl = (ext_dms_chl_tx_addr_s *)uapi_malloc(EXT_MOD_ID_APP_COMMON, sizeof(ext_dms_chl_tx_addr_s));
    if (tx_ctrl == TD_NULL) {
        return;
    }

    frame_length = sizeof(ext_dms_chl_tx_data) + sizeof(app_data_transmit_frame) + data_len;
    tx_data = (ext_dms_chl_tx_data *)uapi_malloc(EXT_MOD_ID_APP_COMMON, frame_length);
    if (tx_data == TD_NULL) {
        uapi_free(EXT_MOD_ID_APP_COMMON, tx_ctrl);
        tx_ctrl = TD_NULL;
        return;
    }

    (td_void) memset_s(tx_ctrl, sizeof(ext_dms_chl_tx_addr_s), 0, sizeof(ext_dms_chl_tx_addr_s));
    (td_void) memset_s(tx_data, frame_length, 0, frame_length);

    trans_frame = (app_data_transmit_frame *)tx_data->data;
    trans_frame->dir = APP_DATA_TRANSMIT_DIR;
    trans_frame->stru_size = sizeof(app_data_transmit_frame);
    trans_frame->data_length = data_len;
    if (trans_frame->data_length > 0 &&
        memcpy_s(tx_data->data + trans_frame->stru_size, trans_frame->data_length, data, data_len) != EOK) {
        uapi_free(EXT_MOD_ID_APP_COMMON, tx_data);
        tx_data = TD_NULL;
        uapi_free(EXT_MOD_ID_APP_COMMON, tx_ctrl);
        tx_ctrl = TD_NULL;
        return;
    }

    tx_data->data_size = sizeof(app_data_transmit_frame) + data_len;

    if (memcpy_s(tx_ctrl->dest_mac, sizeof(tx_ctrl->dest_mac), APP_DEST_MAC_ADDR, EXT_PLC_MAC_ADDR_LEN) != EOK) {
        uapi_free(EXT_MOD_ID_APP_COMMON, tx_data);
        tx_data = TD_NULL;
        uapi_free(EXT_MOD_ID_APP_COMMON, tx_ctrl);
        tx_ctrl = TD_NULL;
        return;
    }
    app_fill_plc_ctrl(tx_ctrl);
    uapi_dms_chl_tx(EXT_DMS_CHL_MAC_PORT_APP, tx_data, tx_ctrl);

    uapi_free(EXT_MOD_ID_APP_COMMON, tx_data);
    tx_data = TD_NULL;

    uapi_free(EXT_MOD_ID_APP_COMMON, tx_ctrl);
    tx_ctrl = TD_NULL;
}

static td_void app_uart_frame_proc(EXT_CONST td_pbyte frame, td_u16 length, td_u16 max_frame_len)
{
    ext_unref_param(max_frame_len);
#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)
    if (app_handle_dut_frame(frame, length) == TD_TRUE) {
        return;
    }
#endif

    app_send_plc_frame(frame, length);
}

static td_u32 app_plc_chl_rx(td_u16 port, EXT_CONST ext_dms_chl_rx *chl_rx)
{
    ext_unref_param(port);
    app_data_transmit_frame *frame = TD_NULL;
#if defined(PRODUCT_CFG_PLC_DEMO_TEST)
    app_plc_test_chl_rx(port, (ext_dms_chl_rx *)chl_rx);
#endif
    if (chl_rx == TD_NULL || chl_rx->payload == TD_NULL) {
        return EXT_ERR_INVALID_PARAMETER;
    }
    if (chl_rx->id != APP_PLC_ID_DATA_TRANSMIT) {
        return EXT_ERR_NOT_SUPPORT_ID;
    }

    frame = (app_data_transmit_frame *)chl_rx->payload;
    if (frame->stru_size + frame->data_length != chl_rx->payload_len) {
        return EXT_ERR_INVALID_PARAMETER;
    }

    return app_uart_tx(chl_rx->payload + frame->stru_size, frame->data_length);
}

#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)
td_void app_on_init_alarm(td_void)
{
    td_u8 mac[EXT_PLC_MAC_ADDR_LEN] = { 0 };

    (td_void) uapi_get_local_mac_addr(mac, sizeof(mac));
    app_set_mac_addr(mac, sizeof(mac));
}
#endif

static td_u32 app_query_demo_version_info(EXT_OUT td_pbyte ver_info, EXT_IN EXT_CONST td_u16 ver_info_size)
{
    td_char app_name[] = {"DEMO"};
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

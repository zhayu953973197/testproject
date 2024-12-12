/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS STA I/O.
 */

#include "mrs_sta_io.h"
#include <soc_mdm_power_failure.h>
#include "mrs_config.h"
#include "mrs_common_tools.h"
#include "mrs_sta_simu_meter.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)
#define MRS_PLC_LED_RX EXT_LED_IDX_0
#define MRS_PLC_LED_TX EXT_LED_IDX_1

td_u8 g_sta_evtout_check_num = 0;
td_u8 g_sta_evtout_param = 0;
mrs_io_evtout_callback g_sta_evtout_callback = TD_NULL;

/* RST INT */
td_void mrs_sta_reset_isr_callback(td_void);

/* EVENT_OUT INT */
td_void mrs_sta_event_out_isr_callback(td_void);

/* Callback of PLC Tx, turn on the tx led */
td_void mrs_sta_plc_tx_callback(td_u8 dt, td_u16 source_type, td_u8 phase);

/* callback of PLC Rx, turn on the rx led */
td_void mrs_sta_plc_rx_callback(td_u8 dt, td_u16 source_type, td_u8 phase);

/* forbid to call dynamic domain content */
__hot td_void app_io_init(td_void)
{
    td_u32 io_dir = 0x003FEE3F;
    td_u32 io_val = 0x00006600;
#if defined(PRODUCT_CFG_DEBUG_JTAG_LED_MRS)
    uapi_io_set_func(EXT_GPIO_IDX_6, 0); /* GPIO6 replace GPIO2_LED0 */
    uapi_io_set_dir(EXT_GPIO_IDX_6, EXT_GPIO_DIRECTION_OUT);
    uapi_io_set_func(EXT_GPIO_IDX_1, 1); /* GPIO1_LED4 replace GPIO3_LED1 */
#endif
    /* LED */
    uapi_io_set_func(EXT_GPIO_IDX_2, 1); /* LED_RX */
    uapi_io_set_func(EXT_GPIO_IDX_3, 1); /* LED_TX */
    uapi_led_init(EXT_LED_MODE_HIGH);

    /* UART0 */
    uapi_io_set_func(EXT_GPIO_IDX_9, 1);  /* GPIO_9_SEL:  UART0_RXD */
    uapi_io_set_func(EXT_GPIO_IDX_10, 3); /* GPIO_10_SEL: UART0_N_TXD , value: 3 */

    /* EVENT_OUT */
    uapi_io_set_func(EXT_GPIO_IDX_7, 0); /* GPIO_7_SEL:  GPIO_7, EVENT_OUT */

    /* RST */
    uapi_io_set_func(EXT_GPIO_IDX_8, 0); /* GPIO_8_SEL:  GPIO_8, RST */

    /* LDRVR_PD */
    uapi_io_set_func(EXT_GPIO_IDX_5, 0);

    if (uapi_get_hw_chip_type() == EXT_FTM_CHIP_TYPE_21EV) {
        /* set drive capability of pins */
        uapi_io_set_driver_strength(EXT_IO_CTL_SFC_CLK, EXT_IO_DRIVER_STRENGTH_2);
        uapi_io_set_driver_strength(EXT_IO_CTL_SFC_WPN, EXT_IO_DRIVER_STRENGTH_2);
        uapi_io_set_driver_strength(EXT_IO_CTL_SFC_HOLDN, EXT_IO_DRIVER_STRENGTH_2);
        uapi_io_set_driver_strength(EXT_IO_CTL_SFC_DI, EXT_IO_DRIVER_STRENGTH_2);
        uapi_io_set_driver_strength(EXT_IO_CTL_SFC_DO, EXT_IO_DRIVER_STRENGTH_2);

        io_dir = 0x00587FFE;
    }

    uapi_io_init(&io_val, &io_dir);
}

td_u32 mrs_sta_io_init(td_void)
{
    ext_gpio_value_e value = EXT_GPIO_VALUE0;
#if defined(PRODUCT_CFG_DEBUG_JTAG_LED_MRS)
    uapi_led_flash(EXT_LED_IDX_4, 1000, 1000); /* time of led on/off: 1000/1000 */
#endif
    uapi_led_flash(MRS_PLC_LED_RX, 1000, 1000); /* time of led on/off: 1000/1000 */
    uapi_led_flash(MRS_PLC_LED_TX, 1000, 1000); /* time of led on/off: 1000/1000 */

    /* GPIO8: RESET */
    uapi_io_set_func(MRS_CFG_IO_RST, 0);
    uapi_io_set_dir(MRS_CFG_IO_RST, EXT_GPIO_DIRECTION_IN);
    uapi_io_int_connect(MRS_CFG_IO_RST, EXT_INT_EDGE, EXT_EDGE_FALL_LEVEL_LOW, mrs_sta_reset_isr_callback);

    /* GPIO7: EVENT_OUT */
    if (mrs_simu_meter_flag() == TD_FALSE) {
        uapi_io_set_func(MRS_CFG_STA_IO_EVT_OUT, 0);
        uapi_io_set_dir(MRS_CFG_STA_IO_EVT_OUT, EXT_GPIO_DIRECTION_IN);
        uapi_io_int_connect(MRS_CFG_STA_IO_EVT_OUT, EXT_INT_EDGE, EXT_EDGE_RISE_LEVEL_HIGH,
            mrs_sta_event_out_isr_callback);

        uapi_io_get_input_val(MRS_CFG_STA_IO_EVT_OUT, &value);
        if (value == EXT_GPIO_VALUE1) {
            mrs_sta_event_out_isr_callback();
        }
    }

    uapi_register_led_handle_frame_send_end_func(mrs_sta_plc_tx_callback);
    uapi_register_led_handle_frame_rcv_end_func(mrs_sta_plc_rx_callback);

    return EXT_ERR_SUCCESS;
}

__isr td_void mrs_sta_reset_isr_callback(td_void)
{
    ext_gpio_value_e value = EXT_GPIO_VALUE0;

    uapi_io_get_input_val(MRS_CFG_IO_RST, &value);

    mrs_timer_stop(MRS_TIMER_ID_RST_IO);

    uapi_io_int_disconnect(MRS_CFG_IO_RST);
    if (value == EXT_GPIO_VALUE0) {
        mrs_timer_start(MRS_TIMER_ID_RST_IO, MRS_RST_IO_PERIOD, EXT_TIMER_TYPE_ONCE);
        uapi_io_int_connect(MRS_CFG_IO_RST, EXT_INT_EDGE, EXT_EDGE_RISE_LEVEL_HIGH, mrs_sta_reset_isr_callback);
    } else {
        uapi_io_int_connect(MRS_CFG_IO_RST, EXT_INT_EDGE, EXT_EDGE_FALL_LEVEL_LOW, mrs_sta_reset_isr_callback);
    }
}

__isr td_void mrs_sta_event_out_isr_callback(td_void)
{
    ext_gpio_value_e value = EXT_GPIO_VALUE0;

    uapi_io_get_input_val(MRS_CFG_STA_IO_EVT_OUT, &value);
    if (value != EXT_GPIO_VALUE1) {
        return;
    }

    if (g_sta_evtout_check_num == 0) {
        mrs_timer_start(MRS_TIMER_ID_STA_IO_EVTOUT, MRS_STA_IO_EVTOUT_TIMEOUT, EXT_TIMER_TYPE_ONCE);
    }
}

td_void mrs_sta_io_evtout_timeout_handle(td_void)
{
    ext_gpio_value_e value = EXT_GPIO_VALUE0;

    uapi_io_get_input_val(MRS_CFG_STA_IO_EVT_OUT, &value);
    if ((td_u8)value != EXT_EDGE_RISE_LEVEL_HIGH) {
        g_sta_evtout_check_num = 0;
        return;
    }

    g_sta_evtout_check_num++;
    if (g_sta_evtout_check_num < MRS_CFG_STA_EVT_CHECK_MAX) {
        mrs_timer_start(MRS_TIMER_ID_STA_IO_EVTOUT, MRS_STA_IO_EVTOUT_TIMEOUT, EXT_TIMER_TYPE_ONCE);
        return;
    }

    g_sta_evtout_check_num = 0;
    if (g_sta_evtout_callback != TD_NULL) {
        g_sta_evtout_callback(g_sta_evtout_param, EXT_EDGE_RISE_LEVEL_HIGH);
    }
}

td_void mrs_sta_io_register_evtout_callback(mrs_io_evtout_callback callback, td_u8 param)
{
    g_sta_evtout_callback = callback;
    g_sta_evtout_param = param;
}

td_void mrs_sta_led_join_network(td_void)
{
#if defined(PRODUCT_CFG_DEBUG_JTAG_LED_MRS)
    uapi_io_set_ouput_val(EXT_GPIO_IDX_6, EXT_GPIO_VALUE0);
    uapi_led_off(EXT_LED_IDX_4);
#endif
    uapi_led_off(MRS_PLC_LED_RX);
    uapi_led_off(MRS_PLC_LED_TX);
}

td_void mrs_sta_led_offline(td_void)
{
#if defined(PRODUCT_CFG_DEBUG_JTAG_LED_MRS)
    uapi_io_set_ouput_val(EXT_GPIO_IDX_6, EXT_GPIO_VALUE1);
    uapi_led_on(EXT_LED_IDX_4);
#endif
    uapi_led_on(MRS_PLC_LED_RX);
    uapi_led_on(MRS_PLC_LED_TX);
}

td_void mrs_sta_plc_tx_callback(td_u8 dt, td_u16 source_type, td_u8 phase)
{
    ext_unref_param(phase);

    if ((dt != SOF_DT) || (source_type != APP_TYPE)) {
        return;
    }

#if defined(PRODUCT_CFG_DEBUG_JTAG_LED_MRS)
    uapi_led_on(EXT_LED_IDX_4);
#endif

    if (uapi_get_join_net_state() == TD_FALSE) {
        return;
    }

    uapi_led_on(MRS_PLC_LED_TX);

    mrs_timer_start(MRS_TIMER_ID_PLC_LED_TX, MRS_PLC_LED_TIME, EXT_TIMER_TYPE_ONCE);
}

td_void mrs_sta_plc_rx_callback(td_u8 dt, td_u16 source_type, td_u8 phase)
{
    ext_unref_param(phase);

    if ((dt != SOF_DT) || (source_type != APP_TYPE)) {
        return;
    }

#if defined(PRODUCT_CFG_DEBUG_JTAG_LED_MRS)
    uapi_io_set_ouput_val(EXT_GPIO_IDX_6, EXT_GPIO_VALUE1);
#endif

    if (uapi_get_join_net_state() == TD_FALSE) {
        return;
    }

    uapi_led_on(MRS_PLC_LED_RX);

    mrs_timer_start(MRS_TIMER_ID_PLC_LED_RX, MRS_PLC_LED_TIME, EXT_TIMER_TYPE_ONCE);
}

td_void mrs_sta_plc_rx_led_off(td_void)
{
#if defined(PRODUCT_CFG_DEBUG_JTAG_LED_MRS)
    uapi_io_set_ouput_val(EXT_GPIO_IDX_6, EXT_GPIO_VALUE0);
#endif
    uapi_led_off(MRS_PLC_LED_RX);
}

td_void mrs_sta_plc_tx_led_off(td_void)
{
#if defined(PRODUCT_CFG_DEBUG_JTAG_LED_MRS)
    uapi_led_off(EXT_LED_IDX_4);
#endif
    uapi_led_off(MRS_PLC_LED_TX);
}

td_void uapi_set_power_failure_module_gpio_init(ext_gpio_idx_e id, td_u8 io_val, ext_gpio_direction_e dir)
{
    uapi_io_set_func(id, io_val);
    uapi_io_set_dir(id, dir);
}

/* is plug module */
td_u32 uapi_get_power_failure_is_plug_module(td_void)
{
    td_u32 ret;
    ext_gpio_value_e gpio0_value = EXT_GPIO_VALUE0;

    /* STA module judge current level. 1 means high-level. high-level means module is plug module */
    ret = uapi_io_get_input_val(EXT_GPIO_IDX_0, &gpio0_value);
    if (ret != EXT_ERR_SUCCESS) {
        return EXT_ERR_FAILURE;
    }

    if (gpio0_value == EXT_GPIO_VALUE1) {
        ret = EXT_ERR_FAILURE;
    } else {
        /* not plug module */
        ret = EXT_ERR_SUCCESS;
    }

    return ret;
}

#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_STA) */


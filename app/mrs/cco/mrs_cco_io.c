/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS STA I/O.
 */

#include "mrs_cco_io.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
/* set pahse */
td_void mrs_plc_set_tx_single_phase(td_u32 phase_name, td_u8 phase_mode);
td_void mrs_plc_set_rx_single_phase(td_u32 phase_name, td_u8 phase_mode);
/* trigger callback function */
td_void mrs_cco_plc_tx_callback(td_u8 dt, td_u16 source_type, td_u8 phase);
td_void mrs_cco_plc_rx_callback(td_u8 dt, td_u16 source_type, td_u8 phase);

td_void mrs_cco_reset_isr_callback(td_void);

td_void mrs_plc_tx_phase_a_on_ev(td_void);
td_void mrs_plc_tx_phase_b_on_ev(td_void);
td_void mrs_plc_tx_phase_c_on_ev(td_void);
td_void mrs_plc_tx_phase_a_off_ev(td_void);
td_void mrs_plc_tx_phase_b_off_ev(td_void);
td_void mrs_plc_tx_phase_c_off_ev(td_void);

td_void mrs_plc_tx_phase_a_on_sv(td_void);
td_void mrs_plc_tx_phase_b_on_sv(td_void);
td_void mrs_plc_tx_phase_c_on_sv(td_void);
td_void mrs_plc_tx_phase_a_off_sv(td_void);
td_void mrs_plc_tx_phase_b_off_sv(td_void);
td_void mrs_plc_tx_phase_c_off_sv(td_void);

td_void mrs_plc_tx_phase_a_on(td_void);
td_void mrs_plc_tx_phase_b_on(td_void);
td_void mrs_plc_tx_phase_c_on(td_void);

typedef td_void (*mrs_plc_led_callback)(td_void);

td_u8 g_cco_io_rst = (td_u8)EXT_GPIO_IDX_19;
td_u8 g_cco_phase_a_tx = (td_u8)EXT_GPIO_IDX_15;
td_u8 g_cco_phase_a_rx = (td_u8)EXT_GPIO_IDX_16;
td_u8 g_cco_phase_b_tx = (td_u8)EXT_GPIO_IDX_6;
td_u8 g_cco_phase_b_rx = (td_u8)EXT_GPIO_IDX_21;
td_u8 g_cco_phase_c_tx = (td_u8)EXT_GPIO_IDX_20;
td_u8 g_cco_phase_c_rx = (td_u8)EXT_GPIO_IDX_0;

mrs_plc_led_callback g_cco_phase_a_led_on = mrs_plc_tx_phase_a_on_ev;
mrs_plc_led_callback g_cco_phase_b_led_on = mrs_plc_tx_phase_b_on_ev;
mrs_plc_led_callback g_cco_phase_c_led_on = mrs_plc_tx_phase_c_on_ev;
mrs_plc_led_callback g_cco_phase_a_led_off = mrs_plc_tx_phase_a_off_ev;
mrs_plc_led_callback g_cco_phase_b_led_off = mrs_plc_tx_phase_b_off_ev;
mrs_plc_led_callback g_cco_phase_c_led_off = mrs_plc_tx_phase_c_off_ev;

__isr td_void app_io_init_chip_ev(td_u32 *io_dir)
{
    uapi_io_set_func(EXT_GPIO_IDX_1, 1); /* A_LED */
    uapi_io_set_func(EXT_GPIO_IDX_6, 0); /* SWITCH_A_TX */
    uapi_io_set_func(EXT_GPIO_IDX_4, 0); /* SWITCH_A_RX */

    uapi_io_set_func(EXT_GPIO_IDX_22, 0); /* B_LED */
    uapi_io_set_func(EXT_GPIO_IDX_7, 0);  /* SWITCH_B_TX */
    uapi_io_set_func(EXT_GPIO_IDX_8, 0);  /* SWITCH_B_RX */

    uapi_io_set_func(EXT_GPIO_IDX_20, 0); /* C_LED */
    uapi_io_set_func(EXT_GPIO_IDX_11, 0); /* SWITCH_C_TX */
    uapi_io_set_func(EXT_GPIO_IDX_12, 0); /* SWITCH_C_RX */

    g_cco_phase_a_tx = (td_u8)EXT_GPIO_IDX_6;
    g_cco_phase_a_rx = (td_u8)EXT_GPIO_IDX_4;
    g_cco_phase_b_tx = (td_u8)EXT_GPIO_IDX_7;
    g_cco_phase_b_rx = (td_u8)EXT_GPIO_IDX_8;
    g_cco_phase_c_tx = (td_u8)EXT_GPIO_IDX_11;
    g_cco_phase_c_rx = (td_u8)EXT_GPIO_IDX_12;

    uapi_io_set_func(EXT_GPIO_IDX_19, 0); /* GPIO_19_SEL: GPIO_19, RST */
    g_cco_io_rst = (td_u8)EXT_GPIO_IDX_19;

    /* set drive capability of pins */
    uapi_io_set_driver_strength(EXT_IO_CTL_SFC_CLK, EXT_IO_DRIVER_STRENGTH_2);
    uapi_io_set_driver_strength(EXT_IO_CTL_SFC_WPN, EXT_IO_DRIVER_STRENGTH_2);
    uapi_io_set_driver_strength(EXT_IO_CTL_SFC_HOLDN, EXT_IO_DRIVER_STRENGTH_2);
    uapi_io_set_driver_strength(EXT_IO_CTL_SFC_DI, EXT_IO_DRIVER_STRENGTH_2);
    uapi_io_set_driver_strength(EXT_IO_CTL_SFC_DO, EXT_IO_DRIVER_STRENGTH_2);

    g_cco_phase_a_led_on = mrs_plc_tx_phase_a_on_ev;
    g_cco_phase_b_led_on = mrs_plc_tx_phase_b_on_ev;
    g_cco_phase_c_led_on = mrs_plc_tx_phase_c_on_ev;
    g_cco_phase_a_led_off = mrs_plc_tx_phase_a_off_ev;
    g_cco_phase_b_led_off = mrs_plc_tx_phase_b_off_ev;
    g_cco_phase_c_led_off = mrs_plc_tx_phase_c_off_ev;

    *io_dir = 0x007FFFFF;
}
__isr td_void app_io_init_chip_sv(td_u32 *io_dir)
{
    uapi_io_set_func(EXT_GPIO_IDX_19, 0); /* A_LED */
    uapi_io_set_func(EXT_GPIO_IDX_15, 0); /* SWITCH_A_TX */
    uapi_io_set_func(EXT_GPIO_IDX_16, 0); /* SWITCH_A_RX */

    uapi_io_set_func(EXT_GPIO_IDX_1, 1);  /* B_LED */
    uapi_io_set_func(EXT_GPIO_IDX_6, 0);  /* SWITCH_B_TX */
    uapi_io_set_func(EXT_GPIO_IDX_21, 0); /* SWITCH_B_RX */

    uapi_io_set_func(EXT_GPIO_IDX_4, 1);  /* C_LED */
    uapi_io_set_func(EXT_GPIO_IDX_20, 0); /* SWITCH_C_TX */
    uapi_io_set_func(EXT_GPIO_IDX_0, 0);  /* SWITCH_C_RX */

    g_cco_phase_a_tx = (td_u8)EXT_GPIO_IDX_15;
    g_cco_phase_a_rx = (td_u8)EXT_GPIO_IDX_16;
    g_cco_phase_b_tx = (td_u8)EXT_GPIO_IDX_6;
    g_cco_phase_b_rx = (td_u8)EXT_GPIO_IDX_21;
    g_cco_phase_c_tx = (td_u8)EXT_GPIO_IDX_20;
    g_cco_phase_c_rx = (td_u8)EXT_GPIO_IDX_0;

    /* RST */
    uapi_io_set_func(EXT_GPIO_IDX_8, 0); /* GPIO_8_SEL:  GPIO_8, RST */
    g_cco_io_rst = (td_u8)EXT_GPIO_IDX_8;

    g_cco_phase_a_led_on = mrs_plc_tx_phase_a_on_sv;
    g_cco_phase_b_led_on = mrs_plc_tx_phase_b_on_sv;
    g_cco_phase_c_led_on = mrs_plc_tx_phase_c_on_sv;
    g_cco_phase_a_led_off = mrs_plc_tx_phase_a_off_sv;
    g_cco_phase_b_led_off = mrs_plc_tx_phase_b_off_sv;
    g_cco_phase_c_led_off = mrs_plc_tx_phase_c_off_sv;

    *io_dir = 0x003FEF7F;
}

__isr td_void app_io_init(td_void)
{
    td_u32 chip_type = uapi_get_hw_chip_type();
    td_u32 io_dir = 0x0078FFFF;
    td_u32 io_val = 0x00006640;

    /* UART0 */
    uapi_io_set_func(EXT_GPIO_IDX_9, 1);  /* GPIO_9_SEL:  UART0_RXD */
    uapi_io_set_func(EXT_GPIO_IDX_10, 3); /* GPIO_10_SEL: UART0_N_TXD ,value:3 */

    /* LED */
    uapi_io_set_func(EXT_GPIO_IDX_2, 1); /* LED_RX */
    uapi_io_set_func(EXT_GPIO_IDX_3, 1); /* LED_TX */
    uapi_led_init(EXT_LED_MODE_HIGH);

    /* LDRVR_PD */
    uapi_io_set_func(EXT_GPIO_IDX_5, 0);

    if (chip_type == EXT_FTM_CHIP_TYPE_21EV) {
        app_io_init_chip_ev(&io_dir);
    } else {
        app_io_init_chip_sv(&io_dir);
    }

    uapi_io_init(&io_val, &io_dir);
}

td_u32 mrs_cco_io_init(td_void)
{
    /* GPIO8: RESET */
    uapi_io_set_func((ext_gpio_idx_e)g_cco_io_rst, 0);
    uapi_io_set_dir((ext_gpio_idx_e)g_cco_io_rst, EXT_GPIO_DIRECTION_IN);
    uapi_io_int_connect((ext_gpio_idx_e)g_cco_io_rst, EXT_INT_EDGE,
        EXT_EDGE_FALL_LEVEL_LOW, mrs_cco_reset_isr_callback);

    uapi_register_led_handle_frame_send_end_func(mrs_cco_plc_tx_callback);
    uapi_register_led_handle_frame_rcv_end_func(mrs_cco_plc_rx_callback);

    return EXT_ERR_SUCCESS;
}

__isr td_void mrs_cco_reset_isr_callback(td_void)
{
    ext_gpio_value_e value = EXT_GPIO_VALUE0;

    uapi_io_get_input_val((ext_gpio_idx_e)g_cco_io_rst, &value);

    mrs_timer_stop(MRS_TIMER_ID_RST_IO);

    uapi_io_int_disconnect((ext_gpio_idx_e)g_cco_io_rst);
    if (value == EXT_GPIO_VALUE0) {
        mrs_timer_start(MRS_TIMER_ID_RST_IO, MRS_RST_IO_PERIOD, EXT_TIMER_TYPE_ONCE);
        uapi_io_int_connect((ext_gpio_idx_e)g_cco_io_rst, EXT_INT_EDGE,
            EXT_EDGE_RISE_LEVEL_HIGH, mrs_cco_reset_isr_callback);
    } else {
        uapi_io_int_connect((ext_gpio_idx_e)g_cco_io_rst, EXT_INT_EDGE,
            EXT_EDGE_FALL_LEVEL_LOW, mrs_cco_reset_isr_callback);
    }
}

__isr td_void mrs_plc_set_tx_single_phase(td_u32 phase_name, td_u8 phase_mode)
{
    ext_gpio_value_e value = (phase_mode == PHASE_MODE_ON) ? EXT_GPIO_VALUE1 : EXT_GPIO_VALUE0;

    switch (phase_name) {
        case PHASE_B:
            uapi_io_set_ouput_val((ext_gpio_idx_e)g_cco_phase_b_tx, value);
            break;

        case PHASE_C:
            uapi_io_set_ouput_val((ext_gpio_idx_e)g_cco_phase_c_tx, value);
            break;

        default:
            uapi_io_set_ouput_val((ext_gpio_idx_e)g_cco_phase_a_tx, value);
            break;
    }
}

__isr td_void mrs_plc_set_rx_single_phase(td_u32 phase_name, td_u8 phase_mode)
{
    ext_gpio_value_e value = (phase_mode == PHASE_MODE_ON) ? EXT_GPIO_VALUE1 : EXT_GPIO_VALUE0;

    switch (phase_name) {
        case PHASE_B:
            uapi_io_set_ouput_val((ext_gpio_idx_e)g_cco_phase_b_rx, value);
            break;

        case PHASE_C:
            uapi_io_set_ouput_val((ext_gpio_idx_e)g_cco_phase_c_rx, value);
            break;

        default:
            uapi_io_set_ouput_val((ext_gpio_idx_e)g_cco_phase_a_rx, value);
            break;
    }
}

td_void mrs_cco_phase_init(td_void)
{
    uapi_register_phase_func(TD_NULL, mrs_plc_set_tx_single_phase, mrs_plc_set_rx_single_phase);
}

td_void mrs_cco_plc_tx_callback(td_u8 dt, td_u16 source_type, td_u8 phase)
{
    if ((dt != SOF_DT) || (source_type != APP_TYPE)) {
        return;
    }

    uapi_led_on(MRS_PLC_LED_TX);
    mrs_timer_start(MRS_TIMER_ID_PLC_LED_TX, MRS_PLC_LED_TIME, EXT_TIMER_TYPE_ONCE);

    switch (phase) {
        case PHASE_ALL:
            mrs_plc_tx_phase_a_on();
            mrs_plc_tx_phase_b_on();
            mrs_plc_tx_phase_c_on();
            break;

        case PHASE_A:
            mrs_plc_tx_phase_a_on();
            break;

        case PHASE_B:
            mrs_plc_tx_phase_b_on();
            break;

        case PHASE_C:
            mrs_plc_tx_phase_c_on();
            break;

        default:
            break;
    }
}

td_void mrs_cco_plc_rx_callback(td_u8 dt, td_u16 source_type, td_u8 phase)
{
    ext_unref_param(phase);

    if ((dt != SOF_DT) || (source_type != APP_TYPE)) {
        return;
    }

    uapi_led_on(MRS_PLC_LED_RX);
    mrs_timer_start(MRS_TIMER_ID_PLC_LED_RX, MRS_PLC_LED_TIME, EXT_TIMER_TYPE_ONCE);
}

td_void mrs_plc_tx_phase_a_on_ev(td_void)
{
    uapi_led_on(EXT_LED_IDX_4);
}

td_void mrs_plc_tx_phase_b_on_ev(td_void)
{
    uapi_io_set_ouput_val(EXT_GPIO_IDX_22, EXT_GPIO_VALUE1);
}

td_void mrs_plc_tx_phase_c_on_ev(td_void)
{
    uapi_io_set_ouput_val(EXT_GPIO_IDX_20, EXT_GPIO_VALUE1);
}

td_void mrs_plc_tx_phase_a_off_ev(td_void)
{
    uapi_led_off(EXT_LED_IDX_4);
}

td_void mrs_plc_tx_phase_b_off_ev(td_void)
{
    uapi_io_set_ouput_val(EXT_GPIO_IDX_22, EXT_GPIO_VALUE0);
}

td_void mrs_plc_tx_phase_c_off_ev(td_void)
{
    uapi_io_set_ouput_val(EXT_GPIO_IDX_20, EXT_GPIO_VALUE0);
}

td_void mrs_plc_tx_phase_a_on_sv(td_void)
{
    uapi_io_set_ouput_val(EXT_GPIO_IDX_19, EXT_GPIO_VALUE1);
}

td_void mrs_plc_tx_phase_b_on_sv(td_void)
{
    uapi_led_on(EXT_LED_IDX_4);
}

td_void mrs_plc_tx_phase_c_on_sv(td_void)
{
    uapi_led_on(EXT_LED_IDX_2);
}

td_void mrs_plc_tx_phase_a_off_sv(td_void)
{
    uapi_io_set_ouput_val(EXT_GPIO_IDX_19, EXT_GPIO_VALUE0);
}

td_void mrs_plc_tx_phase_b_off_sv(td_void)
{
    uapi_led_off(EXT_LED_IDX_4);
}

td_void mrs_plc_tx_phase_c_off_sv(td_void)
{
    uapi_led_off(EXT_LED_IDX_2);
}

td_void mrs_plc_tx_phase_a_off(td_void)
{
    g_cco_phase_a_led_off();
}

td_void mrs_plc_tx_phase_b_off(td_void)
{
    g_cco_phase_b_led_off();
}

td_void mrs_plc_tx_phase_c_off(td_void)
{
    g_cco_phase_c_led_off();
}

td_void mrs_plc_tx_phase_a_on(td_void)
{
    g_cco_phase_a_led_on();
    mrs_timer_start(MRS_TIMER_ID_LED_PHASE_A, MRS_PLC_LED_TIME, EXT_TIMER_TYPE_ONCE);
}

td_void mrs_plc_tx_phase_b_on(td_void)
{
    g_cco_phase_b_led_on();
    mrs_timer_start(MRS_TIMER_ID_LED_PHASE_B, MRS_PLC_LED_TIME, EXT_TIMER_TYPE_ONCE);
}

td_void mrs_plc_tx_phase_c_on(td_void)
{
    g_cco_phase_c_led_on();
    mrs_timer_start(MRS_TIMER_ID_LED_PHASE_C, MRS_PLC_LED_TIME, EXT_TIMER_TYPE_ONCE);
}

#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_CCO) */

/*
 * Copyright (c) CompanyNameMagicTag 2019-2019. All rights reserved.
 * Description: phase driver config.
 */
#include "soc_mdm_phy.h"
#include "soc_mdm_io.h"

/* polyphase RX channel control, GPIO pin corresponding to receive bit switch */
#define RX_PHASE_A_GPIO_PIN EXT_GPIO_IDX_6
#define RX_PHASE_B_GPIO_PIN EXT_GPIO_IDX_8
#define RX_PHASE_C_GPIO_PIN EXT_GPIO_IDX_12
/* GPIO pin corresponding to transmit bit switch */
#define TX_PHASE_A_GPIO_PIN EXT_GPIO_IDX_21
#define TX_PHASE_B_GPIO_PIN EXT_GPIO_IDX_7
#define TX_PHASE_C_GPIO_PIN EXT_GPIO_IDX_11

/* polyphase initialization */
static td_void app_phase_gpio_init(td_void)
{
    uapi_io_set_func(RX_PHASE_A_GPIO_PIN, 0);
    uapi_io_set_func(RX_PHASE_B_GPIO_PIN, 0);
    uapi_io_set_func(RX_PHASE_C_GPIO_PIN, 0);
    uapi_io_set_func(TX_PHASE_A_GPIO_PIN, 0);
    uapi_io_set_func(TX_PHASE_B_GPIO_PIN, 0);
    uapi_io_set_func(TX_PHASE_C_GPIO_PIN, 0);

    uapi_io_set_dir(RX_PHASE_A_GPIO_PIN, EXT_GPIO_DIRECTION_OUT);
    uapi_io_set_dir(RX_PHASE_B_GPIO_PIN, EXT_GPIO_DIRECTION_OUT);
    uapi_io_set_dir(RX_PHASE_C_GPIO_PIN, EXT_GPIO_DIRECTION_OUT);
    uapi_io_set_dir(TX_PHASE_A_GPIO_PIN, EXT_GPIO_DIRECTION_OUT);
    uapi_io_set_dir(TX_PHASE_B_GPIO_PIN, EXT_GPIO_DIRECTION_OUT);
    uapi_io_set_dir(TX_PHASE_C_GPIO_PIN, EXT_GPIO_DIRECTION_OUT);
}

/*
 * polyphase TX path configuration, phase_name represents the phase name(PHASE_A、PHASE_B、PHASE_C)
 * phase_mode represents work mode(PHASE_MODE_OFF、PHASE_MODE_ON)
 */
__isr td_void app_set_tx_single_phase(td_u32 phase_name, td_u8 phase_mode)
{
    ext_gpio_value_e gpio_val;

    gpio_val = (ext_gpio_value_e)((PHASE_MODE_ON == phase_mode) ? EXT_GPIO_VALUE1 : EXT_GPIO_VALUE0);

    switch (phase_name) {
        case PHASE_A:
            uapi_io_set_ouput_val(TX_PHASE_A_GPIO_PIN, gpio_val);
            break;

        case PHASE_B:
            uapi_io_set_ouput_val(TX_PHASE_B_GPIO_PIN, gpio_val);
            break;

        case PHASE_C:
            uapi_io_set_ouput_val(TX_PHASE_C_GPIO_PIN, gpio_val);
            break;

        default:
            uapi_io_set_ouput_val(TX_PHASE_A_GPIO_PIN, gpio_val);
    }
}

/*
 * polyphase TX path configuration, phase_name represents the
 * phase name(PHASE_A、PHASE_B、PHASE_C)(PHASE_A、PHASE_B、PHASE_C)
 * phase_mode represents work mode(PHASE_MODE_OFF、PHASE_MODE_ON)
 */
__isr td_void app_set_rx_single_phase(td_u32 phase_name, td_u8 phase_mode)
{
    ext_gpio_value_e gpio_val;

    gpio_val = (ext_gpio_value_e)((PHASE_MODE_ON == phase_mode) ? EXT_GPIO_VALUE1 : EXT_GPIO_VALUE0);

    switch (phase_name) {
        case PHASE_A:
            uapi_io_set_ouput_val(RX_PHASE_A_GPIO_PIN, gpio_val);
            break;

        case PHASE_B:
            uapi_io_set_ouput_val(RX_PHASE_B_GPIO_PIN, gpio_val);
            break;

        case PHASE_C:
            uapi_io_set_ouput_val(RX_PHASE_C_GPIO_PIN, gpio_val);
            break;

        default:
            uapi_io_set_ouput_val(RX_PHASE_A_GPIO_PIN, gpio_val);
    }
}

td_void app_phase_init(td_void)
{
    uapi_register_phase_func(app_phase_gpio_init, app_set_tx_single_phase, app_set_rx_single_phase);
}

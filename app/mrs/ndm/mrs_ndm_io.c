/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS NDM I/O.
 */

#include "mrs_ndm_io.h"
#include <soc_mdm.h>

#if defined(PRODUCT_CFG_PRODUCT_TYPE_NDM)
#ifdef __cplusplus
extern "C" {
#endif

/* forbid to call dynamic domain content */
__hot td_void app_io_init(td_void)
{
    td_u32 io_dir = 0x003FEE3F;
    td_u32 io_val = 0x00006600;

    /* LED */
    uapi_io_set_func(EXT_GPIO_IDX_2, 1); /* LED_RX */
    uapi_io_set_func(EXT_GPIO_IDX_3, 1); /* LED_TX */
    uapi_led_init(EXT_LED_MODE_HIGH);

    /* UART0 */
    uapi_io_set_func(EXT_GPIO_IDX_9, 1);  /* GPIO_9_SEL:  UART0_RXD */
    uapi_io_set_func(EXT_GPIO_IDX_10, 3); /* GPIO_10_SEL: UART0_N_TXD, value:3 */

    /* LDRVR_PD */
    uapi_io_set_func(EXT_GPIO_IDX_5, 0);

    uapi_io_init(&io_val, &io_dir);
}

#ifdef __cplusplus
}
#endif
#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_NDM) */

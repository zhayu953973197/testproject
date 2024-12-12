/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS sta user demo api.
 */

#include "mrs_sta_user_demo_api.h"

#if (defined(PRODUCT_CFG_PRODUCT_TYPE_STA) && defined(MRS_USR_DEMO))
void sync_param_cb(void)
{
}

void macadd_get_send(void)
{
}

void event_value_init(void)
{
}

void hx_eventout_trigger(void)
{
}

void led_callback(void)
{
}

void iot_sg_sta_hx_evt_plc_ack_handle(uint8_t result)
{
    ext_unref_param(result);
}

uint32_t iot_sg_sta_hx_uart_rec_handle(uint8_t *data, uint16_t len, uint8_t type,
                                       mrs_sta_usr_frame_head *head)
{
    ext_unref_param(data);
    ext_unref_param(len);
    ext_unref_param(type);
    ext_unref_param(head);
    return EXT_ERR_SUCCESS;
}

uint32_t iot_sg_sta_hx_mr_plc_rec_handle(uint8_t *data, uint16_t len, uint8_t ack_req,
                                         mrs_sta_usr_frame_head *head)
{
    ext_unref_param(data);
    ext_unref_param(len);
    ext_unref_param(ack_req);
    ext_unref_param(head);
    return EXT_ERR_SUCCESS;
}

#endif /* (defined(PRODUCT_CFG_PRODUCT_TYPE_STA) && defined(MRS_USR_DEMO)) */

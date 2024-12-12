/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS sta user demo api.
 */

#ifndef __MRS_STA_USER_DEMO_API_H__
#define __MRS_STA_USER_DEMO_API_H__

#include "mrs_sta_usr_tx.h"

#if (defined(PRODUCT_CFG_PRODUCT_TYPE_STA) && defined(MRS_USR_DEMO))
#ifdef __cplusplus
extern "C" {
#endif

typedef td_u8 uint8_t;
typedef td_u16 uint16_t;
typedef td_u32 uint32_t;

void sync_param_cb(void);
void macadd_get_send(void);
void event_value_init(void); /* event struct initialization */
extern void hx_eventout_trigger(void);
extern void led_callback(void);
extern void iot_sg_sta_hx_evt_plc_ack_handle(uint8_t result);
extern uint32_t iot_sg_sta_hx_uart_rec_handle(uint8_t *data, uint16_t len, uint8_t type,
                                              mrs_sta_usr_frame_head *head);
extern uint32_t iot_sg_sta_hx_mr_plc_rec_handle(uint8_t *data, uint16_t len, uint8_t ack_req,
                                                mrs_sta_usr_frame_head *head);
#ifdef __cplusplus
}
#endif

#endif /* (defined(PRODUCT_CFG_PRODUCT_TYPE_STA) && defined(MRS_USR_DEMO)) */
#endif /* __MRS_STA_USER_DEMO_API_H__ */

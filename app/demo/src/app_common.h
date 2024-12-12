/*
 * Copyright (c) CompanyNameMagicTag 2018-2020. All rights reserved.
 * Description: APP common API.
 */

#ifndef __APP_COMMON_H__
#define __APP_COMMON_H__

#include "app_main.h"
#include "soc_mdm_msg.h"

#ifdef __cplusplus
extern "C" {
#endif


td_u32 app_timer_start(app_timer_id id, td_u32 period, ext_timer_type_e option);
td_u32 app_timer_stop(app_timer_id id);
td_u32 app_send_message(EXT_CONST ext_sys_queue_msg *msg);
td_u32 app_semaphore_wait(app_sem_id id, td_u32 timeout);
td_u32 app_semaphore_signal(app_sem_id id);
td_u8 app_tools_checksum8(EXT_CONST td_u8 *buffer, td_u16 length);

#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)
td_u32 app_set_mac_addr(td_u8 *mac, td_u32 mac_size);
#endif

#ifdef __cplusplus
}
#endif

#endif


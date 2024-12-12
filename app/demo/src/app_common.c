/*
 * Copyright (c) CompanyNameMagicTag 2018-2020. All rights reserved.
 * Description: 暂无描述
 */

#include "app_common.h"
#include <soc_mdm_mac.h>
#include "app_main.h"
#include "soc_mdm_sem.h"
#include "securec.h"
#include "stdio.h"
#include "soc_mdm_mem.h"

td_void app_timer_callback(td_u32 param);

__isr td_void app_timer_callback(td_u32 param)
{
    if (param < APP_TIMER_ID_MAX) {
        ext_sys_queue_msg msg;

        (td_void) memset_s(&msg, sizeof(msg), 0, sizeof(msg));
        msg.msg_id = APP_MSG_ID_TIMER;
        msg.param[0] = param;

        app_send_message(&msg);
    }
}

td_u32 app_timer_start(app_timer_id id, td_u32 period, ext_timer_type_e option)
{
    app_srv_context *ctx = app_get_srv_context();
    app_res_cfg *res = &ctx->res;

    if (id >= APP_TIMER_ID_MAX) {
        return EXT_ERR_INVALID_PARAMETER;
    }

    return uapi_timer_start(&res->timer_handle[id], app_timer_callback, period, option, id);
}

td_u32 app_timer_stop(app_timer_id id)
{
    app_srv_context *ctx = app_get_srv_context();
    app_res_cfg *res = &ctx->res;

    if (id >= APP_TIMER_ID_MAX) {
        return EXT_ERR_INVALID_PARAMETER;
    }

    return uapi_timer_stop(&res->timer_handle[id]);
}

__hot td_u32 app_send_message(EXT_CONST ext_sys_queue_msg *msg)
{
    app_srv_context *ctx = app_get_srv_context();
    app_res_cfg *res = &ctx->res;

    return uapi_msg_queue_send(res->queue_id, (td_pvoid)msg, 0, sizeof(ext_sys_queue_msg));
}

td_u32 app_semaphore_wait(app_sem_id id, td_u32 timeout)
{
    app_srv_context *ctx = app_get_srv_context();
    app_res_cfg *res = &ctx->res;

    if (id >= APP_SEM_ID_MAX) {
        return EXT_ERR_INVALID_PARAMETER;
    }

    return uapi_sem_wait(res->sem_id[id], timeout);
}

td_u32 app_semaphore_signal(app_sem_id id)
{
    app_srv_context *ctx = app_get_srv_context();
    app_res_cfg *res = &ctx->res;

    if (id >= APP_SEM_ID_MAX) {
        return EXT_ERR_INVALID_PARAMETER;
    }

    return uapi_sem_signal(res->sem_id[id]);
}

td_u8 app_tools_checksum8(EXT_CONST td_u8 *buffer, td_u16 length)
{
    td_u16 i = 0;
    td_u8 cs = 0;

    if ((buffer == TD_NULL) || (length == 0)) {
        return 0;
    }

    for (i = 0; i < length; i++) {
        cs += buffer[i];
    }

    return cs;
}

#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)
td_u32 app_set_mac_addr(EXT_CONST td_u8 *mac, td_u32 mac_size)
{
    td_u32 ret;
    td_u16 length = sizeof(ext_mac_attr) + sizeof(ext_mac_entry);
    ext_mac_attr *mac_attr = TD_NULL;

    if (mac == TD_NULL || mac_size != EXT_PLC_MAC_ADDR_LEN) {
        return EXT_ERR_FAILURE;
    }

    mac_attr = (ext_mac_attr *)uapi_malloc(EXT_MOD_ID_APP_COMMON, length);
    if (mac_attr == TD_NULL) {
        return EXT_ERR_MALLOC_FAILUE;
    }

    (td_void) memset_s(mac_attr, length, 0, length);
    mac_attr->list_num = 1;
    mac_attr->clear_flag = TD_TRUE;
    mac_attr->type = EXT_MDM_APP_MAC;
    if (memcpy_s(mac_attr->mac_list[0].mac, EXT_PLC_MAC_ADDR_LEN, mac, EXT_PLC_MAC_ADDR_LEN) != EOK) {
        uapi_free(EXT_MOD_ID_APP_COMMON, mac_attr);
        mac_attr = TD_NULL;
        return EXT_ERR_MEMCPY_FAIL;
    }

    ret = uapi_set_backup_mac_addr(mac_attr);

    uapi_free(EXT_MOD_ID_APP_COMMON, mac_attr);
    mac_attr = TD_NULL;

    return ret;
}
#endif


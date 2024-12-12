/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS STA startup handle.
 */

#include "mrs_sta_startup.h"
#include "mrs_sta_srv.h"
#include "mrs_sta_plc.h"
#include "mrs_sta_simu_meter.h"
#include "mrs_sta_io.h"
#include "mrs_common_tools.h"
#include "mrs_proto_645.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)

td_u8 g_startup_detect_retry = 0;
td_u8 g_startup_mr_frame[] = { 0x68, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x68, 0x13, 0x00, 0xdf, 0x16 };

/* processing for recieving reply */
td_void mrs_sta_startup_rx(EXT_CONST mrs_mr_queue_item *item, td_pbyte data, td_u16 data_len);
td_void mrs_sta_startup_detect(td_void)
{
    mrs_sta_srv_ctx *sta = mrs_sta_get_srv_ctx();
    mrs_mr_queue_item *item = TD_NULL;

    if (mrs_sta_get_status() != MRS_STA_STATUS_INIT) {
        return;
    }

    if (mrs_simu_meter_flag() == TD_TRUE) {
        return;
    }

    item = (mrs_mr_queue_item *)mrs_queue_top(&sta->mr_queue.mr_queue);
    if ((item != TD_NULL) && (item->id == MRS_STA_ITEM_ID_DETECT)) {
        mrs_sta_try_dequeue(&sta->mr_queue);
    }

    /* request for meter address */
    item = mrs_malloc(sizeof(mrs_mr_queue_item) + sizeof(g_startup_mr_frame));
    if (item == TD_NULL) {
        return;
    }

    (td_void) memset_s(item, sizeof(mrs_mr_queue_item) + sizeof(g_startup_mr_frame),
                       0, sizeof(mrs_mr_queue_item) + sizeof(g_startup_mr_frame));
    item->id = MRS_STA_ITEM_ID_DETECT;
    item->data_len = sizeof(g_startup_mr_frame);
    item->timeout = MRS_STARTUP_DETECT_TIMEOUT;
    item->rx_handle = mrs_sta_startup_rx;
    if (memcpy_s(item->data, item->data_len, g_startup_mr_frame, sizeof(g_startup_mr_frame)) != EOK) {
        mrs_free(item);
        return;
    }

    if (mrs_sta_try_enqueue(&sta->mr_queue, item) != TD_TRUE) {
        mrs_free(item);
        return;
    }

    mrs_sta_queue_notify();
    mrs_timer_start(MRS_TIMER_ID_STA_INIT, MRS_STARTUP_DETECT_INTERVAL, EXT_TIMER_TYPE_ONCE);
}

td_void mrs_sta_startup_rx(EXT_CONST mrs_mr_queue_item *item, td_pbyte data, td_u16 data_len)
{
    mrs_proto_645_frame_inf frame;
    mrs_sta_srv_ctx *sta = mrs_sta_get_srv_ctx();

    ext_unref_param(item);

    (td_void) memset_s(&frame, sizeof(frame), 0, sizeof(frame));
    if (mrs_proto_645_decode_frame(data, data_len, &frame) != EXT_ERR_SUCCESS) {
        return;
    }

    if (((frame.ctrl & 0x1f) != 0x13) || (frame.data_len != MRS_METER_ADDR_LEN)) {
        return;
    }

    mrs_timer_stop(MRS_TIMER_ID_STA_INIT);
    mrs_sta_set_status(MRS_STA_STATUS_MR);
    mrs_sta_led_offline();

    if (memcpy_s(sta->meter, sizeof(sta->meter), frame.data, frame.data_len) != EOK) {
        return;
    }

    mrs_proto_645_data_decode(sta->meter, sizeof(sta->meter));
    sta->protocol = METER_PROTO_645_2007;
    sta->baudrate = MRS_STA_DEFAULT_BAUDRATE;

    mrs_printf("Set meter addr: %02X%02X%02X%02X%02X%02X\n",
               sta->meter[5], sta->meter[4],  /* index 5&4 */
               sta->meter[3], sta->meter[2],  /* index 3&2 */
               sta->meter[1], sta->meter[0]); /* index 1&0 */

    /* set address */
    mrs_convert_meter_to_mac(sta->meter, sizeof(sta->meter), sta->mac, sizeof(sta->mac));
    mrs_sta_set_mac_addr(sta->mac);
}

#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_STA) */


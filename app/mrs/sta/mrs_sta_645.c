/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS sta 645 handle.
 */

#include "mrs_sta_645.h"
#include "mrs_sta_srv.h"
#include "mrs_msg.h"
#include "mrs_proto_645.h"
#include "mrs_common_tools.h"
#include "mrs_dfx.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)
/* processing for 645 frame recieving from serial ports */
td_void mrs_msg_on_app_frame_rx_645(EXT_CONST mrs_queue_msg *msg)
{
    td_pbyte data = (td_pbyte)msg->param1;
    td_u16 length = msg->param0;
    mrs_sta_srv_ctx *sta = mrs_sta_get_srv_ctx();
    mrs_mr_queue_item *item = (mrs_mr_queue_item *)mrs_queue_top(&sta->mr_queue.mr_queue);

    mrs_dfx_uart_chl_rx(data, length, TD_TRUE);
    mrs_dfx_uart_chl_rx_frame(length, EXT_ERR_SUCCESS, TD_TRUE);

    if (item == TD_NULL) {
        mrs_free(data);
        return;
    }

    if (item->rx_handle != TD_NULL) {
        item->rx_handle(item, data, length);
    }

    mrs_timer_stop(MRS_TIMER_ID_STA_TTL);
    mrs_free(data);

    mrs_sta_try_dequeue(&sta->mr_queue);
}

td_bool mrs_proto_is_rcv_p2p_pkt_by_dest_mac(EXT_CONST td_u8 *dest_mac, td_u8 mac_len)
{
    if (dest_mac == TD_NULL || mac_len != EXT_PLC_MAC_ADDR_LEN) {
        return TD_FALSE;
    }

    /* defination */
    return mrs_is_wild_mac(dest_mac);
}

#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_STA) */

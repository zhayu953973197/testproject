/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS sta service (user).
 */

#include "mrs_sta_usr.h"
#if (defined(PRODUCT_CFG_PRODUCT_TYPE_STA) && defined(MRS_USR_DEMO))
#include "mrs_sta_usr_tx.h"
#include "mrs_sta_user_demo_api.h"
#include "mrs_sta_evt.h"
#include "mrs_sta_plc.h"
#include "mrs_sta_srv.h"
#include "mrs_proto.h"
#include "mrs_common_tools.h"


#define MRS_STA_USR_FRAME_INTERVAL 70 /* Frame interval: 70 ms. */

td_void mrs_proto_usr_dispatch(mrs_proto_buffer *buf);
td_void mrs_proto_usr_rx_notify(mrs_proto_buffer *buf);
td_void mrs_sta_usr_proto_on_timer(td_void);
td_void mrs_sta_usr_startup_detect(td_void);
td_void mrs_sta_usr_led_ctrl(td_void);

td_void mrs_sta_usr_evt_query(td_void);
td_void mrs_sta_usr_evt_ack_notify(td_void);

td_void mrs_sta_usr_xr_plc_rx(mrs_plc_frame_data *plc);
td_void mrs_sta_usr_pr_plc_rx(mrs_plc_frame_data *plc);
td_void mrs_sta_usr_bc_plc_rx(mrs_plc_frame_data *plc);
td_void mrs_sta_usr_ciu_plc_rx(mrs_plc_frame_data *plc);

td_u32 mrs_sta_usr_srv_init(td_void)
{
    mrs_proto_usr_init();
    mrs_sta_register_on_timer_func(MRS_TIMER_ID_UART, mrs_sta_usr_proto_on_timer);
    mrs_sta_register_on_timer_func(MRS_TIMER_ID_STA_INIT, mrs_sta_usr_startup_detect);
    mrs_sta_register_on_timer_func(MRS_TIMER_ID_USR_1SEC, mrs_sta_usr_startup_detect);
    mrs_sta_register_on_timer_func(MRS_TIMER_ID_USR_LED, mrs_sta_usr_led_ctrl);
    mrs_sta_register_on_timer_func(MRS_TIMER_ID_USR_QUEUE, mrs_sta_usr_tx_unlock);
    mrs_sta_register_on_timer_func(MRS_TIMER_ID_USR_TX, mrs_sta_usr_tx_handle);

    mrs_timer_start(MRS_TIMER_ID_USR_LED, 100, EXT_TIMER_TYPE_ONCE); /* 100 ms */

    mrs_sta_event_register_query_notify(mrs_sta_usr_evt_query);
    mrs_sta_event_register_plc_ack_notify(mrs_sta_usr_evt_ack_notify);
    mrs_sta_event_register_trigger_notify(hx_eventout_trigger);

    event_value_init();

    mrs_sta_register_plc_rx_xr_notify(mrs_sta_usr_xr_plc_rx);
    mrs_sta_register_plc_rx_pr_notify(mrs_sta_usr_pr_plc_rx);
    mrs_sta_register_plc_rx_bc_notify(mrs_sta_usr_bc_plc_rx);
    mrs_sta_register_plc_rx_ciu_notify(mrs_sta_usr_ciu_plc_rx);

    return EXT_ERR_SUCCESS;
}

td_void mrs_sta_usr_startup_detect(td_void)
{
    sync_param_cb();
    mrs_timer_start(MRS_TIMER_ID_USR_1SEC, MRS_STA_USR_STARTUP_TIME, EXT_TIMER_TYPE_ONCE);
}

td_void mrs_sta_usr_led_ctrl(td_void)
{
    led_callback();
}

td_void mrs_sta_usr_xr_plc_rx(mrs_plc_frame_data *plc)
{
    /* transparent read meter service */
    mrs_sta_usr_frame_head frame_head;
    mrs_data_transmit_xr *xr = (mrs_data_transmit_xr *)plc->payload;
    errno_t err;

    if ((xr->data_len == 0) || (xr->stru_size < sizeof(mrs_data_transmit_xr)) ||
        (xr->stru_size + xr->data_len != plc->payload_len)) {
        return;
    }

    frame_head.id = plc->id;
    frame_head.protocol = (td_u8)xr->protocol;
    frame_head.seq = xr->seq;
    frame_head.timeout = xr->timeout;
    err = memcpy_s(frame_head.addr, MRS_METER_ADDR_LEN, plc->addr, MRS_METER_ADDR_LEN);
    if (err != EOK) {
        return;
    }

    if (mrs_sta_usr_plc_filter(&frame_head) == TD_TRUE) {
        /* Duplicate frames */
        return;
    }

    iot_sg_sta_hx_mr_plc_rec_handle((uint8_t *)xr->data, (uint16_t)xr->data_len, 1, &frame_head);
}

td_void mrs_sta_usr_pr_plc_rx(mrs_plc_frame_data *plc)
{
    /* parallel read meter service */
    mrs_sta_usr_frame_head frame_head;
    mrs_data_transmit_pr_dn *pr = (mrs_data_transmit_pr_dn *)plc->payload;
    errno_t err;

    if ((pr->data_len == 0) || (pr->stru_size < sizeof(mrs_data_transmit_pr_dn)) ||
        (pr->stru_size + pr->data_len != plc->payload_len)) {
        return;
    }

    frame_head.id = plc->id;
    frame_head.protocol = (td_u8)pr->protocol;
    frame_head.seq = pr->seq;
    frame_head.timeout = pr->timeout;
    err = memcpy_s(frame_head.addr, MRS_METER_ADDR_LEN, plc->addr, MRS_METER_ADDR_LEN);
    if (err != EOK) {
        return;
    }

    if (mrs_sta_usr_plc_filter(&frame_head) == TD_TRUE) {
        /* Duplicate frames */
        return;
    }

    iot_sg_sta_hx_mr_plc_rec_handle((uint8_t *)pr->data, (uint16_t)pr->data_len, 1, &frame_head);
}

td_void mrs_sta_usr_bc_plc_rx(mrs_plc_frame_data *plc)
{
    /* braodcast service */
    mrs_broadcast_frame *bc_frame = (mrs_broadcast_frame *)plc->payload;
    if ((plc->payload == TD_NULL) || (plc->payload_len < sizeof(mrs_broadcast_frame))) {
        return;
    }

    iot_sg_sta_hx_mr_plc_rec_handle((uint8_t *)plc->payload + bc_frame->stru_size, (uint16_t)bc_frame->data_len, 0,
                                    TD_NULL);
}

td_void mrs_sta_usr_evt_query(td_void)
{
    /* event pin trigger, send query meter event frame
     * refer to mrs_sta_evt.c
     * mrs_sta_create_event_report_plc: create plc
     * mrs_sta_send_event_report_plc: send plc */
}

td_void mrs_sta_usr_evt_ack_notify(td_void)
{
    /* recieved CCO reply of event report */
    iot_sg_sta_hx_evt_plc_ack_handle(0); /* 0 means event reported success */
}

td_void mrs_proto_usr_init(td_void)
{
    mrs_proto_handler handler = { mrs_proto_usr_rx_notify, (mrs_proto_rx_full)TD_NULL, mrs_proto_usr_dispatch };
    mrs_proto_register_handler(&handler);
    uapi_register_judge_is_rcv_p2p_pkt_by_dest_mac(mrs_proto_is_rcv_p2p_pkt_by_dest_mac);
}

td_void mrs_proto_usr_dispatch(mrs_proto_buffer *buf)
{
    mrs_usr_tx_item *item = mrs_sta_usr_get_tx_item();

    td_u32 ret = iot_sg_sta_hx_uart_rec_handle((uint8_t *)buf->buffer, (uint16_t)buf->rx_len, item->usr_id,
                                               &item->frame_head);
    if (ret == EXT_ERR_SUCCESS) {
        mrs_timer_stop(MRS_TIMER_ID_USR_QUEUE);
        mrs_usr_tx_dequeue();
    } else {
        mrs_printf("[ERR: %s, error:%d]\n", __FUNCTION__, ret);
    }

    /* CIU communication. Meter transmit data by self. */
}

td_void mrs_sta_usr_proto_on_timer(td_void)
{
    mrs_proto_parse(TD_TRUE);
}

td_void mrs_proto_usr_rx_notify(mrs_proto_buffer *buf)
{
    ext_unref_param(buf);
    mrs_timer_start(MRS_TIMER_ID_UART, MRS_STA_USR_FRAME_INTERVAL, EXT_TIMER_TYPE_ONCE);
}

td_void mrs_sta_usr_ciu_plc_rx(mrs_plc_frame_data *plc)
{
    /* processing for CIU frame */
    mrs_sta_usr_frame_head frame_head;
    mrs_ciu_transmit_inf *ciu_inf = (mrs_ciu_transmit_inf *)plc->payload;
    errno_t err;

    if ((ciu_inf->dir != MRS_DIR_CIU_TO_STA) || (ciu_inf->prm != MRS_PLC_PRM_HOST)) {
        return;
    }

    if ((ciu_inf->data_len == 0) || (ciu_inf->stru_size < sizeof(mrs_ciu_transmit_inf)) ||
        (ciu_inf->stru_size + ciu_inf->data_len != plc->payload_len)) {
        return;
    }

    uapi_app_received_p2p_pkt(plc->addr, sizeof(plc->addr));

    frame_head.id = plc->id;
    frame_head.protocol = (td_u8)METER_PROTO_645_2007;
    frame_head.seq = ciu_inf->seq;
    frame_head.timeout = MRS_CFG_METER_TIMEOUT;
    err = memcpy_s(frame_head.addr, sizeof(frame_head.addr), plc->addr, sizeof(plc->addr));
    if (err != EOK) {
        return;
    }

    /* CIU handler */
}

#endif /* (defined(PRODUCT_CFG_PRODUCT_TYPE_STA) && defined(MRS_USR_DEMO)) */

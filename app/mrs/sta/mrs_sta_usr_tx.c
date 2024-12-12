/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS sta uart sending list.
 */

#include "mrs_sta_usr_tx.h"
#include "mrs_common_tools.h"
#include "mrs_common_uart.h"
#include "mrs_common_queue.h"
#include "mrs_common_plc.h"

#if (defined(PRODUCT_CFG_PRODUCT_TYPE_STA) && defined(MRS_USR_DEMO))
#define mrs_sta_usr_tx_notify() mrs_msg_queue_send(MRS_MSG_ID_TIMER, 0, MRS_TIMER_ID_USR_TX, 0)

typedef struct {
    EXT_LIST mr_list;
    td_u16 item_num;
    td_u8 lock;
    td_u8 rsv;
} mrs_usr_tx_queue;

td_void mrs_usr_tx_clean(td_void);

mrs_usr_tx_queue *g_usr_tx_queue;

td_u32 mrs_sta_usr_tx_queue_init(td_void)
{
    if (g_usr_tx_queue == TD_NULL) {
        g_usr_tx_queue = (mrs_usr_tx_queue *)mrs_malloc(sizeof(mrs_usr_tx_queue));
        if (g_usr_tx_queue == TD_NULL) {
            return EXT_ERR_MALLOC_FAILUE;
        }
    }

    g_usr_tx_queue->item_num = 0;
    uapi_list_init(&g_usr_tx_queue->mr_list);
    mrs_lock_init(g_usr_tx_queue->lock);
    mrs_timer_stop(MRS_TIMER_ID_STA_QUEUE_LOCK);

    return EXT_ERR_SUCCESS;
}

td_bool mrs_sta_usr_tx(mrs_usr_tx_item *item)
{
    EXT_LIST *link = TD_NULL;
    mrs_usr_tx_queue *head = g_usr_tx_queue;

    if ((head == TD_NULL) || (item == TD_NULL)) {
        return TD_FALSE;
    }

    /* Remain the first one as the working one, leave it be */
    for (link = head->mr_list.prev; link != head->mr_list.next; link = link->prev) {
        mrs_usr_tx_item *temp = (mrs_usr_tx_item *)link;
        if (item->priority <= temp->priority) {
            break;
        }
    }

    item->link.prev = link;
    item->link.next = link->next;
    item->link.next->prev = &item->link;
    item->link.prev->next = &item->link;
    head->item_num++;

    mrs_sta_usr_tx_notify();
    mrs_printf("[xxx Items in Queue] %d\n", head->item_num);

    return TD_TRUE;
}

td_void mrs_sta_usr_tx_handle(td_void)
{
    td_u32 ret;
    mrs_usr_tx_item *item = TD_NULL;
    mrs_usr_tx_queue *head = g_usr_tx_queue;
    EXT_LIST *link = TD_NULL;
    td_u16 timeout = (td_u16)item->frame_head.timeout;

    if ((head == TD_NULL) || (head->item_num == 0)) {
        return;
    }

    if (mrs_try_lock(head->lock) == 0) {
        return;
    }

    mrs_timer_start(MRS_TIMER_ID_STA_QUEUE_LOCK, MRS_STA_MR_QUEUE_LOCK_TIME, EXT_TIMER_TYPE_ONCE);

    link = ext_list_first(&head->mr_list);
    item = (mrs_usr_tx_item *)link;

    if (item->retry > MRS_USR_TX_MAX_RETRY) {
        mrs_usr_tx_dequeue();
        return;
    }

    item->retry++;
    ret = mrs_uart_tx(item->data, item->data_len);
    if (ret != EXT_ERR_SUCCESS) {
        mrs_usr_tx_dequeue();
        return;
    }

    /* timeout == 0 means not need response.  by default ,timer equal to 10 ms */
    mrs_timer_start(MRS_TIMER_ID_USR_QUEUE, timeout * 1000, EXT_TIMER_TYPE_ONCE); /* time * 1000 */
}

td_void mrs_sta_usr_tx_unlock(td_void)
{
    if (g_usr_tx_queue == TD_NULL) {
        return;
    }

    /* Delete invalid ones */
    mrs_usr_tx_dequeue();
}

mrs_usr_tx_item *mrs_sta_usr_get_tx_item(td_void)
{
    EXT_LIST *link = TD_NULL;
    mrs_usr_tx_queue *head = g_usr_tx_queue;

    if (head == TD_NULL) {
        return TD_NULL;
    }

    if (head->item_num > 0) {
        link = ext_list_first(&head->mr_list);
        return (mrs_usr_tx_item *)link;
    }

    return TD_NULL;
}

td_void mrs_usr_tx_dequeue(td_void)
{
    mrs_usr_tx_queue *head = g_usr_tx_queue;

    if (head == TD_NULL) {
        return;
    }

    if (head->item_num > 0) {
        mrs_usr_tx_item *temp = (mrs_usr_tx_item *)ext_list_first(&head->mr_list);
        uapi_list_delete(&temp->link);
        mrs_free(temp);
        head->item_num--;
    }

    mrs_unlock(head->lock);
    mrs_timer_stop(MRS_TIMER_ID_STA_QUEUE_LOCK);

    mrs_sta_usr_tx_notify();
}

td_void mrs_usr_tx_clean(td_void)
{
    mrs_usr_tx_queue *head = g_usr_tx_queue;

    if (head == TD_NULL) {
        return;
    }

    while (head->item_num > 0) {
        mrs_usr_tx_item *temp = (mrs_usr_tx_item *)ext_list_first(&head->mr_list);
        uapi_list_delete(&temp->link);
        mrs_free(temp);
        head->item_num--;
    }
}

td_bool mrs_sta_usr_plc_filter(mrs_sta_usr_frame_head *head)
{
    mrs_usr_tx_queue *queue = g_usr_tx_queue;
    EXT_LIST *link = TD_NULL;

    if ((queue == TD_NULL) || (queue->item_num == 0)) {
        return TD_FALSE;
    }

    if (head == TD_NULL) {
        return TD_FALSE;
    }

    for (link = queue->mr_list.prev; link != &queue->mr_list; link = link->prev) {
        mrs_usr_tx_item *temp = (mrs_usr_tx_item *)link;
        if ((temp->frame_head.seq == head->seq) && (temp->frame_head.id == head->id) &&
            (memcmp(temp->frame_head.addr, head->addr, sizeof(head->addr)) == 0)) {
            return TD_TRUE;
        }
    }

    return TD_FALSE;
}

td_u32 mrs_sta_usr_xr_plc_tx(mrs_usr_tx_item *item, td_pbyte data, td_u16 data_len)
{
    td_u16 payload_len = sizeof(mrs_plc_frame_data) + sizeof(mrs_data_transmit_xr) + data_len;
    td_pbyte payload;
    mrs_plc_frame_data *plc = TD_NULL;
    mrs_data_transmit_xr *xr = TD_NULL;
    mrs_sta_usr_frame_head *head = TD_NULL;

    if ((item == TD_NULL) || (data == TD_NULL) || (data_len == 0)) {
        return EXT_ERR_INVALID_PARAMETER;
    }

    payload = mrs_malloc(payload_len);
    if (payload == TD_NULL) {
        return EXT_ERR_MALLOC_FAILUE;
    }

    (td_void) memset_s(payload, payload_len, 0, payload_len);
    plc = (mrs_plc_frame_data *)payload;
    head = &item->frame_head;
    (td_void) memcpy_s(plc->addr, sizeof(plc->addr), head->addr, sizeof(head->addr));
    plc->id = head->id;
    plc->payload_len = sizeof(mrs_data_transmit_xr) + data_len;
    plc->payload = payload + sizeof(mrs_plc_frame_data);

    xr = (mrs_data_transmit_xr *)plc->payload;
    xr->stru_ver = MRS_PLC_PROTO_VERSION;
    xr->stru_size = sizeof(mrs_data_transmit_xr);
    xr->dir = MRS_PLC_UP_FLG;
    xr->protocol = head->protocol;
    xr->seq = head->seq;
    xr->data_len = data_len;
    (td_void) memcpy_s(xr->data, xr->data_len, data, data_len);

    mrs_plc_frame_send(plc);
    mrs_free(payload);

    return EXT_ERR_SUCCESS;
}

td_u32 mrs_sta_usr_pr_plc_tx(mrs_usr_tx_item *item, td_pbyte data, td_u16 data_len)
{
    td_u16 payload_len = sizeof(mrs_plc_frame_data) + sizeof(mrs_data_transmit_pr_up) + data_len;
    td_pbyte payload;
    mrs_plc_frame_data *plc = TD_NULL;
    mrs_data_transmit_pr_up *pr = TD_NULL;
    mrs_sta_usr_frame_head *head = TD_NULL;

    if ((item == TD_NULL) || (data == TD_NULL) || (data_len == 0)) {
        return EXT_ERR_INVALID_PARAMETER;
    }

    payload = mrs_malloc(payload_len);
    if (payload == TD_NULL) {
        return EXT_ERR_MALLOC_FAILUE;
    }

    (td_void) memset_s(payload, payload_len, 0, payload_len);
    plc = (mrs_plc_frame_data *)payload;
    head = &item->frame_head;
    (td_void) memcpy_s(plc->addr, sizeof(plc->addr), head->addr, sizeof(head->addr));
    plc->id = head->id;
    plc->payload_len = sizeof(mrs_data_transmit_xr) + data_len;
    plc->payload = payload + sizeof(mrs_plc_frame_data);

    pr = (mrs_data_transmit_pr_up *)plc->payload;
    pr->stru_ver = MRS_PLC_PROTO_VERSION;
    pr->stru_size = sizeof(mrs_data_transmit_xr);
    pr->protocol = head->protocol;
    pr->seq = head->seq;
    pr->data_len = data_len;
    (td_void) memcpy_s(pr->data, pr->data_len, data, data_len);

    mrs_plc_frame_send(plc);
    mrs_free(payload);

    return EXT_ERR_SUCCESS;
}

td_u32 mrs_sta_usr_ciu_plc_tx(mrs_usr_tx_item *item, td_pbyte data, td_u16 data_len)
{
    mrs_ciu_transmit_inf *inf = TD_NULL;
    ext_p2p_mac_frame_ctrl ctrl;
    errno_t err;
    td_u16 payload_len = sizeof(mrs_ciu_transmit_inf) + data_len;
    mrs_sta_usr_frame_head *head = &item->frame_head;

    err = memset_s(&ctrl, sizeof(ctrl), 0, sizeof(ctrl));
    if (err != EOK) {
        return EXT_ERR_FAILURE;
    }

    err = memcpy_s(ctrl.dest_mac, sizeof(ctrl.dest_mac), head->addr, sizeof(head->addr));
    err |= memcpy_s(ctrl.src_mac, sizeof(ctrl.src_mac), head->addr, sizeof(head->addr));
    if (err != EOK) {
        return EXT_ERR_FAILURE;
    }

    ctrl.lid = 3; /* priority 3 */
    ctrl.port_num = EXT_DMS_CHL_MAC_PORT_APP;
    ctrl.ctrl = EXT_DMS_CHL_TX_CTRL_FORCE;
    ctrl.id = PLC_CMD_ID_CIU_TRANSMIT;
    ctrl.data_size = payload_len;

    ctrl.data = (td_pbyte)mrs_malloc(payload_len);
    if (ctrl.data == TD_NULL) {
        return EXT_ERR_MALLOC_FAILURE;
    }

    inf = (mrs_ciu_transmit_inf *)ctrl.data;
    inf->stru_ver = MRS_PLC_PROTO_VERSION;
    inf->stru_size = sizeof(mrs_ciu_transmit_inf);
    inf->dir = MRS_DIR_STA_TO_CIU;
    inf->prm = MRS_PLC_PRM_SLAVE;
    inf->seq = head->seq;
    inf->data_len = data_len;
    err = memcpy_s(inf->data, inf->data_len, data, data_len);
    if (err != EOK) {
        mrs_free(ctrl.data);
        return EXT_ERR_FAILURE;
    }

    uapi_p2p_chl_tx(&ctrl, sizeof(ctrl));
    mrs_free(ctrl.data);

    return EXT_ERR_SUCCESS;
}

#endif /* (defined(PRODUCT_CFG_PRODUCT_TYPE_STA) && defined(MRS_USR_DEMO)) */

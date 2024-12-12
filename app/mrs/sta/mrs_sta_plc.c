/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS STA plc handle.
 */

#include "mrs_sta_plc.h"
#include "mrs_common_plc.h"
#include "mrs_common_tools.h"
#include "mrs_sta_io.h"
#include "mrs_sta_srv.h"
#include "mrs_msg.h"
#include "mrs_proto_645.h"
#include "mrs_sta_tf.h"
#include "app_nv.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)
typedef td_u32 (*dut_di_handler)(mrs_proto_645_frame_inf *dut_inf, td_u8 **frm_645, td_u16 *frm_645_len);

typedef struct {
    td_u32 di;
    dut_di_handler handler;
} dut_di_pair;

td_void mrs_sta_xr_plc_frame_rx_default(EXT_CONST mrs_plc_frame_data *plc);
td_void mrs_sta_pr_plc_frame_rx_default(EXT_CONST mrs_plc_frame_data *plc);
td_void mrs_sta_bc_plc_frame_rx_default(EXT_CONST mrs_plc_frame_data *plc);
td_void mrs_sta_ciu_plc_frame_rx_default(EXT_CONST mrs_plc_frame_data *plc);

static mrs_sta_plc_rx_callback g_sta_plc_rx_xr_notify = mrs_sta_xr_plc_frame_rx_default;
static mrs_sta_plc_rx_callback g_sta_plc_rx_pr_notify = mrs_sta_pr_plc_frame_rx_default;
static mrs_sta_plc_rx_callback g_sta_plc_rx_bc_notify = mrs_sta_bc_plc_frame_rx_default;
static mrs_sta_plc_rx_callback g_sta_plc_rx_ciu_notify = mrs_sta_ciu_plc_frame_rx_default;

/* transmit PLC frame */
td_void mrs_sta_xr_uart_rx(EXT_CONST mrs_mr_queue_item *item, td_pbyte data, td_u16 data_len);
td_void mrs_sta_pr_uart_rx(EXT_CONST mrs_mr_queue_item *item, td_pbyte data, td_u16 data_len);
td_void mrs_sta_ciu_uart_rx(EXT_CONST mrs_mr_queue_item *item, td_pbyte data, td_u16 data_len);

/* equip test */
td_u32 mrs_sta_dut_set_power_gain(mrs_proto_645_frame_inf *dut_inf, td_u8 **frm_645, td_u16 *frm_645_len);
td_u32 mrs_sta_dut_get_verion(mrs_proto_645_frame_inf *dut_inf, td_u8 **frm_645, td_u16 *frm_645_len);
td_u32 mrs_sta_dut_get_mac(mrs_proto_645_frame_inf *dut_inf, td_u8 **frm_645, td_u16 *frm_645_len);
td_u32 mrs_sta_dut_set_mac(mrs_proto_645_frame_inf *dut_inf, td_u8 **frm_645, td_u16 *frm_645_len);
mrs_dut_test_frame *mrs_sta_dut_create_reply_frame(const mrs_dut_test_frame *dut_frm, const td_u8 *data,
                                                   td_u16 len);

/* query station info */
td_u32 mrs_sta_qry_sw_version(td_u8 **payload, td_u8 *payload_len);
td_void mrs_sta_qry_clean(sta_qry_element *elements, td_u8 cnt);

td_void mrs_sta_register_plc_rx_xr_notify(mrs_sta_plc_rx_callback notify)
{
    g_sta_plc_rx_xr_notify = notify;
}

td_void mrs_sta_register_plc_rx_pr_notify(mrs_sta_plc_rx_callback notify)
{
    g_sta_plc_rx_pr_notify = notify;
}

td_void mrs_sta_register_plc_rx_bc_notify(mrs_sta_plc_rx_callback notify)
{
    g_sta_plc_rx_bc_notify = notify;
}

td_void mrs_sta_register_plc_rx_ciu_notify(mrs_sta_plc_rx_callback notify)
{
    g_sta_plc_rx_ciu_notify = notify;
}

td_void mrs_sta_xr_plc_frame_rx(EXT_CONST mrs_plc_frame_data *plc)
{
    if (g_sta_plc_rx_xr_notify != TD_NULL) {
        g_sta_plc_rx_xr_notify(plc);
    }
}

td_void mrs_sta_pr_plc_frame_rx(EXT_CONST mrs_plc_frame_data *plc)
{
    if (g_sta_plc_rx_pr_notify != TD_NULL) {
        g_sta_plc_rx_pr_notify(plc);
    }
}

td_void mrs_broadcast_plc_frame_rx(EXT_CONST mrs_plc_frame_data *plc)
{
    if (g_sta_plc_rx_bc_notify != TD_NULL) {
        g_sta_plc_rx_bc_notify(plc);
    }
}

td_void mrs_sta_ciu_plc_frame_rx(EXT_CONST mrs_plc_frame_data *plc)
{
    if (g_sta_plc_rx_ciu_notify != TD_NULL) {
        g_sta_plc_rx_ciu_notify(plc);
    }
}

td_u32 mrs_sta_xr_fill_item(mrs_mr_queue_item *item, EXT_CONST mrs_plc_frame_data *plc,
    EXT_CONST mrs_data_transmit_xr *xr)
{
    td_u32 err;

    item->id = plc->id;
    item->seq = xr->seq;
    item->protocol = (td_u8)xr->protocol;
    item->timeout = xr->timeout;
    item->try_max = 0;
    item->data_len = xr->data_len;
    item->rx_handle = mrs_sta_xr_uart_rx;

    err = (td_u32)memcpy_s(item->mac_addr, sizeof(item->mac_addr), plc->addr, sizeof(plc->addr));
    err |= (td_u32)memcpy_s(item->data, item->data_len, plc->payload + xr->stru_size, xr->data_len);
    if (err != EOK) {
        return EXT_ERR_FAILURE;
    }

    if (item->timeout == 0) {
        item->timeout = MRS_CFG_METER_TIMEOUT;
    }

    return EXT_ERR_SUCCESS;
}

td_void mrs_sta_xr_plc_frame_rx_default(EXT_CONST mrs_plc_frame_data *plc)
{
    mrs_data_transmit_xr *xr = (mrs_data_transmit_xr *)plc->payload;
    mrs_mr_queue_item *item = TD_NULL;
    td_u32 err;
    td_u16 length;
    td_pbyte payload;

    if (xr->dir != MRS_PLC_DN_FLG) {
        return;
    }

    if ((xr->data_len == 0) || (xr->stru_size < sizeof(mrs_data_transmit_xr)) ||
        (xr->stru_size + xr->data_len != plc->payload_len) || (xr->data_len > PLC_DATA_LEN_MAX)) {
        return;
    }

    length = sizeof(mrs_mr_queue_item) + xr->data_len;
    payload = (td_pbyte)mrs_malloc(length);
    if (payload == TD_NULL) {
        return;
    }

    err = (td_u32)memset_s(payload, length, 0, length);
    if (err != EOK) {
        mrs_free(payload);
        return;
    }

    item = (mrs_mr_queue_item *)payload;
    if (mrs_sta_xr_fill_item(item, plc, xr) != EXT_ERR_SUCCESS) {
        mrs_free(item);
        return;
    }

    if (mrs_sta_try_enqueue(mrs_sta_get_queue(), item) != TD_TRUE) {
        mrs_free(item);
        return;
    }

    mrs_sta_queue_notify();
}

td_u32 mrs_sta_pr_fill_item(mrs_mr_queue_item *item, EXT_CONST mrs_plc_frame_data *plc,
    EXT_CONST mrs_data_transmit_pr_dn *pr)
{
    td_u32 err;

    item->id = plc->id;
    item->seq = pr->seq;
    item->protocol = (td_u8)pr->protocol;
    item->timeout = pr->timeout;
    item->try_max = 0;
    item->data_len = pr->data_len;
    item->rx_handle = mrs_sta_pr_uart_rx;

    err = (td_u32)memcpy_s(item->mac_addr, sizeof(item->mac_addr), plc->addr, sizeof(plc->addr));
    err |= (td_u32)memcpy_s(item->data, item->data_len, plc->payload + pr->stru_size, pr->data_len);
    if (err != EOK) {
        return EXT_ERR_FAILURE;
    }

    if (item->timeout == 0) {
        item->timeout = MRS_CFG_METER_TIMEOUT;
    }
    return EXT_ERR_SUCCESS;
}
td_void mrs_sta_pr_plc_frame_rx_default(EXT_CONST mrs_plc_frame_data *plc)
{
    mrs_data_transmit_pr_dn *pr = (mrs_data_transmit_pr_dn *)plc->payload;
    mrs_mr_queue_item *item = TD_NULL;
    td_u32 err;
    td_u16 length;
    td_pbyte payload;

    if ((pr->data_len == 0) || (pr->stru_size < sizeof(mrs_data_transmit_pr_dn)) ||
        (pr->stru_size + pr->data_len != plc->payload_len) || (pr->data_len > PLC_DATA_LEN_MAX)) {
        return;
    }

    length = sizeof(mrs_mr_queue_item) + pr->data_len;
    payload = (td_pbyte)mrs_malloc(length);
    if (payload == TD_NULL) {
        return;
    }

    err = (td_u32)memset_s(payload, length, 0, length);
    if (err != EOK) {
        mrs_free(payload);
        return;
    }

    item = (mrs_mr_queue_item *)payload;
    if (mrs_sta_pr_fill_item(item, plc, pr) != EXT_ERR_SUCCESS) {
        mrs_free(item);
        return;
    }

    if (mrs_sta_try_enqueue(mrs_sta_get_queue(), item) != TD_TRUE) {
        mrs_free(item);
        return;
    }

    mrs_sta_queue_notify();
}

td_void mrs_sta_bc_plc_frame_rx_default(EXT_CONST mrs_plc_frame_data *plc)
{
    mrs_mr_queue_item *item = TD_NULL;
    mrs_broadcast_frame *bc_frame = TD_NULL;
    errno_t err;

    if (plc == TD_NULL) {
        return;
    }

    if ((plc->payload == TD_NULL) || (plc->payload_len < sizeof(mrs_broadcast_frame))) {
        return;
    }

    bc_frame = (mrs_broadcast_frame *)plc->payload;
    if ((plc->payload_len != bc_frame->stru_size + bc_frame->data_len)
        || (bc_frame->data_len > PLC_DATA_LEN_MAX)) {
        return;
    }

    item = (mrs_mr_queue_item *)mrs_malloc(sizeof(mrs_mr_queue_item) + bc_frame->data_len);
    if (item == TD_NULL) {
        return;
    }

    err = memset_s(item, sizeof(mrs_mr_queue_item) + bc_frame->data_len, 0,
                   sizeof(mrs_mr_queue_item) + bc_frame->data_len);
    if (err != EOK) {
        mrs_free(item);
        return;
    }

    item->id = plc->id;
    item->timeout = 1;
    item->data_len = bc_frame->data_len;

    err = memcpy_s(item->data, item->data_len, plc->payload + bc_frame->stru_size, bc_frame->data_len);
    if (err != EOK) {
        mrs_free(item);
        return;
    }

    if (mrs_sta_try_enqueue(mrs_sta_get_queue(), item) != TD_TRUE) {
        mrs_free(item);
        return;
    }

    mrs_sta_frame_buf_add(item, item->data, item->data_len);

    mrs_sta_queue_notify();
}

td_void mrs_msg_on_plc_chl(EXT_CONST mrs_queue_msg *msg)
{
    td_u8 status = (td_u8)msg->param1;
    mrs_sta_srv_ctx *sta = mrs_sta_get_srv_ctx();

    if (status == 0) {
        /* LED always lighting while not netted. Extinguishing for 100ms when recieving or transmitting. */
        sta->chl_status = TD_FALSE;
        mrs_sta_led_offline();
    } else {
        /* LED always extinct while netted. lighting for 100ms when recieving or transmitting. */
        sta->chl_status = TD_TRUE;
        mrs_sta_led_join_network();
    }

    mrs_sta_chl_notify_handle(sta->chl_status);
}

td_bool mrs_sta_plc_chl_status(td_void)
{
    mrs_sta_srv_ctx *sta = mrs_sta_get_srv_ctx();
    return sta->chl_status;
}

td_void mrs_sta_set_mac_addr(const td_u8 mac[EXT_METER_ADDR_LEN])
{
    td_u16 length = sizeof(ext_mac_attr) + sizeof(ext_mac_entry);
    ext_mac_attr *mac_attr = (ext_mac_attr *)mrs_malloc(length);
    td_u32 ret;
    app_dev_addr_inf nv;

    if (mac_attr == TD_NULL) {
        return;
    }

    if (mrs_sta_get_sync_mac_flag() == TD_TRUE) {
        mrs_free(mac_attr);
        return;
    }
    mrs_sta_set_sync_mac_flag(TD_TRUE);

    if (memset_s(&nv, sizeof(nv), 0, sizeof(nv)) != EOK) {
        mrs_free(mac_attr);
        return;
    }

    ret = uapi_nv_read(ID_NV_APP_DEV_ADDR_INF, &nv, sizeof(nv));
    if (ret == EXT_ERR_SUCCESS) {
        if ((memcmp(mac, nv.dev_addr, EXT_METER_ADDR_LEN) != 0)) {
            if (memcpy_s(nv.dev_addr, EXT_METER_ADDR_LEN, mac, EXT_METER_ADDR_LEN) != EOK) {
                mrs_free(mac_attr);
                return;
            }
            uapi_nv_write(ID_NV_APP_DEV_ADDR_INF, &nv, sizeof(nv));
            mrs_sta_tf_meter_change();
        }
    }

    if (memset_s(mac_attr, length, 0, length) != EOK) {
        mrs_free(mac_attr);
        return;
    }

    mac_attr->list_num = 1;
    mac_attr->clear_flag = TD_TRUE;
    mac_attr->type = EXT_MDM_APP_MAC;

    if (memcpy_s(mac_attr->mac_list[0].mac, sizeof(mac_attr->mac_list[0].mac), mac, EXT_PLC_MAC_ADDR_LEN) != EOK) {
        mrs_free(mac_attr);
        return;
    }

    uapi_set_backup_mac_addr(mac_attr);
    mrs_free(mac_attr);

    mrs_printf("Set MAC addr: %02X:%02X:%02X:%02X:%02X:%02X\n",
               mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]); /* print index 0/1/2/3/4/5 */
}

td_void mrs_sta_xr_uart_rx(EXT_CONST mrs_mr_queue_item *item, td_pbyte data, td_u16 data_len)
{
    td_u16 payload_len = sizeof(mrs_plc_frame_data) + sizeof(mrs_data_transmit_xr) + data_len;
    td_pbyte payload = mrs_malloc(payload_len);
    mrs_plc_frame_data *plc = TD_NULL;
    mrs_data_transmit_xr *xr = TD_NULL;
    errno_t err;

    if (payload == TD_NULL || item == TD_NULL) {
        return;
    }

    err = memset_s(payload, payload_len, 0, payload_len);
    if (err != EOK) {
        mrs_free(payload);
        return;
    }

    plc = (mrs_plc_frame_data *)payload;
    err = memcpy_s(plc->addr, sizeof(plc->addr), item->mac_addr, sizeof(item->mac_addr));
    if (err != EOK) {
        mrs_free(payload);
        return;
    }

    plc->id = item->id;
    plc->payload_len = sizeof(mrs_data_transmit_xr) + data_len;
    plc->payload = payload + sizeof(mrs_plc_frame_data);

    xr = (mrs_data_transmit_xr *)plc->payload;
    xr->stru_ver = MRS_PLC_PROTO_VERSION;
    xr->stru_size = sizeof(mrs_data_transmit_xr);
    xr->dir = MRS_PLC_UP_FLG;
    xr->protocol = mrs_sta_get_meter_protocol();
    xr->seq = item->seq;
    xr->data_len = data_len;

    err = memcpy_s(xr->data, xr->data_len, data, data_len);
    if (err != EOK) {
        mrs_free(payload);
        return;
    }

    mrs_sta_frame_buf_add(item, data, data_len);

    mrs_plc_frame_send(plc);
    mrs_free(payload);
}

td_void mrs_sta_pr_uart_rx(EXT_CONST mrs_mr_queue_item *item, td_pbyte data, td_u16 data_len)
{
    td_u16 payload_len = sizeof(mrs_plc_frame_data) + sizeof(mrs_data_transmit_pr_up) + data_len;
    td_pbyte payload = TD_NULL;
    mrs_plc_frame_data *plc = TD_NULL;
    mrs_data_transmit_pr_up *pr = TD_NULL;
    errno_t err;

    if (item == TD_NULL || data == TD_NULL) {
        return;
    }
    payload = mrs_malloc(payload_len);
    if (payload == TD_NULL) {
        return;
    }

    err = memset_s(payload, payload_len, 0, payload_len);
    if (err != EOK) {
        mrs_free(payload);
        return;
    }

    plc = (mrs_plc_frame_data *)payload;
    err = memcpy_s(plc->addr, sizeof(plc->addr), item->mac_addr, sizeof(item->mac_addr));
    if (err != EOK) {
        mrs_free(plc);
        return;
    }

    plc->id = item->id;
    plc->payload_len = sizeof(mrs_data_transmit_xr) + data_len;
    plc->payload = payload + sizeof(mrs_plc_frame_data);

    pr = (mrs_data_transmit_pr_up *)plc->payload;
    if (pr == TD_NULL) {
        mrs_free(plc);
        return;
    }
    pr->stru_ver = MRS_PLC_PROTO_VERSION;
    pr->stru_size = sizeof(mrs_data_transmit_xr);
    pr->protocol = mrs_sta_get_meter_protocol();
    pr->seq = item->seq;
    pr->data_len = data_len;

    err = memcpy_s(pr->data, pr->data_len, data, data_len);
    if (err != EOK) {
        mrs_free(plc);
        return;
    }

    mrs_sta_frame_buf_add(item, data, data_len);
    mrs_plc_frame_send(plc);
    mrs_free(payload);
}

td_u32 mrs_sta_ciu_fill_item(mrs_mr_queue_item *item, EXT_CONST mrs_plc_frame_data *plc,
    EXT_CONST mrs_ciu_transmit_inf *ciu_inf)
{
    td_u32 err;

    item->id = plc->id;
    item->seq = ciu_inf->seq;
    item->protocol = (td_u8)METER_PROTO_645_2007;
    item->timeout = MRS_CFG_METER_TIMEOUT;
    item->try_max = 0;
    item->data_len = ciu_inf->data_len;
    item->rx_handle = mrs_sta_ciu_uart_rx;

    err = (td_u32)memcpy_s(item->mac_addr, sizeof(item->mac_addr), plc->addr, sizeof(plc->addr));
    err |= (td_u32)memcpy_s(item->data, item->data_len, plc->payload + ciu_inf->stru_size, ciu_inf->data_len);
    if (err != EOK) {
        return EXT_ERR_FAILURE;
    }

    return EXT_ERR_SUCCESS;
}
/* CIU frame */
td_void mrs_sta_ciu_plc_frame_rx_default(EXT_CONST mrs_plc_frame_data *plc)
{
    mrs_ciu_transmit_inf *ciu_inf = TD_NULL;
    mrs_mr_queue_item *item = TD_NULL;
    td_u32 err;
    td_u16 length;
    td_pbyte payload;

    ciu_inf = (mrs_ciu_transmit_inf *)plc->payload;
    uapi_app_received_p2p_pkt((td_u8 *)plc->addr, sizeof(plc->addr));

    if ((ciu_inf->dir != MRS_DIR_CIU_TO_STA) || (ciu_inf->prm != MRS_PLC_PRM_HOST)) {
        return;
    }

    if ((ciu_inf->data_len == 0) || (ciu_inf->stru_size < sizeof(mrs_ciu_transmit_inf)) ||
        (ciu_inf->stru_size + ciu_inf->data_len != plc->payload_len) ||
        (ciu_inf->data_len > PLC_DATA_LEN_MAX)) {
        return;
    }

    length = (td_u16)sizeof(mrs_mr_queue_item) + ciu_inf->data_len;
    payload = (td_pbyte)mrs_malloc(length);
    if (payload == TD_NULL) {
        return;
    }

    err = (td_u32)memset_s(payload, length, 0, length);
    if (err != EOK) {
        mrs_free(payload);
        return;
    }

    item = (mrs_mr_queue_item *)payload;
    if (mrs_sta_ciu_fill_item(item, plc, ciu_inf) != EXT_ERR_SUCCESS) {
        mrs_free(item);
        return;
    }

    if (mrs_sta_try_enqueue(mrs_sta_get_queue(), item) != TD_TRUE) {
        mrs_free(item);
        return;
    }

    mrs_sta_queue_notify();
}

td_void mrs_sta_ciu_uart_rx(EXT_CONST mrs_mr_queue_item *item, td_pbyte data, td_u16 data_len)
{
    mrs_ciu_transmit_inf *inf = TD_NULL;
    ext_p2p_mac_frame_ctrl ctrl;
    td_u32 err;
    td_u16 payload_len = sizeof(mrs_ciu_transmit_inf) + data_len;

    err = (td_u32)memset_s(&ctrl, sizeof(ctrl), 0, sizeof(ctrl));
    if (err != EOK) {
        return;
    }

    /* Here, the STA uses the CIU to use the dynamic MAC address mode (that is, the CIU MAC address is the
     * same as that of the STA). If the CIU uses a fixed MAC address for communication, here ctrl->dest_mac
     * should fill in the MAC address currently being used by the CIU.
     */
    err = (td_u32)memcpy_s(ctrl.dest_mac, sizeof(ctrl.dest_mac), item->mac_addr, sizeof(item->mac_addr));
    err |= (td_u32)memcpy_s(ctrl.src_mac, sizeof(ctrl.src_mac), item->mac_addr, sizeof(item->mac_addr));
    if (err != EOK) {
        return;
    }

    ctrl.lid = 3; /* priority 3 */
    ctrl.port_num = EXT_DMS_CHL_MAC_PORT_APP;
    ctrl.ctrl = EXT_DMS_CHL_TX_CTRL_FORCE;
    ctrl.id = PLC_CMD_ID_CIU_TRANSMIT;
    ctrl.data_size = payload_len;

    ctrl.data = (td_pbyte)mrs_malloc(payload_len);
    if (ctrl.data == TD_NULL) {
        return;
    }

    inf = (mrs_ciu_transmit_inf *)ctrl.data;
    inf->stru_ver = MRS_PLC_PROTO_VERSION;
    inf->stru_size = sizeof(mrs_ciu_transmit_inf);
    inf->dir = MRS_DIR_STA_TO_CIU;
    inf->prm = MRS_PLC_PRM_SLAVE;
    inf->seq = item->seq;
    inf->data_len = data_len;

    err = (td_u32)memcpy_s(inf->data, inf->data_len, data, data_len);
    if (err != EOK) {
        mrs_free(ctrl.data);
        return;
    }

    uapi_p2p_chl_tx(&ctrl, sizeof(ctrl));
    mrs_free(ctrl.data);
}

static td_u32 mrs_sta_rx_payload(sta_qry_element *elements, const mrs_plc_frame_data *plc_rx,
    td_u16 *rsp_payload_len, td_u8 *count)
{
    td_u8 idx;
    td_u8 payload_len = *rsp_payload_len;
    td_u32 ret = EXT_ERR_SUCCESS;
    const mrs_query_sta_inf *inf = (mrs_query_sta_inf *)plc_rx->payload;

    for (idx = 0; idx < inf->element_cnt; idx++) {
        if (plc_rx->payload_len < (payload_len + 2 + elements[idx].len)) { /* 2: id 1B , len 1B */
            /* element number is incorrect */
            break;
        }

        if (inf->element[idx] == MRS_QUERY_SW_VERSION) {
            elements[idx].id = MRS_QUERY_SW_VERSION;
            ret = mrs_sta_qry_sw_version(&elements[idx].data, &elements[idx].len);
        } else {
            ret = EXT_ERR_BAD_DATA;
        }

        if (ret == EXT_ERR_SUCCESS) {
            payload_len += 2; /* 2: id 1B, len 1B */
            payload_len += elements[idx].len;
            (*count)++;
        }
    }
    *rsp_payload_len = payload_len;

    return ret;
}

static td_u32 mrs_sta_set_rsp_payload(td_u8 *payload, td_u16 payload_len, EXT_CONST sta_qry_element *elements,
    td_u8 element_cnt, td_u8 count)
{
    td_u32 position = 0;
    mrs_query_sta_inf *inf =  (mrs_query_sta_inf *)payload;
    td_u8 idx;
    td_u32 ret = EXT_ERR_SUCCESS;

    position += sizeof(mrs_query_sta_inf);
    inf->element_cnt = count;
    for (idx = 0; idx < element_cnt; idx++) {
        if (elements[idx].data != TD_NULL) {
            payload[position++] = elements[idx].id;
            payload[position++] = elements[idx].len;

            if (memcpy_s(payload + position, payload_len - position, elements[idx].data, elements[idx].len) != EOK) {
                ret = EXT_ERR_FAILURE;
                break;
            }
            position += elements[idx].len;
            ret = EXT_ERR_SUCCESS;
        }
    }

    return ret;
}

/* query station info */
td_void mrs_sta_qry_plc_frame_rx(const mrs_plc_frame_data *plc)
{
    mrs_query_sta_inf *inf = TD_NULL;
    td_u32 ret, err;
    mrs_plc_frame_data frm_plc;
    sta_qry_element *elements = TD_NULL;
    td_u8 *payload = TD_NULL;
    td_u16 payload_len = 0;
    td_u8 element_cnt;
    td_u8 cnt = 0;

    inf = (mrs_query_sta_inf *)plc->payload;
    element_cnt = inf->element_cnt;
    if ((element_cnt == 0) || (inf->stru_size < sizeof(mrs_query_sta_inf))) {
        return;
    }

    err = (td_u32)memset_s(&frm_plc, sizeof(mrs_plc_frame_data), 0, sizeof(mrs_plc_frame_data));
    err |= (td_u32)memcpy_s(frm_plc.addr, sizeof(frm_plc.addr), plc->addr, sizeof(plc->addr));
    if (err != EOK) {
        return;
    }

    elements = (sta_qry_element *)mrs_malloc(sizeof(sta_qry_element) * element_cnt);
    if (elements == TD_NULL) {
        return;
    }

    if (memset_s(elements, sizeof(sta_qry_element) * element_cnt, 0, sizeof(sta_qry_element) * element_cnt) != EOK) {
        mrs_free(elements);
        return;
    }
    ret = mrs_sta_rx_payload(elements, plc, &payload_len, &cnt);
    payload_len += sizeof(mrs_query_sta_inf);
    payload = (td_u8 *)mrs_malloc(payload_len);
    if (payload == TD_NULL) {
        mrs_sta_qry_clean(elements, element_cnt);
        return;
    }

    if (memcpy_s(payload, payload_len, inf, sizeof(mrs_query_sta_inf)) != EOK) {
        mrs_sta_qry_clean(elements, element_cnt);
        return;
    }

    ret = mrs_sta_set_rsp_payload(payload, payload_len, elements, element_cnt, cnt);
    mrs_sta_qry_clean(elements, element_cnt);

    if (ret == EXT_ERR_SUCCESS) {
        frm_plc.id = PLC_CMD_ID_QRY_STA_INF;
        frm_plc.payload = payload;
        frm_plc.payload_len = payload_len;

        mrs_plc_frame_send(&frm_plc);
    }

    mrs_free(payload);
}

td_void mrs_sta_qry_clean(sta_qry_element *elements, td_u8 cnt)
{
    td_u8 idx;

    if (elements == TD_NULL) {
        return;
    }

    for (idx = 0; idx < cnt; idx++) {
        if (elements[idx].data != TD_NULL) {
            mrs_free(elements[idx].data);
        }
    }

    mrs_free(elements);
}

td_u32 mrs_sta_qry_sw_version(td_u8 **payload, td_u8 *payload_len)
{
    td_u32 err;
    td_u8 *data = TD_NULL;
    td_u8 ver_len;

    if (payload == TD_NULL || payload_len == TD_NULL) {
        return EXT_ERR_FAILURE;
    }
    ver_len = (td_u8)strlen(MRS_SW_VERSION_STR);
    if ((ver_len > MRS_SW_VERSION_MAX) || (ver_len == 0)) {
        return EXT_ERR_FAILURE;
    }

    data = (td_u8 *)mrs_malloc(MRS_SW_VERSION_MAX);
    if (data == TD_NULL) {
        return EXT_ERR_MALLOC_FAILURE;
    }

    err = (td_u32)memset_s(data, MRS_SW_VERSION_MAX, 0, MRS_SW_VERSION_MAX);
    err |= (td_u32)memcpy_s(data, MRS_SW_VERSION_MAX, MRS_SW_VERSION_STR, ver_len);
    if (err != EOK) {
        mrs_free(data);
        return EXT_ERR_FAILURE;
    }

    *payload = data;
    *payload_len = MRS_SW_VERSION_MAX;

    return EXT_ERR_SUCCESS;
}

/* filter invalid frame */
td_u32 mrs_sta_dut_plc_frame_filter(const mrs_plc_frame_data *plc)
{
    mrs_dut_test_frame *dut_frm = (mrs_dut_test_frame *)plc->payload;
    if ((dut_frm->data_len == 0) || (dut_frm->stru_size < sizeof(mrs_dut_test_frame)) ||
        (dut_frm->stru_size + dut_frm->data_len != plc->payload_len)) {
        return EXT_ERR_FAILURE;
    }

    if ((dut_frm->dir != MRS_PLC_DN_FLG) || (dut_frm->cmd_id != 0)
        || (dut_frm->cmd_id > MRS_QRY_STA_VERSION) || (dut_frm->vendor != MRS_DUT_VENDOR)) {
        return EXT_ERR_FAILURE;
    }

    if (uapi_crc32(0, plc->payload + 8, (td_u32)(dut_frm->data_len + 8)) != dut_frm->crc) { /* shifts 8bytes */
        return EXT_ERR_FAILURE;
    }
    return EXT_ERR_SUCCESS;
}

td_void mrs_fill_p2p_frame_ctrl(ext_p2p_mac_frame_ctrl *ctrl)
{
    ctrl->lid = 3; /* priority 3 */
    ctrl->port_num = EXT_DMS_CHL_MAC_PORT_APP;
    ctrl->ctrl = EXT_DMS_CHL_TX_CTRL_FORCE;
    ctrl->id = PLC_CMD_ID_DUT_STA_TEST;
    uapi_get_my_mac(ctrl->src_mac, sizeof(ctrl->src_mac));
    /* Here, the STA uses the CIU to use the dynamic MAC address mode (that is, the CIU MAC address is the
     * same as that of the STA). If the CIU uses a fixed MAC address for communication, here ctrl->dest_mac
     * should fill in the MAC address currently being used by the CIU.
     */
    uapi_get_my_mac(ctrl->dest_mac, sizeof(ctrl->dest_mac));
}
td_u32 mrs_sta_dut_di_pair_handle(mrs_proto_645_frame_inf *dut_inf, td_u16 *frm_645_len, td_u8 **frm_645, td_u16 di)
{
    td_u32 i;
    dut_di_pair di_handlers[4] = { /* 4 */
        /* 4groups */
        { 0xE310, mrs_sta_dut_set_power_gain },
        { 0xE211, mrs_sta_dut_get_verion },
        { 0xE212, mrs_sta_dut_get_mac },
        { 0xE313, mrs_sta_dut_set_mac },
    };

    for (i = 0; i < ext_array_count(di_handlers); i++) {
        if (di_handlers[i].di == di) {
            return  di_handlers[i].handler(dut_inf, frm_645, frm_645_len);
        }
    }

    return EXT_ERR_FAILURE;
}
/* STA equip test frame */
td_void mrs_sta_dut_plc_frame_rx(EXT_CONST mrs_plc_frame_data *plc)
{
    mrs_dut_test_frame *dut_frm = (mrs_dut_test_frame *)plc->payload;
    td_u32 ret, err;
    td_u16 di = 0;
    td_u16 frm_645_len = 0;
    td_u8 *frm_645 = TD_NULL;
    ext_p2p_mac_frame_ctrl ctrl;
    mrs_proto_645_frame_inf dut_inf = { 0 };

    dut_frm = (mrs_dut_test_frame *)plc->payload;
    if (mrs_sta_dut_plc_frame_filter(plc) != EXT_ERR_SUCCESS) {
        return;
    }

    ret = mrs_proto_645_decode_frame((const td_pbyte)(plc->payload + dut_frm->stru_size), dut_frm->data_len,
                                     &dut_inf);
    if (ret != EXT_ERR_SUCCESS) {
        return;
    }

    uapi_app_received_p2p_pkt((td_u8 *)plc->addr, sizeof(plc->addr));
    mrs_proto_645_data_decode(dut_inf.data, dut_inf.data_len);
    err = (td_u32)memcpy_s(&di, sizeof(di), dut_inf.data, MRS_645_97_DATA_DI_SIZE);
    err |= (td_u32)memset_s(&ctrl, sizeof(ext_p2p_mac_frame_ctrl), 0, sizeof(ext_p2p_mac_frame_ctrl));
    if (err != EOK) {
        return;
    }

    if (mrs_sta_dut_di_pair_handle(&dut_inf, &frm_645_len, &frm_645, di) != EXT_ERR_SUCCESS) {
        return;
    }

    mrs_fill_p2p_frame_ctrl(&ctrl);
    dut_frm = mrs_sta_dut_create_reply_frame(dut_frm, frm_645, frm_645_len);
    if (dut_frm == TD_NULL) {
        return;
    }

    ctrl.data = (td_u8 *)dut_frm;
    ctrl.data_size = sizeof(mrs_dut_test_frame) + frm_645_len;

    uapi_p2p_chl_tx(&ctrl, sizeof(ctrl));
    mrs_free(ctrl.data);
}

td_u32 mrs_sta_dut_set_power_gain(mrs_proto_645_frame_inf *dut_inf, td_u8 **frm_645, td_u16 *frm_645_len)
{
    /* 68 00 00 00 00 00 00 68 07 04 43 16 00 00 00 16 */
    td_u32 ret;
    td_u8 addr[EXT_PLC_MAC_ADDR_LEN] = { 0 };
    td_u8 meter[MRS_METER_ADDR_LEN] = { 0 };
    td_u8 data[3] = { 0 }; /* data 3B */
    td_u8 power_step, afe_pga_gain;

    if (dut_inf->data_len != MRS_645_97_DATA_DI_SIZE + 2) { /* data length equal to 2+2 */
        return EXT_ERR_BAD_DATA;
    }

    power_step = dut_inf->data[MRS_645_97_DATA_DI_SIZE];
    afe_pga_gain = dut_inf->data[MRS_645_97_DATA_DI_SIZE + 1];
    if (uapi_get_seconds() > MRS_SET_POWER_GAIN_TIME) {
        return EXT_ERR_CFG_NOT_ALLOW;
    }

    uapi_get_my_mac(addr, sizeof(addr));
    mrs_convert_mac_to_meter(addr, sizeof(addr), meter, sizeof(meter));
    if (memcmp(dut_inf->addr, meter, sizeof(meter)) != 0) {
        return EXT_ERR_BAD_DATA;
    }

    data[0] = 0x10;
    data[1] = 0xE3;

    ret = uapi_set_power_step((td_s8)power_step);
    ret |= uapi_set_afe_pga_gain(afe_pga_gain);
    if (ret == EXT_ERR_SUCCESS) {
        data[2] = 0; /* index 2: 0 */
    } else {
        data[2] = 1; /* index 2; 1 */
    }

    dut_inf->ctrl = dut_inf->ctrl | 0x80;
    dut_inf->data = data;
    dut_inf->data_len = MRS_645_97_DATA_DI_SIZE + 1;
    ret = mrs_proto_645_create_frame(dut_inf, frm_645, frm_645_len);

    dut_inf->data = TD_NULL;

    return ret;
}

td_u32 mrs_sta_dut_get_verion(mrs_proto_645_frame_inf *dut_inf, td_u8 **frm_645, td_u16 *frm_645_len)
{
    /* 68 00 00 00 00 00 00 68 07 02 44 15 00 00 16 */
    td_u32 ret;
    errno_t err;
    td_u8 version[MRS_645_97_DATA_DI_SIZE + MRS_SW_VERSION_MAX] = { 0 };
    td_u8 addr[EXT_PLC_MAC_ADDR_LEN] = { 0 };
    td_u8 meter[MRS_METER_ADDR_LEN] = { 0 };
    td_u16 ver_len;

    if (dut_inf->data_len != MRS_645_97_DATA_DI_SIZE) {
        return EXT_ERR_BAD_DATA;
    }

    uapi_get_my_mac(addr, sizeof(addr));
    mrs_convert_mac_to_meter(addr, sizeof(addr), meter, sizeof(meter));
    if (((td_bool)mrs_is_wildcast_meter_addr(dut_inf->addr) != TD_TRUE) &&
        ((td_bool)memcmp(dut_inf->addr, meter, MRS_METER_ADDR_LEN) != 0)) {
        return EXT_ERR_BAD_DATA;
    }

    version[0] = 0x11;
    version[1] = 0xE2;

    ver_len = (td_u16)strlen(MRS_SW_VERSION_STR);
    if ((ver_len > MRS_SW_VERSION_MAX) || (ver_len == 0)) {
        return EXT_ERR_FAILURE;
    }

    err = memcpy_s(version + MRS_645_97_DATA_DI_SIZE, MRS_SW_VERSION_MAX, MRS_SW_VERSION_STR, ver_len);
    if (err != EOK) {
        return EXT_ERR_FAILURE;
    }

    dut_inf->ctrl = dut_inf->ctrl | 0x80;
    dut_inf->data = version;
    dut_inf->data_len = (td_u8)sizeof(version);
    ret = mrs_proto_645_create_frame(dut_inf, frm_645, frm_645_len);

    dut_inf->data = TD_NULL;

    return ret;
}

td_u32 mrs_sta_dut_get_mac(mrs_proto_645_frame_inf *dut_inf, td_u8 **frm_645, td_u16 *frm_645_len)
{
    td_u32 ret;
    td_u8 data[MRS_645_97_DATA_DI_SIZE + EXT_PLC_MAC_ADDR_LEN];
    td_u8 meter[MRS_METER_ADDR_LEN] = { 0 };

    if ((td_bool)mrs_is_product_relay() == TD_FALSE) {
        return EXT_ERR_NOT_SUPPORT;
    }

    data[0] = 0x12;
    data[1] = 0xE2;

    uapi_get_my_mac(data + 2, EXT_PLC_MAC_ADDR_LEN);                                  /* shifts 2 */
    mrs_convert_mac_to_meter(data + 2, EXT_PLC_MAC_ADDR_LEN, meter, sizeof(meter)); /* shifts 2 */
    if (((td_bool)mrs_is_wildcast_meter_addr(dut_inf->addr) != TD_TRUE) &&
        ((td_bool)memcmp(dut_inf->addr, meter, sizeof(meter)) != 0)) {
        return EXT_ERR_BAD_DATA;
    }

    dut_inf->ctrl = dut_inf->ctrl | 0x80;
    dut_inf->data = data;
    dut_inf->data_len = MRS_645_97_DATA_DI_SIZE + EXT_PLC_MAC_ADDR_LEN;

    ret = mrs_proto_645_create_frame(dut_inf, frm_645, frm_645_len);

    dut_inf->data = TD_NULL;

    return ret;
}

td_u32 mrs_sta_dut_set_mac(mrs_proto_645_frame_inf *dut_inf, td_u8 **frm_645, td_u16 *frm_645_len)
{
    td_u32 ret;
    td_u8 mac[EXT_PLC_MAC_ADDR_LEN] = { 0 };
    td_u8 meter[MRS_METER_ADDR_LEN] = { 0 };
    td_u8 data[3]; /* data 3B */

    if ((td_bool)mrs_is_product_relay() == TD_FALSE) {
        return EXT_ERR_NOT_SUPPORT;
    }

    if (dut_inf->data_len != MRS_645_97_DATA_DI_SIZE + MRS_645_FRAME_METERADD_LEN) {
        return EXT_ERR_BAD_DATA;
    }

    uapi_get_my_mac(mac, sizeof(mac));
    mrs_convert_mac_to_meter(mac, sizeof(mac), meter, sizeof(meter));
    if (memcmp(dut_inf->addr, meter, sizeof(meter)) != 0) {
        return EXT_ERR_BAD_DATA;
    }

    mrs_convert_meter_to_mac(dut_inf->data + MRS_645_97_DATA_DI_SIZE, EXT_PLC_MAC_ADDR_LEN, mac, sizeof(mac));
    mrs_sta_set_mac_addr(mac);

    data[0] = 0x13;
    data[1] = 0xE3;
    data[2] = 0; /* index 2: 0 */

    dut_inf->ctrl = dut_inf->ctrl | 0x80;
    dut_inf->data = data;
    dut_inf->data_len = MRS_645_97_DATA_DI_SIZE + 1;

    ret = mrs_proto_645_create_frame(dut_inf, frm_645, frm_645_len);

    dut_inf->data = TD_NULL;

    return ret;
}

mrs_dut_test_frame *mrs_sta_dut_create_reply_frame(const mrs_dut_test_frame *dut_frm, const td_u8 *data,
                                                   td_u16 len)
{
    mrs_dut_test_frame *dut_reply = TD_NULL;
    td_u32 err;
    td_u16 reply_len = sizeof(mrs_dut_test_frame) + len;

    dut_reply = (mrs_dut_test_frame *)mrs_malloc(reply_len);
    if (dut_reply == TD_NULL) {
        return TD_NULL;
    }

    err = (td_u32)memcpy_s(dut_reply, reply_len, dut_frm, sizeof(mrs_dut_test_frame));
    if (data) {
        err |= (td_u32)memcpy_s(dut_reply->data, len, data, len);
    }

    if (err != EOK) {
        mrs_free(dut_reply);
        return TD_NULL;
    }

    dut_reply->dir = MRS_PLC_UP_FLG;
    dut_reply->data_len = len;
    dut_reply->crc = uapi_crc32(0, (td_u8 *)dut_reply + 8, (td_u32)(dut_reply->data_len + 8)); /* shifts 8 */

    return dut_reply;
}

#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_STA) */


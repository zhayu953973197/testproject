/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: Simulate Meter handler.
 */

#include "mrs_sta_simu_meter.h"
#include "mrs_sta_srv.h"
#include "mrs_sta_plc.h"
#include "mrs_sta_io.h"
#include "mrs_msg.h"
#include "mrs_common_uart.h"
#include "mrs_common_tools.h"
#include "mrs_proto_645.h"
#include "mrs_dfx.h"
#include "app_nv.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)
typedef td_void (*simu_di_proc)(mrs_proto_645_frame_inf *frame, td_pbyte *out_buf, td_u16 *out_len);

typedef struct {
    td_u32 di;
    simu_di_proc proc;
} simu_di_pair;

typedef struct {
    td_bool simu_flag;
    td_u8 simu_evt_en;
    td_u8 meter[EXT_METER_ADDR_LEN];
} simu_meter_ctx;

/* Handlers */
td_void simu_di_05060001(mrs_proto_645_frame_inf *frame, td_pbyte *out_buf, td_u16 *out_len);
td_void simu_di_05060101(mrs_proto_645_frame_inf *frame, td_pbyte *out_buf, td_u16 *out_len);
td_void simu_di_00010000(mrs_proto_645_frame_inf *frame, td_pbyte *out_buf, td_u16 *out_len);
td_void simu_di_04001501(mrs_proto_645_frame_inf *frame, td_pbyte *out_buf, td_u16 *out_len);
td_void mrs_simu_meter_init(td_void);
td_u16 mrs_simu_meter_send(td_pbyte buffer, td_u16 length);

simu_meter_ctx g_simu_meter_ctx = { TD_FALSE, 0, { 0 } };

td_void mrs_simu_meter_enable(td_bool enable)
{
    if (enable == g_simu_meter_ctx.simu_flag) {
        return;
    }

    g_simu_meter_ctx.simu_flag = enable;
    if (enable == TD_TRUE) {
        mrs_simu_meter_init();
    }
}

td_bool mrs_simu_meter_flag(td_void)
{
    return g_simu_meter_ctx.simu_flag;
}

td_void mrs_simu_meter_init(td_void)
{
    mrs_sta_srv_ctx *sta = mrs_sta_get_srv_ctx();
    td_u8 mac[EXT_METER_ADDR_LEN] = { 0 };
    td_u32 l;
    td_u32 h;

    uapi_get_local_mac_addr(mac, sizeof(mac));
    h = ((td_u32)mac[2] * 1000) + mac[3]; /* index 2*1000 add index 3 */
    l = ((td_u32)mac[4] * 1000) + mac[5]; /* index 4*1000 add index 5 */

    mrs_convert_int_to_bcd(h, g_simu_meter_ctx.meter + 3, 3); /* shifts 3 */
    mrs_convert_int_to_bcd(l, g_simu_meter_ctx.meter, 3);     /* low 3bytes */

    (td_void) memcpy_s(sta->meter, sizeof(sta->meter), g_simu_meter_ctx.meter, sizeof(g_simu_meter_ctx.meter));
    sta->protocol = METER_PROTO_645_2007;
    sta->baudrate = MRS_STA_DEFAULT_BAUDRATE;

    mrs_convert_meter_to_mac(sta->meter, sizeof(sta->meter), sta->mac, sizeof(sta->mac));
    mrs_sta_set_mac_addr(sta->mac);
    mrs_uart_register_tx_callback(mrs_simu_meter_send);
    mrs_sta_led_offline();

    mrs_simu_meter_nv_detect();
}

td_void mrs_simu_meter_nv_detect(td_void)
{
    mrs_simu_meter_inf simu = { 0, 0, 0, 0 };

    if (uapi_nv_read(ID_NV_APP_SIMU_METER, &simu, sizeof(simu)) == EXT_ERR_SUCCESS) {
        if ((simu.simu_meter == 1) && (simu.simu_evt == 1)) {
            g_simu_meter_ctx.simu_evt_en = simu.simu_evt;
            mrs_timer_start(MRS_TIMER_ID_STA_IO_EVTOUT, simu.simu_evt_delay * 10000, /* time*10000 */
                            EXT_TIMER_TYPE_ONCE);
        }
    }
}

td_u16 mrs_simu_meter_send(td_pbyte buf, td_u16 len)
{
    td_u32 di, i;
    td_u16 start_pos = 0;
    td_u16 frame_len = 0;
    td_pbyte frame;
    /* 0x05060001, daily-freeze clock
         * 0x05060101, daily-freeze positive active ennergy data
         * 0x00010000, total positive active ennergy
         * 0x04001501, report event by self */
    simu_di_pair di_pair_tbl[] = { { 0x05060001, simu_di_05060001 }, { 0x05060101, simu_di_05060101 },
                                   { 0x00010000, simu_di_00010000 }, { 0x04001501, simu_di_04001501 }, };

    mrs_logbuf("[SIMU-Rx]: ", buf, len);

    if (mrs_proto_645_find_frame(buf, len, &start_pos, &frame_len) != EXT_ERR_SUCCESS) {
        return len;
    }

    frame = buf + start_pos;
    if (memcmp(g_simu_meter_ctx.meter, frame + MRS_645_FRAME_METERADD_OFFSET, MRS_METER_ADDR_LEN) != 0) {
        mrs_logbuf("meter addr: ", g_simu_meter_ctx.meter, sizeof(g_simu_meter_ctx.meter));
        mrs_logbuf("frame addr: ", (frame + MRS_645_FRAME_METERADD_OFFSET), MRS_METER_ADDR_LEN);
        return len;
    }

    if ((frame[MRS_645_FRAME_CTRL_OFFSET] != MRS_645_CTRL_READ_DATA) ||
        (frame[MRS_645_FRAME_LEN_OFFSET] != MRS_645_DATA_DI_SIZE)) {
        return len;
    }

    mrs_proto_645_data_decode(frame + MRS_645_FRAME_DATA_OFFSET, MRS_645_DATA_DI_SIZE);
    (td_void) memcpy_s(&di, sizeof(di), frame + MRS_645_FRAME_DATA_OFFSET, MRS_645_DATA_DI_SIZE);

    for (i = 0; i < ext_array_count(di_pair_tbl); i++) {
        if (di == di_pair_tbl[i].di) {
            mrs_proto_645_frame_inf param;
            td_pbyte out_buf = TD_NULL;
            td_u16 out_len = 0;

            if (memcpy_s(param.addr, sizeof(param.addr),
                         frame + MRS_645_FRAME_METERADD_OFFSET, MRS_METER_ADDR_LEN) != EOK) {
                break;
            }

            param.ctrl = frame[MRS_645_FRAME_CTRL_OFFSET] | 0x80;
            di_pair_tbl[i].proc(&param, &out_buf, &out_len);

            if (out_buf == TD_NULL) {
                break;
            }

            mrs_logbuf("[SIMU-Tx]: ", out_buf, out_len);
            mrs_dfx_uart_chl_rx(out_buf, out_len, TD_FALSE);
            if (mrs_msg_queue_send(MRS_MSG_ID_APP_FRAME_RX_645, out_len, (uintptr_t)out_buf, 0) != EXT_ERR_SUCCESS) {
                mrs_free(out_buf);
            }
            break;
        }
    }

    return len;
}

td_void simu_di_05060001(mrs_proto_645_frame_inf *frame, td_pbyte *out_buf, td_u16 *out_len)
{
    td_u8 data[9] = { 0x01, 0x00, 0x06, 0x05, 0x00, 0x00, 0x01, 0x03, 0x19 }; /* data 9B */

    frame->data_len = sizeof(data);
    frame->data = data;
    (td_void) mrs_proto_645_create_frame(frame, out_buf, out_len);

    frame->data = TD_NULL;
}

td_void simu_di_05060101(mrs_proto_645_frame_inf *frame, td_pbyte *out_buf, td_u16 *out_len)
{
    td_u8 data[24] = { 0x01, 0x01, 0x06, 0x05 }; /* data 24B */

    frame->data_len = sizeof(data);
    frame->data = data;
    (td_void) mrs_proto_645_create_frame(frame, out_buf, out_len);

    frame->data = TD_NULL;
}

td_void simu_di_00010000(mrs_proto_645_frame_inf *frame, td_pbyte *out_buf, td_u16 *out_len)
{
    td_u8 data[8] = { 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 }; /* data 8B */

    frame->data_len = sizeof(data);
    frame->data = data;
    (td_void) mrs_proto_645_create_frame(frame, out_buf, out_len);

    frame->data = TD_NULL;
}

td_void simu_di_04001501(mrs_proto_645_frame_inf *frame, td_pbyte *out_buf, td_u16 *out_len)
{
    td_u8 data[19] = { /* data 19B */
        0x01, 0x15, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0xaa, 0x01, 0xaa
    };

    if (g_simu_meter_ctx.simu_evt_en == 0) {
        out_buf = TD_NULL;
        return;
    }

    frame->data_len = sizeof(data);
    frame->data = data;
    (td_void) mrs_proto_645_create_frame(frame, out_buf, out_len);

    frame->data = TD_NULL;
}
#endif

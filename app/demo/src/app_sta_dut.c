/*
 * Copyright (c) CompanyNameMagicTag 2018-2020. All rights reserved.
 * Description: APP code for DUT test.
 */

#include "app_sta_dut.h"
#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)
#include "app_proto_645.h"
#include "app_main.h"
#include "app_common.h"
#include "app_uart.h"
#include "string.h"
#include "securec.h"
#include "soc_mdm_mem.h"
#include "soc_ft_nv.h"
#include "soc_mdm_phy.h"
#include "soc_mdm_nv.h"

/* frame datagram */
/* +----+-----------+----+----+----+-------------+------+----+----+ */
/* |head|address(6) |head|ctrl|len | (di 4)      | DATA | CS |tail| */
/* +----+-----------+----+----+----+-------------+------+----+----+ */
/* | 68 | a5 ... a0 | 68 | xx | xx | di3 ... di0 | DATA | CS | 16 | */
/* +----+-----------+----+----+----+-------------+------+----+----+ */
#define DI_FEFF1202 (0x31324535) /* set configuration. 'FEFF1202' + '33' */

#define DUT_POWER_STEP_SIZE (4)

typedef struct {
    td_u8 stru_ver;
    td_u8 stru_size;
    td_u8 dut_mac[EXT_PLC_MAC_ADDR_LEN];
    td_u8 mac[EXT_PLC_MAC_ADDR_LEN];
    td_s8 power_step;
    td_u8 afe_gain;
} dut_mac_wrt_req;

typedef struct {
    td_u8 stru_ver;
    td_u8 stru_size;
    td_u16 dut_mac_ret : 4;
    td_u16 mac_ret : 4;
    td_u16 power_reduce_ret : 4;
    td_u16 reserved : 4;
} dut_mac_wrt_ind;

td_u32 app_dut_feff1202(td_pbyte frame, td_u16 length);
td_u32 dut_set_mac_addr(td_u8 mac[EXT_PLC_MAC_ADDR_LEN]);
td_u32 dut_write_mac_2_nv(EXT_CONST td_u8 mac[EXT_PLC_MAC_ADDR_LEN]);
td_void dut_reply(td_u32 di, td_pbyte data, td_u8 data_length);

td_bool app_handle_dut_frame(EXT_CONST td_pbyte frame, td_u16 length)
{
    td_u32 ret;
    td_u16 start_pos = 0;
    td_u16 frame_length = 0;
    td_u8 dut_addr[] = { 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC };
    td_u32 di = 0;

    ret = app_find_frame_645((td_pbyte)frame, length, &start_pos, &frame_length);
    if (ret != EXT_ERR_SUCCESS) {
        return TD_FALSE;
    }

    /* check frame 645 length: the frame must contail 'DI' filed. */
    if (frame_length < PROTO_645_DATAGRAM_LEN_MIN + sizeof(di)) {
        return TD_FALSE;
    }

    /* check frame 645 address filed: \xCC\xCC\xCC\xCC\xCC\xCC */
    if (memcmp(dut_addr, frame + start_pos + 1, sizeof(dut_addr)) != 0) {
        /* Not dut frame. */
        return TD_FALSE;
    }

    /* process di */
    (td_void) memcpy_s(&di, sizeof(di), frame + start_pos + APP_DUT_DI_BYTE_OFFSET, APP_DUT_DI_LEN);
    if (di == DI_FEFF1202) {
        ret = app_dut_feff1202(frame + start_pos, frame_length);
        return (ret == EXT_ERR_SUCCESS) ? TD_TRUE : TD_FALSE;
    }

    return TD_FALSE;
}

td_u32 app_dut_feff1202(td_pbyte frame, td_u16 length)
{
    td_u32 ret;
    dut_mac_wrt_req *dut_mac_inf = TD_NULL;
    dut_mac_wrt_ind dut_ret;

    if (length < sizeof(td_u32) + sizeof(dut_mac_wrt_req) + PROTO_645_DATAGRAM_LEN_MIN) {
        return EXT_ERR_INVALID_PARAMETER;
    }

    dut_mac_inf = (dut_mac_wrt_req *)uapi_malloc(EXT_MOD_ID_APP_COMMON, sizeof(dut_mac_wrt_req));
    if (dut_mac_inf == TD_NULL) {
        return EXT_ERR_MALLOC_FAILUE;
    }

    (td_void) memcpy_s(dut_mac_inf, sizeof(dut_mac_wrt_req),
                       frame + APP_DUT_MAC_BYTE_OFFSET, sizeof(dut_mac_wrt_req));
    app_frame_645_decode((td_pbyte)dut_mac_inf, sizeof(dut_mac_wrt_req));

    if (length != sizeof(td_u32) + dut_mac_inf->stru_size + PROTO_645_DATAGRAM_LEN_MIN) {
        uapi_free(EXT_MOD_ID_APP_COMMON, dut_mac_inf);
        dut_mac_inf = TD_NULL;
        return EXT_ERR_INVALID_PARAMETER;
    }

    dut_ret.stru_ver = 0;
    dut_ret.stru_size = sizeof(dut_ret);
    dut_ret.dut_mac_ret = 0xF;
    dut_ret.mac_ret = 0xF;
    dut_ret.power_reduce_ret = 0xF;
    dut_ret.reserved = 0;

    do {
        app_timer_stop(APP_TIMER_ID_INIT);
        ret = app_set_mac_addr(dut_mac_inf->dut_mac, sizeof(dut_mac_inf->dut_mac));
        if (ret != EXT_ERR_SUCCESS) {
            break;
        }

        dut_ret.dut_mac_ret = 0;

        ret = dut_write_mac_2_nv(dut_mac_inf->mac);
        if (ret != EXT_ERR_SUCCESS) {
            break;
        }

        dut_ret.mac_ret = 0;

        uapi_set_power_step(dut_mac_inf->power_step);
        uapi_set_afe_pga_gain(dut_mac_inf->afe_gain);
        dut_ret.power_reduce_ret = 0;
    } while (0);

    uapi_free(EXT_MOD_ID_APP_COMMON, dut_mac_inf);
    dut_mac_inf = TD_NULL;

    dut_reply(0xfeff1202, (td_pbyte)&dut_ret, sizeof(dut_ret));

    return EXT_ERR_FAILURE;
}

td_void dut_reply(td_u32 di, td_pbyte data, td_u8 length)
{
    td_u8 data_length = sizeof(td_u32) + length;
    td_u8 payload_length = PROTO_645_DATAGRAM_LEN_MIN + data_length;
    td_u8 *payload = TD_NULL;
    td_u8 *buffer = TD_NULL;

    if (data == TD_NULL) {
        return;
    }
    payload = (td_u8 *)uapi_malloc(EXT_MOD_ID_APP_COMMON, payload_length + APP_DUT_PAYLOAD_LEN);
    if (payload == TD_NULL) {
        return;
    }
    buffer = payload + APP_DUT_PAYLOAD_LEN;
    payload[0] = APP_DUT_REPLY_DATA;
    payload[APP_DUT_PAYLOAD_INDEX1] = APP_DUT_REPLY_DATA;
    payload[APP_DUT_PAYLOAD_INDEX2] = APP_DUT_REPLY_DATA;
    payload[APP_DUT_PAYLOAD_INDEX3] = APP_DUT_REPLY_DATA;

    buffer[0] = PROTO_645_HEAD_BYTE;
    (td_void) memset_s(buffer + 1, payload_length - 1, APP_DUT_BUF_DATA1, APP_DUT_BUF_LEN1);
    buffer[APP_DUT_BUF_INDEX7] = PROTO_645_HEAD_BYTE;
    buffer[APP_DUT_BUF_INDEX8] = APP_DUT_BUF_INDEX8_DATA;
    buffer[APP_DUT_BUF_INDEX9] = data_length;
    (td_void) memcpy_s(buffer + APP_DUT_BUF_LEN2, payload_length - APP_DUT_BUF_LEN2, &di, sizeof(td_u32));
    (td_void) memcpy_s(buffer + APP_DUT_BUF_LEN3, payload_length - APP_DUT_BUF_LEN3, data, length);
    app_frame_645_encode(buffer + APP_DUT_BUF_LEN2, data_length);

    buffer[payload_length - APP_DUT_BUF_CHECKSUM_LEN] = app_tools_checksum8(buffer,
                                                                            payload_length - APP_DUT_BUF_CHECKSUM_LEN);
    buffer[payload_length - 1] = PROTO_645_TAIL_BYTE;

    app_uart_tx(payload, payload_length + APP_DUT_PAYLOAD_LEN);

    uapi_free(EXT_MOD_ID_APP_COMMON, payload);
    payload = TD_NULL;

    return;
}

td_u32 dut_write_mac_2_nv(EXT_CONST td_u8 mac[EXT_PLC_MAC_ADDR_LEN])
{
    ext_nv_ftm_product_id *nv = TD_NULL;
    td_u32 ret;

    if (mac == TD_NULL) {
        return EXT_ERR_FAILURE;
    }
    nv = (ext_nv_ftm_product_id *)uapi_malloc(EXT_MOD_ID_APP_COMMON, sizeof(ext_nv_ftm_product_id));
    if (nv == TD_NULL) {
        return EXT_ERR_MALLOC_FAILUE;
    }

    ret = uapi_factory_nv_read(EXT_NV_FTM_PRODUCT_ID, nv, sizeof(ext_nv_ftm_product_id));
    if (ret == EXT_ERR_SUCCESS) {
        if (memcpy_s(nv->plc_mac, EXT_PLC_MAC_ADDR_LEN, mac, EXT_PLC_MAC_ADDR_LEN) != EXT_ERR_SUCCESS) {
            uapi_free(EXT_MOD_ID_APP_COMMON, nv);
            return EXT_ERR_FAILURE;
        }
        ret = uapi_factory_nv_write(EXT_NV_FTM_PRODUCT_ID, nv, sizeof(ext_nv_ftm_product_id));
    }

    uapi_free(EXT_MOD_ID_APP_COMMON, nv);
    nv = TD_NULL;

    return ret;
}

#endif
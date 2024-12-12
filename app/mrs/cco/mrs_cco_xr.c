/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS cco mr - AFN=13H-F1.
 */

#include "mrs_cco_xr.h"
#include "mrs_common_plc.h"
#include "mrs_cco_mr_list.h"
#include "mrs_common_tools.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)

typedef td_void (*mrs_xr_fill_uart_frame)(td_pbyte data, td_u16 data_len, mr_task_node *node, td_pbyte buf,
                                          td_u16 buf_len);

/* reply 376.2 transparent read meter frame */
td_void mrs_xr_fill_frame_13_f1(td_pbyte data, td_u16 data_len, mr_task_node *node, td_pbyte buf, td_u16 buf_len);
td_void mrs_xr_fill_frame_13_f255(td_pbyte data, td_u16 data_len, mr_task_node *node, td_pbyte buf,
                                  td_u16 buf_len);
td_void mrs_xr_fill_frame_02_f1(td_pbyte data, td_u16 data_len, mr_task_node *node, td_pbyte buf, td_u16 buf_len);
td_void mrs_xr_fill_frame_02_f255(td_pbyte data, td_u16 data_len, mr_task_node *node, td_pbyte buf,
                                  td_u16 buf_len);

#define MRS_AFN_COMM_DELAY (1)

td_u32 mrs_cco_create_xr_plc_frame(mr_task_node *node, td_pbyte buffer, td_u16 length)
{
    mrs_plc_frame_data *new_frame = TD_NULL;
    mrs_data_transmit_xr *xr = TD_NULL;
    td_u16 payload_len = sizeof(mrs_plc_frame_data) + sizeof(mrs_data_transmit_xr) + length;
    td_pbyte payload = (td_pbyte)mrs_malloc(payload_len);
    if (payload == TD_NULL) {
        return EXT_ERR_MALLOC_FAILUE;
    }

    (td_void) memset_s(payload, payload_len, 0, payload_len);
    new_frame = (mrs_plc_frame_data *)payload;
    mrs_convert_meter_to_mac(node->dst_addr, sizeof(node->dst_addr), new_frame->addr, sizeof(new_frame->addr));
    new_frame->id = PLC_CMD_ID_XR;
    new_frame->payload_len = sizeof(mrs_data_transmit_xr) + length;
    new_frame->payload = payload + sizeof(mrs_plc_frame_data);

    xr = (mrs_data_transmit_xr *)new_frame->payload;
    xr->stru_ver = MRS_PLC_PROTO_VERSION;
    xr->stru_size = sizeof(mrs_data_transmit_xr);
    xr->protocol = node->protocol;
    xr->data_len = length;
    xr->seq = node->plc_seq;
    xr->timeout = MRS_CFG_METER_TIMEOUT;
    xr->dir = MRS_PLC_DN_FLG;
    if (xr->data_len > 0) {
        if (memcpy_s(xr->data, xr->data_len, buffer, length) != EOK) {
            mrs_free(payload);
            return EXT_ERR_MEMCPY_FAIL;
        }
    }
    node->plc_frame = new_frame;

    return EXT_ERR_SUCCESS;
}

td_u32 mrs_cco_create_xr_1376_2_frame(td_pbyte *out_buffer, td_u16 *out_len,
                                      mr_task_node *node, td_pbyte buffer, td_u16 length)
{
    mrs_proto_1376_2_encode encode;
    td_u32 ret;
    td_u16 data_length = length;
    td_u8 head_length[] = { 4,    5,    2,    3 };
    td_u8 afn_tbl[] = { 0x13, 0x13, 0x02, 0x02 };
    td_u8 tbl[] = { 0x01, 0xff, 0x01, 0xff };
    mrs_xr_fill_uart_frame fill_func[] = {
        mrs_xr_fill_frame_13_f1, mrs_xr_fill_frame_13_f255,
        mrs_xr_fill_frame_02_f1, mrs_xr_fill_frame_02_f255
    };

    if (node->option > MRS_XR_02H_F255) {
        return EXT_ERR_INVALID_PARAMETER;
    }

    data_length += head_length[node->option];

    (td_void) memset_s(&encode, sizeof(encode), 0, sizeof(encode));
    encode.length = data_length;
    encode.data = mrs_malloc(encode.length);
    if (encode.data == TD_NULL) {
        return EXT_ERR_MALLOC_FAILUE;
    }

    fill_func[node->option](encode.data, encode.length, node, buffer, length);

    encode.afn = afn_tbl[node->option];
    encode.fn = tbl[node->option];
    encode.seq = node->seq;
    encode.module_flag = 1;
    (td_void) memcpy_s(encode.src_addr, sizeof(encode.src_addr), node->dst_addr, sizeof(node->dst_addr));
    (td_void) memcpy_s(encode.dst_addr, sizeof(encode.dst_addr), node->src_addr, sizeof(node->src_addr));

    ret = mrs_proto_1376_2_create_frame(&encode, out_buffer, out_len);
    mrs_free(encode.data);
    return ret;
}

td_void mrs_xr_fill_frame_13_f1(td_pbyte data, td_u16 data_len, mr_task_node *node, td_pbyte buf, td_u16 buf_len)
{
    td_u16 offset = 0;
    td_u8 len = (td_u8)buf_len;

    if (data_len < len + 4) { /* 4: time 2B, protocol 1B, length 1B */
        return;
    }

    data[offset++] = MRS_AFN_COMM_DELAY;
    data[offset++] = 0;
    data[offset++] = node->protocol;
    data[offset++] = len;
    if (memcpy_s(data + offset, data_len - offset, buf, len) != EOK) {}
}

td_void mrs_xr_fill_frame_13_f255(td_pbyte data, td_u16 data_len, mr_task_node *node, td_pbyte buf, td_u16 buf_len)
{
    td_u16 offset = 0;
    td_u16 len = buf_len;

    if ((data_len < len + 5) || (len > 512)) { /* 5: time 2B, protocol 1B, length 1B, and data length less than 512 */
        return;
    }

    data[offset++] = MRS_AFN_COMM_DELAY;
    data[offset++] = 0;
    data[offset++] = node->protocol;
    data[offset++] = (td_u8)(len & 0xff);
    data[offset++] = (td_u8)((len >> 8) & 0xff); /* high 8bits */
    if (memcpy_s(data + offset, data_len - offset, buf, len) != EOK) {}
}

td_void mrs_xr_fill_frame_02_f1(td_pbyte data, td_u16 data_len, mr_task_node *node, td_pbyte buf, td_u16 buf_len)
{
    td_u16 offset = 0;
    td_u8 len = (td_u8)buf_len;

    if (data_len < len + 2) { /* 2: protocol 1B, length 1B */
        return;
    }

    data[offset++] = node->protocol;
    data[offset++] = len;
    if (memcpy_s(data + offset, data_len - offset, buf, len) != EOK) {}
}

td_void mrs_xr_fill_frame_02_f255(td_pbyte data, td_u16 data_len, mr_task_node *node, td_pbyte buf, td_u16 buf_len)
{
    td_u16 offset = 0;
    td_u16 len = buf_len;

    if ((data_len < len + 3) || (len > 512)) { /* 3: protocol 1B, length 2B ,length less than 512 */
        return;
    }

    data[offset++] = node->protocol;
    data[offset++] = (td_u8)(len & 0xff);
    data[offset++] = (td_u8)((len >> 8) & 0xff); /* high 8bits */
    if (memcpy_s(data + offset, data_len - offset, buf, len) != EOK) {}
}

#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_CCO) */


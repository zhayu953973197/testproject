/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS cco mr - AFN=F1H-F1.
 */

#include "mrs_cco_pr.h"
#include "mrs_common_plc.h"
#include "mrs_cco_mr_list.h"
#include "mrs_common_tools.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)

td_u32 mrs_cco_create_pr_plc_frame(mr_task_node *node, td_pbyte buffer, td_u16 length)
{
    mrs_plc_frame_data *new_frame = TD_NULL;
    mrs_data_transmit_pr_dn *pr = TD_NULL;
    td_pbyte payload;
    td_u16 payload_len = sizeof(mrs_plc_frame_data) + sizeof(mrs_data_transmit_pr_dn) + length;

    if (length > 1000) { /* data length less than 1000 */
        return EXT_ERR_MALLOC_FAILUE;
    }

    payload = (td_pbyte)mrs_malloc(payload_len);
    if (payload == TD_NULL) {
        return EXT_ERR_MALLOC_FAILUE;
    }

    (td_void) memset_s(payload, payload_len, 0, payload_len);
    new_frame = (mrs_plc_frame_data *)payload;
    mrs_convert_meter_to_mac(node->dst_addr, sizeof(node->dst_addr), new_frame->addr, sizeof(new_frame->addr));
    new_frame->id = PLC_CMD_ID_PR;
    new_frame->payload_len = sizeof(mrs_data_transmit_pr_dn) + length;
    new_frame->payload = payload + sizeof(mrs_plc_frame_data);

    pr = (mrs_data_transmit_pr_dn *)new_frame->payload;
    pr->stru_ver = MRS_PLC_PROTO_VERSION;
    pr->stru_size = sizeof(mrs_data_transmit_pr_dn);
    pr->protocol = node->protocol;
    pr->data_len = length;
    pr->seq = node->plc_seq;
    pr->no_rsp_flg = TD_TRUE;
    pr->nak_flg = TD_TRUE;
    pr->try_max = MRS_CFG_PR_METER_MAX_RETRY;
    pr->timeout = MRS_CFG_METER_TIMEOUT;
    pr->gap = 50; /* frame interval 50*10ms */

    if (pr->data_len > 0) {
        if (memcpy_s(pr->data, pr->data_len, buffer, length) != EOK) {
            mrs_free(payload);
            return EXT_ERR_MEMCPY_FAIL;
        }
    }

    node->plc_frame = new_frame;
    return EXT_ERR_SUCCESS;
}

td_u32 mrs_cco_create_pr_1376_2_frame(td_pbyte *out_buffer, td_u16 *out_len,
                                      mr_task_node *node, td_pbyte buffer, td_u16 length)
{
    mrs_proto_1376_2_encode encode;
    td_u32 ret;

    (td_void) memset_s(&encode, sizeof(encode), 0, sizeof(encode));
    encode.length = 3 + length; /* 3: protocol 1B, length 2B, data domain L */
    encode.data = mrs_malloc(encode.length);
    if (encode.data == TD_NULL) {
        return EXT_ERR_MALLOC_FAILUE;
    }

    encode.data[0] = node->protocol;
    encode.data[1] = (td_u8)(length & 0xff);
    encode.data[2] = (td_u8)((length >> 8) & 0xff); /* index 2: high 8bits */
    if (length > 0) {
        if (memcpy_s(encode.data + 3, length, buffer, length) != EOK) { /* 3: protocol 1B,length 2B */
            mrs_free(encode.data);
            return EXT_ERR_MEMCPY_FAIL;
        }
    }

    encode.afn = 0xF1;
    encode.fn = 0x01;
    encode.seq = node->seq;
    encode.module_flag = 1;
    (td_void) memcpy_s(encode.src_addr, sizeof(encode.src_addr), node->dst_addr, sizeof(node->dst_addr));
    (td_void) memcpy_s(encode.dst_addr, sizeof(encode.dst_addr), node->src_addr, sizeof(node->src_addr));

    ret = mrs_proto_1376_2_create_frame(&encode, out_buffer, out_len);
    mrs_free(encode.data);
    return ret;
}

#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_CCO) */

/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS meter-reading list.
 */

#ifndef __MRS_CCO_MR_LIST_H__
#define __MRS_CCO_MR_LIST_H__

#include "mrs_config.h"
#include "mrs_proto_1376_2.h"
#include "mrs_common_plc.h"
#include "mrs_msg.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
    MRS_CCO_MR_TYPE_XR, /* read meter by transparent treaty: AFN=13H-F1 */
    MRS_CCO_MR_TYPE_PR, /* parallel read meter: AFN=F1H-F1 */
    MRS_CCO_MR_TYPE_MAX
} mrs_cco_mr_type;

#define MRS_MR_TASK_PRI_LO (0)
#define MRS_MR_TASK_PRI_ML (1)
#define MRS_MR_TASK_PRI_MH (2)
#define MRS_MR_TASK_PRI_HI (3)

#define MRS_XR_13H_F1   (0)
#define MRS_XR_13H_F255 (1)
#define MRS_XR_02H_F1   (2)
#define MRS_XR_02H_F255 (3)
#define MRS_PR_F1H_F1   (4)

struct mr_task_node;

typedef td_u32 (*mrs_create_mr_uart_frame)(td_pbyte *, td_u16 *, struct mr_task_node *, td_pbyte, td_u16);
typedef td_u32 (*mrs_send_uart_frame)(td_pbyte, td_u16);

/* struct of read meter task node */
typedef struct mr_task_node {
    EXT_LIST link;
    td_u8 src_addr[MRS_METER_ADDR_LEN];
    td_u8 dst_addr[MRS_METER_ADDR_LEN];
    td_u8 type : 4;
    td_u8 option : 4;
    td_u8 protocol;
    td_u8 priority;
    td_u8 seq;
    td_u8 tx_max;
    td_u8 timeout;
    td_u16 plc_seq;
    td_u32 rx_time;
    mrs_plc_frame_data *plc_frame;
    mrs_create_mr_uart_frame create_uart_frame;
    mrs_send_uart_frame send_uart_frame;
} mr_task_node;

typedef struct {
    td_u32 item_num;
    EXT_LIST mr_list;
} mr_task_list_head;

/* read meter list initialization */
td_void mrs_cco_mr_list_init(mr_task_list_head *head);
/* insert meter */
td_u32 mrs_cco_mr_list_insert(mr_task_node *node);
/* remove meter */
td_void mrs_cco_mr_list_remove(mr_task_node *node);
/* periodically check read meter queue status */
td_void mrs_mr_queue_check_on_timer(td_void);

#define mrs_cco_mr_notify() mrs_msg_queue_send(MRS_MSG_ID_MR_NOTIFY, 0, 0, 0)

#ifdef __cplusplus
}
#endif

#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_CCO) */
#endif /* __MRS_CCO_MR_LIST_H__ */


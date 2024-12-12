/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS meter-reading list.
 */

#include "mrs_cco_mr_list.h"
#include "mrs_cco_srv.h"
#include "mrs_cco_xr.h"
#include "mrs_common_tools.h"
#include "mrs_common_uart.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
mr_task_list_head *mrs_cco_mr_list(td_void);

td_void mrs_cco_mr_list_init(mr_task_list_head *head)
{
    head->item_num = 0;
    uapi_list_init(&head->mr_list);
}

mr_task_list_head *mrs_cco_mr_list(td_void)
{
    mrs_cco_srv_ctx *cco = (mrs_cco_srv_ctx *)mrs_cco_get_srv_ctx();
    if (cco != TD_NULL) {
        return &cco->mr_list_head;
    }

    return TD_NULL;
}

td_u32 mrs_cco_mr_list_insert(mr_task_node *node)
{
    mr_task_list_head *head = TD_NULL;
    EXT_LIST *link = TD_NULL;

    if (node == TD_NULL) {
        return EXT_ERR_INVALID_PARAMETER;
    }

    head = mrs_cco_mr_list();
    if (head == TD_NULL) {
        return EXT_ERR_NO_INITILIZATION;
    }

    if (head->item_num >= MRS_CFG_MR_LIST_MAX) {
        return EXT_ERR_FULL;
    }

    for (link = head->mr_list.prev; link != &head->mr_list; link = link->prev) {
        mr_task_node *temp = (mr_task_node *)link;
        if (node->priority <= temp->priority) {
            break;
        }
    }

    node->link.prev = link;
    node->link.next = link->next;
    node->link.next->prev = &node->link;
    node->link.prev->next = &node->link;
    head->item_num++;

    if (head->item_num == 1) {
        mrs_cco_mr_notify(); /* notfiy mr module to start work */
        mrs_timer_start(MRS_TIMER_ID_MR_QUEUE_CHECK, MRS_MR_QUEUE_CHECK_INTERVAL, EXT_TIMER_TYPE_PERIOD);
    }

    return EXT_ERR_SUCCESS;
}

td_void mrs_cco_mr_list_remove(mr_task_node *node)
{
    EXT_LIST *link = TD_NULL;
    mr_task_list_head *head = mrs_cco_mr_list();
    if (head == TD_NULL || node == TD_NULL) {
        return;
    }

    link = head->mr_list.next;
    while (link != &head->mr_list) {
        mr_task_node *temp = (mr_task_node *)link;
        link = link->next;

        if (temp != node) {
            continue;
        }

        uapi_list_delete(&node->link);
        if (node->plc_frame) {
            mrs_free(node->plc_frame);
        }

        mrs_free(node);
        head->item_num--;
        break;
    }
}

td_void mrs_msg_on_mr_notify(EXT_CONST mrs_queue_msg *msg)
{
    EXT_LIST *link = TD_NULL;
    td_u32 ret;
    mr_task_list_head *head = mrs_cco_mr_list();

    ext_unref_param(msg);

    if (head == TD_NULL) {
        return;
    }

    link = head->mr_list.next;
    while (link != &head->mr_list) {
        mr_task_node *node = (mr_task_node *)link;
        link = link->next;

        ret = mrs_cco_mr_add_pool_item(node);
        if (ret == EXT_ERR_FULL) {
            break;
        }
    }
}

td_void mrs_mr_queue_check_on_timer(td_void)
{
    EXT_LIST *link = TD_NULL;
    td_u32 current;
    td_u32 ret;
    ext_mdm_nm_topo_node_info entry;
    mr_task_list_head *head = mrs_cco_mr_list();

    if (head == TD_NULL) {
        return;
    }

    link = head->mr_list.next;
    current = uapi_get_seconds();

    while (link != &head->mr_list) {
        td_pbyte out_buf = TD_NULL;
        td_u16 out_len = 0;
        td_bool flag = TD_FALSE;
        mr_task_node *node = (mr_task_node *)link;

        link = link->next;

        ret = uapi_query_mac_attr(node->plc_frame->addr, &entry);
        if (ret != EXT_ERR_SUCCESS) {
            flag = TD_TRUE;
        } else if ((node->type == MRS_CCO_MR_TYPE_XR) && (node->rx_time + MRS_CFG_XR_13762_TIMEOUT <= current)) {
            flag = TD_TRUE;
        }

        if (flag == TD_FALSE) {
            continue;
        }

        ret = mrs_cco_create_xr_1376_2_frame(&out_buf, &out_len, node, TD_NULL, 0);
        if (ret == EXT_ERR_SUCCESS) {
            mrs_uart_tx(out_buf, out_len);
        }

        mrs_free(out_buf);

        mrs_cco_mr_remove_pool_item(node);
        uapi_list_delete(&node->link);
        mrs_free(node->plc_frame);
        mrs_free(node);
        head->item_num--;
    }
}

#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_CCO) */


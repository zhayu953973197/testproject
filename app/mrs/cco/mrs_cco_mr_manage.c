/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS meter-reading manage.
 */

#include "mrs_cco_mr_manage.h"
#include "mrs_common_tools.h"
#include "mrs_cco_srv.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
mrs_mr_pool *mrs_cco_mr_pool(td_void);

td_void mrs_cco_mr_pool_init(mrs_mr_pool *pool)
{
    pool->num = 0;
    uapi_list_init(&pool->list);
}

mrs_mr_pool *mrs_cco_mr_pool(td_void)
{
    mrs_cco_srv_ctx *cco = (mrs_cco_srv_ctx *)mrs_cco_get_srv_ctx();
    if (cco != TD_NULL) {
        return &cco->mr_pool;
    }

    return TD_NULL;
}

td_u32 mrs_cco_mr_add_pool_item(mr_task_node *node)
{
    EXT_LIST *link = TD_NULL;
    mrs_mr_item *item = TD_NULL;
    mrs_mr_pool *pool = mrs_cco_mr_pool();
    if (pool == TD_NULL) {
        return EXT_ERR_NO_INITILIZATION;
    }

    if (pool->num >= MRS_CFG_PARALLEL_MAX) {
        return EXT_ERR_FULL;
    }

    link = pool->list.next;
    while (link != &pool->list) {
        mrs_mr_item *temp = (mrs_mr_item *)link;
        if (memcmp(node->dst_addr, temp->node->dst_addr, sizeof(node->dst_addr)) == 0) {
            return EXT_ERR_BUSY;
        }

        link = link->next;
    }

    item = (mrs_mr_item *)mrs_malloc(sizeof(mrs_mr_item));
    if (item == TD_NULL) {
        return EXT_ERR_MALLOC_FAILUE;
    }

    (td_void) memset_s(item, sizeof(mrs_mr_item), 0, sizeof(mrs_mr_item));
    item->node = node;
    item->wait_tx = TD_TRUE;
    uapi_list_tail_insert(&pool->list, &item->link);
    pool->num++;

    if (pool->num == 1) {
        mrs_timer_start(MRS_TIMER_ID_PARALLEL_CHECK, MRS_PARALLEL_CHECK_INTERVAL, EXT_TIMER_TYPE_PERIOD);
    }

    return EXT_ERR_SUCCESS;
}

td_void mrs_cco_mr_remove_pool_item(const mr_task_node *node)
{
    EXT_LIST *link = TD_NULL;
    mrs_mr_pool *pool = mrs_cco_mr_pool();
    if (pool == TD_NULL) {
        return;
    }

    link = pool->list.next;
    while (link != &pool->list) {
        mrs_mr_item *item = (mrs_mr_item *)link;
        link = link->next;

        if (node != item->node) {
            continue;
        }

        uapi_list_delete(&item->link);
        pool->num--;
        mrs_free(item);
        return;
    }
}

mrs_mr_item *mrs_cco_mr_find_pool_item(const td_pbyte mac, td_u16 seq, td_u8 type)
{
    EXT_LIST *link = TD_NULL;
    mrs_mr_pool *pool = mrs_cco_mr_pool();
    if (pool == TD_NULL) {
        return TD_NULL;
    }

    link = pool->list.next;
    while (link != &pool->list) {
        mrs_mr_item *item = (mrs_mr_item *)link;
        mr_task_node *node = item->node;

        link = link->next;

        if ((node->type == type) && (node->plc_seq == seq) &&
            (memcmp(node->plc_frame->addr, mac, EXT_PLC_MAC_ADDR_LEN) == 0)) {
            return item;
        }
    }

    return TD_NULL;
}

td_void mrs_parallel_check_on_timer(td_void)
{
    td_u32 ret = EXT_ERR_SUCCESS;
    mrs_mr_pool *pool = mrs_cco_mr_pool();
    if (pool == TD_NULL) {
        return;
    }

    EXT_LIST *link = pool->list.next;
    while (link != &pool->list) {
        mrs_mr_item *item = (mrs_mr_item *)link;
        link = link->next;

        if (item->wait_tx == TD_TRUE) {
            item->wait_tx = TD_FALSE;
            if (mrs_plc_frame_send(item->node->plc_frame) != EXT_ERR_SUCCESS) {
                mrs_printf("mrs_parallel_check_on_timer(): send plc frame failed. ret = %d\n", ret);
            }
            continue;
        }

        item->run_time += MRS_PARALLEL_CHECK_INTERVAL;

        if (item->run_time < (td_u32)(item->node->timeout) * 100) { /* unit : 100ms */
            continue;
        }
        item->try_times++;
        item->run_time = 0;
        if (item->try_times >= item->node->tx_max) {
            td_pbyte out_buf = TD_NULL;
            td_u16 out_len = 0;
            ret = item->node->create_uart_frame(&out_buf, &out_len, item->node, TD_NULL, 0);
            if (ret == EXT_ERR_SUCCESS) {
                item->node->send_uart_frame(out_buf, out_len);
            }
            mrs_free(out_buf);

            mrs_cco_mr_list_remove(item->node);
            uapi_list_delete(&item->link);
            pool->num--;
            mrs_free(item);
        } else if (mrs_plc_frame_send(item->node->plc_frame) != EXT_ERR_SUCCESS) {
            mrs_printf("mrs_parallel_check_on_timer(): send plc frame failed. ret = %d\n", ret);
        }
    }

    if (pool->num < MRS_CFG_PARALLEL_MAX) {
        mrs_cco_mr_notify();
    }

    if (pool->num == 0) {
        mrs_timer_stop(MRS_TIMER_ID_PARALLEL_CHECK);
    }
}
#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_CCO) */

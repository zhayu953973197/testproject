/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: MRS meter-reading manage.
 */

#ifndef __MRS_CCO_MR_MANAGE_H__
#define __MRS_CCO_MR_MANAGE_H__

#include "mrs_cco_mr_list.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    EXT_LIST link;
    mr_task_node *node;
    td_u16 run_time;
    td_u16 try_times : 4;
    td_u16 wait_tx : 1;
    td_u16 rsv : 11;
} mrs_mr_item;

typedef struct {
    td_u32 num;
    EXT_LIST list;
} mrs_mr_pool;

/* CCO parallel read meter pool initialization */
td_void mrs_cco_mr_pool_init(mrs_mr_pool *pool);
/* add new read meter task node */
td_u32 mrs_cco_mr_add_pool_item(mr_task_node *node);
/* remove read meter task node */
td_void mrs_cco_mr_remove_pool_item(const mr_task_node *node);
/* find read meter task node */
mrs_mr_item *mrs_cco_mr_find_pool_item(const td_pbyte mac, td_u16 seq, td_u8 type);
/* periodically manage parallel read meter */
td_void mrs_parallel_check_on_timer(td_void);

#ifdef __cplusplus
}
#endif

#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_CCO) */
#endif /* __MRS_CCO_MR_MANAGE_H__ */

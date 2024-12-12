/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: CCO meter archive handle.
 */

#include "mrs_cco_archives.h"
#include "mrs_cco_srv.h"
#include "mrs_common_tools.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
#define ARCHIVES_LOAD_MAX        ARCHIVES_NUM_MAX
#define ARCHIVES_ADDR_OFFSET     0x2000
#define METER_ARCHIVES_MAGIC     uapi_make_identifier('M', 'R', 'S', 'M')
#define METER_ARCHIVES_ITEM_SIZE (sizeof(meter_item))

meter_archives *g_meter_archives = TD_NULL;
td_bool g_meter_archives_changed = TD_FALSE;
td_u32 g_archives_flash_addr = 0;

/* Free globle context */
td_void mrs_archives_free(td_void);
/* Load head and meters from flash */
td_u32 mrs_archives_load(td_void);
/* Save head and meters to flash */
td_u32 mrs_archives_save(td_void);
/* Verify archives saved be avaliable */
td_u32 mrs_meter_archives_verify(const meter_archives *archives);

/* timeout proc */
td_void mrs_archives_on_timer(td_void)
{
    if (g_meter_archives != TD_NULL) {
        if (g_meter_archives_changed == TD_TRUE) {
            mrs_archives_save();
            mrs_cco_setup_whitelist();

            /* set back to default value */
            g_meter_archives_changed = TD_FALSE;
        }
    }

    mrs_archives_free();
}

td_u32 mrs_archives_init(td_void)
{
    td_u32 ret;
    ext_flash_partition_table *ptbl = TD_NULL;

    if (g_meter_archives != TD_NULL) {
        mrs_archives_free();
    }

    g_meter_archives = (meter_archives *)mrs_malloc(sizeof(meter_archives));
    if (g_meter_archives == TD_NULL) {
        return EXT_ERR_FAILURE;
    }

    ret = (td_u32)memset_s(g_meter_archives, sizeof(meter_archives), 0, sizeof(meter_archives));
    if (ret != EXT_ERR_SUCCESS) {
        mrs_archives_free();
        return ret;
    }

    /* Get archives address */
    ptbl = uapi_get_partition_table();
    if (ptbl == TD_NULL) {
        mrs_archives_free();
        return EXT_ERR_FAILURE;
    }

    g_archives_flash_addr = (td_u32)(ptbl->table[EXT_FLASH_PARTITON_USER_INFO_1].addr_blk * PRODUCT_CFG_FLASH_BLOCK_SIZE
                                     + ARCHIVES_ADDR_OFFSET);

    ret = mrs_archives_load();
    if (ret != EXT_ERR_SUCCESS) {
        mrs_archives_free();
        g_meter_archives_changed = TD_FALSE;
        return EXT_ERR_FAILURE;
    }

    return ret;
}

td_void mrs_archives_free(td_void)
{
    if (g_meter_archives != TD_NULL) {
        mrs_free(g_meter_archives);
        g_meter_archives = TD_NULL;
    }
}

td_u32 mrs_archives_load(td_void)
{
    td_u32 ret;
    td_u32 read_addr = g_archives_flash_addr;
    td_u32 read_size = sizeof(meter_archives);
    meter_archives *archives = mrs_archives_get();

    if ((archives == TD_NULL) || (read_addr == 0)) {
        return EXT_ERR_FAILURE;
    }

    /* Read head from flash first in order to get number of available meters */
    ret = uapi_flash_read(read_addr, read_size, (td_u8 *)archives);
    if (ret != EXT_ERR_SUCCESS) {
        return ret;
    }

    ret = mrs_meter_archives_verify(archives);
    if (ret != EXT_ERR_SUCCESS) {
        /* Dirty Data on flash, reset */
        mrs_archives_clear();
    }

    return EXT_ERR_SUCCESS;
}

td_u32 mrs_meter_archives_verify(const meter_archives *archives)
{
    td_u32 crc = 0;
    if (archives == TD_NULL) {
        return EXT_ERR_FAILURE;
    }

    if (archives->head.magic != METER_ARCHIVES_MAGIC) {
        return EXT_ERR_FAILURE;
    }

    /* From magic in head to the end, skip size of element crc32 */
    crc = uapi_crc32(crc, (EXT_CONST td_u8 *)&archives->head.magic, (td_u32)sizeof(meter_archives) - sizeof(td_u32));
    if (crc != archives->head.crc32) {
        return EXT_ERR_FAILURE;
    }

    return EXT_ERR_SUCCESS;
}

td_u32 mrs_archives_save()
{
    td_u32 ret;

    if ((g_meter_archives == TD_NULL) || (g_archives_flash_addr == 0)) {
        return EXT_ERR_FAILURE;
    }

    g_meter_archives->head.magic = METER_ARCHIVES_MAGIC;
    /* From magic in head to the end, skip size of element crc32 */
    g_meter_archives->head.crc32 = uapi_crc32(0, (EXT_CONST td_u8 *)&g_meter_archives->head.magic,
                                            (td_u32)sizeof(meter_archives) - sizeof(td_u32));

    ret = uapi_flash_erase(g_archives_flash_addr, PRODUCT_CFG_FLASH_BLOCK_SIZE * 2); /* erase 2 block */

    ret |= uapi_flash_write(g_archives_flash_addr, sizeof(meter_archives), (EXT_CONST td_u8 *)g_meter_archives,
    TD_FALSE);

    return ret;
}

meter_archives *mrs_archives_get(td_void)
{
    if (g_meter_archives == TD_NULL) {
        if (mrs_archives_init() != EXT_ERR_SUCCESS) {
            return TD_NULL;
        }
    }

    if (g_meter_archives != TD_NULL) {
        mrs_timer_start(MRS_TIMER_ID_METER_ARCHIVES, ARCHIVES_SLIDE_WINDOW, EXT_TIMER_TYPE_ONCE);
    }

    return g_meter_archives;
}

td_u32 mrs_archives_append(const td_pbyte list, td_u16 count)
{
    meter_archives *archives = mrs_archives_get();
    td_u16 i;
    td_u16 j;

    if (archives == TD_NULL) {
        return EXT_ERR_FAILURE;
    }

    if (archives->head.num == ARCHIVES_NUM_MAX) {
        return EXT_ERR_FULL;
    }

    mrs_timer_start(MRS_TIMER_ID_METER_ARCHIVES, ARCHIVES_SLIDE_WINDOW, EXT_TIMER_TYPE_ONCE);

    /* Because meter struct is one byte larger than 1376.2 defined, pay attention to the struct size
     * CNComments: 因为档案中的表结构比376.2协议报文中的从节点结构不同，
     * 在比较和拷贝的时候需要注意。
 */
    for (i = 0; (i < count) && (archives->head.num < ARCHIVES_NUM_MAX); i++) {
        for (j = 0; j < archives->head.num; j++) {
            if (0 == memcmp(list + MRS_1376_2_SEC_NODE_ITEM_SIZE * i,
                            archives->items + j,
                            MRS_1376_2_SEC_NODE_ITEM_SIZE)) {
                break;
            }
        }

        if (j == archives->head.num) {
            if (memcpy_s(archives->items + j, sizeof(meter_item),
                         list + MRS_1376_2_SEC_NODE_ITEM_SIZE * i, MRS_1376_2_SEC_NODE_ITEM_SIZE) != EOK) {
                continue;
            }

            archives->head.num++;
            g_meter_archives_changed = TD_TRUE;
        }
    }

    return EXT_ERR_SUCCESS;
}

td_u32 mrs_archives_delete(const td_pbyte list, td_u16 count)
{
    meter_archives *archives = mrs_archives_get();
    meter_archives_head *head = TD_NULL;
    meter_item *items = TD_NULL;
    td_u16 num_tmp;
    td_u16 i;
    td_u16 j;

    if (archives == TD_NULL) {
        return EXT_ERR_FAILURE;
    }

    head = &archives->head;
    items = archives->items;
    num_tmp = head->num;

    if (num_tmp == 0) {
        /* At bottom */
        return EXT_ERR_NOT_FOUND;
    }

    mrs_timer_start(MRS_TIMER_ID_METER_ARCHIVES, ARCHIVES_SLIDE_WINDOW, EXT_TIMER_TYPE_ONCE);

    for (i = 0; i < count; i++) {
        for (j = 0; j < head->num; j++) {
            if (memcmp(list + ARCHIVES_METER_ADDR_SIZE * i, items + j, ARCHIVES_METER_ADDR_SIZE) != 0) {
                continue;
            }

            if (j != (head->num - 1)) {
                /* Move the last one to fill in */
                (td_void) memcpy_s(items + j, sizeof(meter_item),
                                   items + (head->num - 1), sizeof(meter_item));
            }

            (td_void) memset_s(items + (head->num - 1), sizeof(meter_item), 0xff, sizeof(meter_item));

            /* Keep updating the head */
            head->num--;
            g_meter_archives_changed = TD_TRUE;
            break;
        }
    }

    if (num_tmp == head->num) {
        /* Did NOT delete any meter */
        return EXT_ERR_NOT_FOUND;
    }

    return EXT_ERR_SUCCESS;
}

td_u32 mrs_archives_clear(td_void)
{
    meter_archives *archives = mrs_archives_get();

    if (archives == TD_NULL) {
        return EXT_ERR_FAILURE;
    }

    if ((archives->head).num != 0) {
        (td_void) memset_s(archives, sizeof(meter_archives), 0xFF, sizeof(meter_archives));
        (archives->head).crc32 = 0;
        archives->head.magic = METER_ARCHIVES_MAGIC;
        (archives->head).num = 0;
        g_meter_archives_changed = TD_TRUE;
    }

    mrs_timer_start(MRS_TIMER_ID_METER_ARCHIVES, ARCHIVES_SLIDE_WINDOW, EXT_TIMER_TYPE_ONCE);

    return EXT_ERR_SUCCESS;
}

td_u16 mrs_archives_get_number(td_void)
{
    meter_archives *archives = mrs_archives_get();

    if (archives == TD_NULL) {
        return 0;
    }

    return archives->head.num;
}

meter_item *mrs_archives_get_by_idx(td_u16 idx)
{
    meter_archives_head *head = TD_NULL;
    meter_item *meters = TD_NULL;
    meter_archives *archives = mrs_archives_get();

    if (archives == TD_NULL) {
        return TD_NULL;
    }

    head = &archives->head;
    if (head->num <= idx) {
        /* No valid meter behind */
        return TD_NULL;
    }

    meters = archives->items;

    return (meters + idx);
}

meter_item *mrs_archives_get_meter_by_addr(const td_u8 addr[MRS_METER_ADDR_LEN])
{
    meter_archives *archives = mrs_archives_get();
    meter_item *items = TD_NULL;
    td_u16 item_num = mrs_archives_get_number();
    td_u16 idx;

    if ((archives == TD_NULL) || (item_num == 0)) {
        return TD_NULL;
    }

    items = archives->items;

    for (idx = 0; idx < item_num; idx++) {
        if (memcmp(items[idx].addr, addr, MRS_METER_ADDR_LEN) == 0) {
            break;
        }
    }

    return (items + idx);
}

td_u8 mrs_archives_get_meter_protocol(const td_u8 addr[MRS_METER_ADDR_LEN])
{
    meter_item *meter = mrs_archives_get_meter_by_addr(addr);

    if (meter != TD_NULL) {
        return meter->protocol;
    }

    return METER_PROTO_UNKNOWN;
}

#endif


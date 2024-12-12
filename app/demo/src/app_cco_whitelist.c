/*
 * Copyright (c) CompanyNameMagicTag 2012-2020. All rights reserved.
 * Description: No description.
 */

#include "app_cco_whitelist.h"
#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
#include "app_main.h"
#include "app_common.h"
#include <soc_mdm_mac.h>
#include "soc_mdm_partition_table.h"
#include "securec.h"
#include "soc_mdm_mem.h"
#include "soc_mdm_crc.h"
#include "soc_mdm_flash.h"

#define WHITE_LIST_ADDR_OFFSET  (0x2000)
#define WHITE_LIST_WRITE_FLASH   20000 /* 20s */
#define WHITE_LIST_SLIDE_WINDOW  2000  /* 2s */

app_whitelist_head *g_white_list = TD_NULL;
td_u32 g_white_list_flash_addr = 0;
td_bool g_white_list_changed = TD_FALSE;

app_whitelist_head *app_whitelist_obtain(td_void);
td_void app_whitelist_free(td_void);
td_u32 app_whitelist_verify(app_whitelist_head *white_list);
td_u32 app_set_whitelist_to_mac(app_whitelist_head *white_list);
td_u32 app_whitelist_storage(app_whitelist_head *white_list);
td_u32 app_whitelist_load(app_whitelist_head *white_list);

app_whitelist_head *app_whitelist_obtain(td_void)
{
    td_u32 ret;
    ext_flash_partition_table *ptbl = TD_NULL;

    g_white_list = (app_whitelist_head *)uapi_malloc(EXT_MOD_ID_APP_COMMON, sizeof(app_whitelist_head));
    if (g_white_list == TD_NULL) {
        return TD_NULL;
    }

    /* Get archives address */
    ptbl = uapi_get_partition_table();
    if (ptbl == TD_NULL) {
        return TD_NULL;
    }

    g_white_list_flash_addr = (td_u32)(WHITE_LIST_ADDR_OFFSET +
                                        ptbl->table[EXT_FLASH_PARTITON_USER_INFO_1].addr_blk *
                                        PRODUCT_CFG_FLASH_BLOCK_SIZE);

    ret = app_whitelist_load(g_white_list);
    if (ret != EXT_ERR_SUCCESS) {
        (td_void) memset_s(g_white_list, sizeof(app_whitelist_head), 0xFF, sizeof(app_whitelist_head));
        g_white_list->total = 0;
        g_white_list_changed = TD_TRUE;
    }

    return g_white_list;
}

td_void app_whitelist_free(td_void)
{
    if (g_white_list != TD_NULL) {
        uapi_free(EXT_MOD_ID_APP_COMMON, g_white_list);
        g_white_list = TD_NULL;
    }

    g_white_list_changed = TD_FALSE;
    app_timer_stop(APP_TIMER_ID_WRITE_WHITELIST_TO_FLASH);
}

/* White List: Get White List */
app_whitelist_head *app_get_whitelist(td_void)
{
    app_whitelist_head *white_list = g_white_list;

    if (white_list == TD_NULL) {
        white_list = app_whitelist_obtain();
    }

    if (white_list != TD_NULL) {
        app_timer_start(APP_TIMER_ID_WRITE_WHITELIST_TO_FLASH, WHITE_LIST_WRITE_FLASH, EXT_TIMER_TYPE_ONCE);
    }

    return white_list;
}

/* Setup whitelist to MAC */
td_u32 app_whitelist_setup(td_void)
{
    td_u32 ret;
    app_whitelist_head *white_list = app_get_whitelist();

    if (white_list == TD_NULL) {
        return EXT_ERR_FAILURE;
    }

    ret = app_set_whitelist_to_mac(white_list);
    app_whitelist_free();

    return ret;
}

td_u32 app_whitelist_verify(app_whitelist_head *white_list)
{
    td_u32 crc = 0;

    if (white_list->magic != WHITE_LIST_MAGIC) {
        return EXT_ERR_FAILURE;
    }

    crc = uapi_crc32(crc, (EXT_CONST td_u8 *)&white_list->magic, (td_u32)sizeof(app_whitelist_head) - 4);
    if (crc != white_list->crc) {
        return EXT_ERR_FAILURE;
    }

    return EXT_ERR_SUCCESS;
}

td_u32 app_set_whitelist_to_mac(app_whitelist_head *white_list)
{
    ext_mac_valid_mac_list *mac_list = TD_NULL;
    ext_mac_entry *entry = TD_NULL;
    td_u16 i;
    td_u32 data_size;
    td_u16 tei = uapi_topo_get_max_tei();
    data_size = uapi_min(tei, white_list->total);
    data_size = data_size * sizeof(ext_mac_entry) + sizeof(ext_mac_valid_mac_list);

    mac_list = (ext_mac_valid_mac_list *)uapi_malloc(EXT_MOD_ID_APP_COMMON, data_size);
    if (mac_list == TD_NULL) {
        return EXT_ERR_MALLOC_FAILUE;
    }

    (td_void) memset_s(mac_list, data_size, 0, data_size);
    tei = uapi_topo_get_max_tei();
    mac_list->list_num = uapi_min(tei, white_list->total);

    entry = mac_list->mac_list;
    for (i = 0; i < mac_list->list_num; i++) {
        if (memcpy_s(entry->mac, sizeof(entry->mac), white_list->list + WHITE_LIST_NODE_SIZE * i,
                     WHITE_LIST_NODE_SIZE) != EOK) {
            continue;
        }
        entry++;
    }

    uapi_set_valid_mac_list(mac_list, EXT_MAC_WHITE_LIST_CHANGE_REASON_APP_INIT);
    uapi_free(EXT_MOD_ID_APP_COMMON, mac_list);
    mac_list = TD_NULL;

    return EXT_ERR_SUCCESS;
}

/* White List: clear */
td_u32 app_whitelist_clear(td_void)
{
    app_whitelist_head *white_list = app_get_whitelist();

    if (white_list == TD_NULL) {
        return EXT_ERR_FAILURE;
    }

    if (white_list->total != 0) {
        (td_void) memset_s(white_list, sizeof(app_whitelist_head), 0xFF, sizeof(app_whitelist_head));
        white_list->total = 0;
        g_white_list_changed = TD_TRUE;
    }
    app_timer_start(APP_TIMER_ID_WRITE_WHITELIST_TO_FLASH, WHITE_LIST_WRITE_FLASH, EXT_TIMER_TYPE_ONCE);
    /* modifications to the whitelist need to take effect after a delay */
    app_timer_start(APP_TIMER_ID_WHITELIST, WHITE_LIST_SLIDE_WINDOW, EXT_TIMER_TYPE_ONCE);

    return EXT_ERR_SUCCESS;
}

/* White List: Add node */
td_u32 app_whitelist_add(td_pbyte list, td_u16 count)
{
    app_whitelist_head *white_list = app_get_whitelist();
    td_u16 i, j;
    td_u16 tei = uapi_topo_get_max_tei();
    td_u32 ret = EXT_ERR_SUCCESS;

    if (white_list == TD_NULL) {
        return EXT_ERR_FAILURE;
    }

    app_timer_start(APP_TIMER_ID_WRITE_WHITELIST_TO_FLASH, WHITE_LIST_WRITE_FLASH, EXT_TIMER_TYPE_ONCE);

    for (i = 0; (i < count) && (white_list->total < (sizeof(white_list->list) / WHITE_LIST_NODE_SIZE)); i++) {
        for (j = 0; j < white_list->total; j++) {
            if (memcmp(list + WHITE_LIST_NODE_SIZE * i, white_list->list + WHITE_LIST_NODE_SIZE * j,
                       WHITE_LIST_NODE_SIZE) == 0) {
                break;
            }
        }

        if (j == white_list->total) {
            if (white_list->total >= tei) {
                ret = EXT_ERR_FAILURE;
                break;
            }
            if (memcpy_s(white_list->list + WHITE_LIST_NODE_SIZE * j, WHITE_LIST_NODE_SIZE,
                         list + WHITE_LIST_NODE_SIZE * i, WHITE_LIST_NODE_SIZE) != EOK) {
                continue;
            }
            white_list->total++;
            g_white_list_changed = TD_TRUE;
        }
    }
    if (g_white_list_changed == TD_TRUE) {
        /* modifications to the whitelist need to take effect after a delay */
        app_timer_start(APP_TIMER_ID_WHITELIST, WHITE_LIST_SLIDE_WINDOW, EXT_TIMER_TYPE_ONCE);
    }

    return ret;
}

td_bool app_whitelist_is_continue_check(td_pbyte list, app_whitelist_head *white_list, td_u16 i, td_u16 j)
{
    if (memcmp(list + WHITE_LIST_NODE_SIZE * i, white_list->list + WHITE_LIST_NODE_SIZE * j,
               WHITE_LIST_NODE_SIZE) != 0) {
        return TD_FALSE;
    }

    if (j != white_list->total - 1) {
        if (memcpy_s(white_list->list + WHITE_LIST_NODE_SIZE * j,
                     WHITE_LIST_NODE_SIZE,
                     white_list->list + WHITE_LIST_NODE_SIZE * (white_list->total - 1),
                     WHITE_LIST_NODE_SIZE) != EOK) {
            return TD_FALSE;
        }
    }

    return TD_TRUE;
}

/* White List: Del node */
td_u32 app_whitelist_delete(td_pbyte list, td_u16 count)
{
    app_whitelist_head *white_list = app_get_whitelist();
    td_u16 i, j;

    if (white_list == TD_NULL) {
        return EXT_ERR_FAILURE;
    }

    app_timer_start(APP_TIMER_ID_WRITE_WHITELIST_TO_FLASH, WHITE_LIST_WRITE_FLASH, EXT_TIMER_TYPE_ONCE);
    for (i = 0; i < count; i++) {
        for (j = 0; j < white_list->total; j++) {
            if (app_whitelist_is_continue_check(list, white_list, i, j) == TD_FALSE) {
                continue;
            }

            (td_void) memset_s(white_list->list + WHITE_LIST_NODE_SIZE * (white_list->total - 1),
                               WHITE_LIST_NODE_SIZE, 0xFF, WHITE_LIST_NODE_SIZE);
            white_list->total--;
            g_white_list_changed = TD_TRUE;
            break;
        }
    }

    if (g_white_list_changed == TD_TRUE) {
        /* modifications to the whitelist need to take effect after a delay */
        app_timer_start(APP_TIMER_ID_WHITELIST, WHITE_LIST_SLIDE_WINDOW, EXT_TIMER_TYPE_ONCE);
    }

    return EXT_ERR_SUCCESS;
}

td_u32 app_whitelist_storage(app_whitelist_head *white_list)
{
    td_u32 ret;

    if (g_white_list_flash_addr == 0) {
        return EXT_ERR_FAILURE;
    }

    white_list->magic = WHITE_LIST_MAGIC;
    white_list->crc = uapi_crc32(0, (EXT_CONST td_u8 *)&white_list->magic, (td_u32)sizeof(app_whitelist_head) - 4);

    ret = uapi_flash_erase(g_white_list_flash_addr, PRODUCT_CFG_FLASH_BLOCK_SIZE);
    ret |= uapi_flash_write(g_white_list_flash_addr, sizeof(app_whitelist_head), (EXT_CONST td_u8 *)white_list,
                          TD_FALSE);

    return ret;
}

td_u32 app_whitelist_load(app_whitelist_head *white_list)
{
    td_u32 ret;

    if (g_white_list_flash_addr == 0) {
        return EXT_ERR_FAILURE;
    }

    ret = uapi_flash_read(g_white_list_flash_addr, sizeof(app_whitelist_head), (td_u8 *)white_list);
    if (ret != EXT_ERR_SUCCESS) {
        return EXT_ERR_FAILURE;
    }

    ret = app_whitelist_verify(white_list);
    if (ret != EXT_ERR_SUCCESS) {
        return EXT_ERR_FAILURE;
    }

    return EXT_ERR_SUCCESS;
}

td_void app_whitelist_timeout(td_void)
{
    if (g_white_list != TD_NULL) {
        if (g_white_list_changed == TD_TRUE) {
            (td_void) app_whitelist_storage(g_white_list);
            app_whitelist_timeout_notify_mac();
        }
    }

    app_whitelist_free();
}

td_void app_whitelist_timeout_notify_mac(td_void)
{
    if (g_white_list != TD_NULL) {
        if (g_white_list_changed == TD_TRUE) {
            (td_void) app_set_whitelist_to_mac(g_white_list);
        }
    }
}
#endif


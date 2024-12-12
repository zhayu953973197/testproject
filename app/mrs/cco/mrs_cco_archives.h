/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: CCO meter archive handle.
 */

#ifndef MRS_ARCHIVES_H__
#define MRS_ARCHIVES_H__

#include "mrs_config.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
#ifdef __cplusplus
extern "C" {
#endif


#define ARCHIVES_NUM_MAX         (MRS_CFG_METER_MAX)
#define ARCHIVES_METER_ADDR_SIZE (MRS_METER_ADDR_LEN)

/* Meter Archives Header */
typedef struct {
    td_u32 crc32;  /* < crc sum, from magic to last block */
    td_u32 magic;  /* < "MRSM" */
    td_u16 num;    /* < Number of avalid meters in the archives */
    td_u16 rsv[3]; /* < reserved 3 */
} meter_archives_head;

/* Meter Archives Item */
typedef struct {
    td_u8 addr[ARCHIVES_METER_ADDR_SIZE]; /* < Meter mac address */
    td_u8 protocol;                       /* < Meter protocol version */
    td_u8 rsv;
} meter_item;

/* Meter Archives */
typedef struct {
    meter_archives_head head;           /* < Archives head */
    meter_item items[ARCHIVES_NUM_MAX]; /* < Meter list */
} meter_archives;

/* Initialize meter archives */
td_u32 mrs_archives_init(td_void);

/* Get meter archives */
meter_archives *mrs_archives_get(td_void);

/* Get meter archives by index */
meter_item *mrs_archives_get_by_idx(td_u16 idx);

/* Add new meter item to archives */
td_u32 mrs_archives_append(const td_pbyte list, td_u16 count);

/* Delete meter item from archives
 * Make sure the meter addresses follow each other one by one as a list.
 */
td_u32 mrs_archives_delete(const td_pbyte item, td_u16 count);

/* Clear meter archives */
td_u32 mrs_archives_clear(td_void);

/* Get number of avaliable meter archive items */
td_u16 mrs_archives_get_number(td_void);

/* Timer for archives saving */
td_void mrs_archives_on_timer(td_void);

meter_item *mrs_archives_get_meter_by_addr(const td_u8 addr[MRS_METER_ADDR_LEN]);
td_u8 mrs_archives_get_meter_protocol(const td_u8 addr[MRS_METER_ADDR_LEN]);

#ifdef __cplusplus
}
#endif

#endif /* defined(PRODUCT_CFG_PRODUCT_TYPE_CCO) */
#endif /* __MRS_ARCHIVES_H__ */


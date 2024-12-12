/*
 * Copyright (c) CompanyNameMagicTag 2019-2020. All rights reserved.
 * Description: tools.
 */

#ifndef __MRS_COMMON_TOOLS_H__
#define __MRS_COMMON_TOOLS_H__

#include "mrs_config.h"
#include "mrs_msg.h"
#include "mrs_common_uart.h"

#ifdef __cplusplus
extern "C" {
#endif

#define mrs_malloc(sz) uapi_malloc(EXT_MOD_ID_APP_COMMON, (sz))
#define mrs_free(p)    \
    do {                                        \
        if ((p) != TD_NULL) {                   \
            uapi_free(EXT_MOD_ID_APP_COMMON, (p)); \
            (p) = TD_NULL;                      \
        }                                       \
    } while (0)

#define MRS_INVALID_METER_ADDR   "\x00\x00\x00\x00\x00\x00"
#define mrs_invalid_meter(meter) (memcmp((meter), MRS_INVALID_METER_ADDR, EXT_PLC_MAC_ADDR_LEN) == 0)

#define MRS_INVALID_MAC_ADDR   "\x00\x00\x00\x00\x00\x00"
#define MRS_LOCAL_BC_MAC_ADDR  "\x00\xFF\xFF\xFF\xFF\xFF"
#define MRS_BROADCAST_MAC_ADDR "\xFF\xFF\xFF\xFF\xFF\xFF"
#define MRS_WILDCARD_MAC_ADDR  "\xAA\xAA\xAA\xAA\xAA\xAA"

#define mrs_is_invalid_mac_addr(x)    (memcmp((x), MRS_INVALID_MAC_ADDR, EXT_PLC_MAC_ADDR_LEN) == 0)
#define mrs_is_local_bc_mac_addr(x)   (memcmp((x), MRS_LOCAL_BC_MAC_ADDR, EXT_PLC_MAC_ADDR_LEN) == 0)
#define mrs_is_broadcast_mac_addr(x)  (memcmp((x), MRS_BROADCAST_MAC_ADDR, EXT_PLC_MAC_ADDR_LEN) == 0)
#define mrs_is_wildcast_meter_addr(x) (memcmp((x), MRS_WILDCARD_MAC_ADDR, EXT_PLC_MAC_ADDR_LEN) == 0)

#define bcd2int(x)                  (td_u8)((x) / 16 * 10 + ((x)&0x0f))
#define is_leap_year(y)             ((((y) % 4) == 0 && ((y) % 100) != 0) || ((y) % 400) == 0)
#define not_in_range(_min, _max, d) (((_min) > (d)) || ((_max) < (d)))

#define uapi_sec_2_ms(x)     ((td_u32)(x)*1000)
#define ext_minute_2_sec(x) ((td_u32)(x)*60)
#define ext_minute_2_ms(x)  (uapi_sec_2_ms(ext_minute_2_sec(x)))

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
#undef MRS_LOG_DEBUG
#else
#define MRS_LOG_DEBUG
#endif
#if defined(MRS_LOG_DEBUG)
#define mrs_printf(fmt, ...) /* printf(fmt, ##__VA_ARGS__) */
td_void mrs_print_buffer(const char *hint, td_pbyte buffer, td_u16 length);
#define mrs_logbuf(sz, buf, len) mrs_print_buffer((char *)(sz), (td_pbyte)(buf), (td_u16)(len))
#else
#define mrs_printf(fmt, ...)
#define mrs_logbuf(sz, buf, len)
#endif

td_u32 mrs_timer_start(mrs_timer_id timer_id, td_u32 timeout, ext_timer_type_e option);
td_u32 mrs_timer_stop(mrs_timer_id timer_id);

td_u32 mrs_msg_queue_send(td_u16 id, td_u16 param0, uintptr_t param1, uintptr_t param2);

td_u32 mrs_semaphore_wait(mrs_sem_id sem_id);

td_u32 mrs_semaphore_signal(mrs_sem_id sem_id);

td_pbyte mrs_find_byte_in_stream(const td_pbyte buffer, td_u16 length, td_u8 ch);

td_bool mrs_check_bcd_stream(const td_pbyte stream, td_u16 length);

td_u8 mrs_checksum8(const td_pbyte buffer, td_u16 length);

td_void mrs_hex_invert(td_pbyte buffer, td_u16 length);

/* convert integer to BCD code with fixed length, and invert it */
td_u32 mrs_convert_int_to_bcd(td_u32 data, td_u8 *bcd, td_u8 length);

td_u32 mrs_time_sub_duration(td_u32 time, td_u32 duration);

#ifdef __cplusplus
}
#endif
#endif /* __MRS_COMMON_TOOLS_H__ */


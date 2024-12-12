/*
 * Copyright (c) CompanyNameMagicTag 2018-2020. All rights reserved.
 * Description: APP code for DUT test.
 */

#ifndef APP_STA_DUT_H
#define APP_STA_DUT_H

#include "soc_types.h"

#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)
#ifdef __cplusplus
extern "C" {
#endif

#define APP_DUT_DI_BYTE_OFFSET   10
#define APP_DUT_DI_LEN           4
#define APP_DUT_MAC_BYTE_OFFSET  14
#define APP_DUT_REPLY_DATA       0xFE
#define APP_DUT_PAYLOAD_INDEX0   0
#define APP_DUT_PAYLOAD_INDEX1   1
#define APP_DUT_PAYLOAD_INDEX2   2
#define APP_DUT_PAYLOAD_INDEX3   3
#define APP_DUT_PAYLOAD_LEN      4
#define APP_DUT_BUF_LEN1         6
#define APP_DUT_BUF_DATA1        0xCC
#define APP_DUT_BUF_INDEX7       7
#define APP_DUT_BUF_INDEX8       8
#define APP_DUT_BUF_INDEX9       9
#define APP_DUT_BUF_INDEX8_DATA  0x9E
#define APP_DUT_BUF_LEN2         10
#define APP_DUT_BUF_LEN3         14
#define APP_DUT_BUF_CHECKSUM_LEN 2

td_bool app_handle_dut_frame(EXT_CONST td_pbyte frame, td_u16 length);

#ifdef __cplusplus
}
#endif
#endif
#endif

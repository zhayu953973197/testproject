/*
 * Copyright (c) CompanyNameMagicTag 2018-2020. All rights reserved.
 * Description: Command/Message Define.
 */

#ifndef APP_CMD_MSG_H
#define APP_CMD_MSG_H

#define APP_PLC_ID_DATA_TRANSMIT  0x09FF
#define APP_DATA_TRANSMIT_VERSION 0
#define APP_DATA_TRANSMIT_DIR_DN  0
#define APP_DATA_TRANSMIT_DIR_UP  1

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)
#define APP_DATA_TRANSMIT_DIR (APP_DATA_TRANSMIT_DIR_DN)
#define APP_DEST_MAC_ADDR     "\xff\xff\xff\xff\xff\xff"
#endif

#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)
#define APP_DATA_TRANSMIT_DIR (APP_DATA_TRANSMIT_DIR_UP)
#define APP_DEST_MAC_ADDR     "\x00\x00\x00\x00\x00\x00"
#endif

#if defined(PRODUCT_CFG_PRODUCT_TYPE_NDM)
#define APP_DATA_TRANSMIT_DIR (APP_DATA_TRANSMIT_DIR_UP)
#define APP_DEST_MAC_ADDR     "\xff\xff\xff\xff\xff\xff"
#endif

typedef struct {
    td_u8 stru_ver : 7;
    td_u8 dir : 1;
    td_u8 stru_size;
    td_u16 data_length;
    td_u8 data[0];
} app_data_transmit_frame;

#endif

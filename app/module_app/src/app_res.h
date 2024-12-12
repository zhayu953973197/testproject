/*
 * Copyright (c) CompanyNameMagicTag 2018-2020. All rights reserved.
 * Description: APP resource define.
 */

#ifndef __APP_RES_H__
#define __APP_RES_H__

typedef enum {
    APP_TIMER_ID_UART = 0,
    APP_TIMER_ID_INIT = 1,
    APP_TIMER_ID_WHITELIST = 1,
    APP_TIMER_ID_REBOOT,
    APP_TIMER_ID_HLK,
    APP_TIMER_ID_TRANS_FILE_RX,
    APP_TIMER_ID_TRANS_FILE_STOP_RETRY,
    APP_TIMER_ID_TRANS_FILE_STATUS_QRY,
    APP_TIMER_ID_TRANS_FILE_QRY_ACTIVE,
    APP_TIMER_ID_TRANS_FILE_QRY_INFO,
    APP_TIMER_ID_WAIT_PLC_AVAILABLE,
    /* The values of RSP ~ RSP5  must be consecutive */
    APP_TIMER_ID_NI_WAIT_STA_RSP,
    APP_TIMER_ID_NI_WAIT_STA_RSP2,
    APP_TIMER_ID_NI_WAIT_STA_RSP3,
    APP_TIMER_ID_NI_WAIT_STA_RSP4,
    APP_TIMER_ID_NI_WAIT_STA_RSP5,

    APP_TIMER_ID_NI_WAIT_NEW_STA_JOIN,
    APP_TIMER_ID_WRITE_WHITELIST_TO_FLASH,
    APP_TIMER_ID_MAX
} app_timer_id;

typedef enum {
    APP_SEM_ID_UART,
    APP_SEM_ID_MAX
} app_sem_id;

#endif


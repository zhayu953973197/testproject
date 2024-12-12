/*
 * Copyright (c) CompanyNameMagicTag 2018-2021. All rights reserved.
 * Description: Notice!!!: This is a temporary file for FPGA
 */

#include <soc_mdm_spi.h>
#include <dw21.h>
#include <soc_errno.h>
#include <soc_stdlib.h>

#define TX_BUF_TOTAL_LEN 32
#define RX_BUF_TOTAL_LEN 32

td_u16 g_spi_send_buf[TX_BUF_TOTAL_LEN];
td_u16 g_spi_rcv_buf[RX_BUF_TOTAL_LEN];
ext_spi_hw_dev_s g_spi_hw_device[1];
ext_spi_sw_dev_s g_spi_soft_device[1];
ext_spi_ctrl_s g_usr_spi_ctrl;

td_u32 app_spi_init(td_void)
{
    td_u32 ret = EXT_ERR_SUCCESS;

    g_spi_hw_device[0].reg_base = REG_SPI0_BASE_ADDR;
    g_spi_hw_device[0].irq_num = Ssp0_IRQn;
    g_spi_hw_device[0].tx_buf.p_buf = g_spi_send_buf;
    g_spi_hw_device[0].tx_buf.total_len = TX_BUF_TOTAL_LEN;
    g_spi_hw_device[0].rx_buf.p_buf = g_spi_rcv_buf;
    g_spi_hw_device[0].rx_buf.total_len = RX_BUF_TOTAL_LEN;
    g_spi_hw_device[0].data_width_byte = EXT_SPI_SOFT_BUF_DATA_WIDTH_2_BYTES;

    g_usr_spi_ctrl.p_hw_devs = g_spi_hw_device;
    g_usr_spi_ctrl.p_sw_devs = g_spi_soft_device;
    g_usr_spi_ctrl.hw_device_count = 1;
    g_usr_spi_ctrl.sw_device_count = 1;
    ret += uapi_spi_init(&g_usr_spi_ctrl);
    ret += uapi_spi_set_water_line(0, EXT_SPI_TX_FIFO_WATER_LINE_32, EXT_SPI_RX_FIFO_WATER_LINE_32);
    if (sizeof(ext_spi_device_usr_cfg_s) != sizeof(ext_spi_device_inner_cfg_s)) {
        for (;;) {
            printf("ext_spi_device_usr_cfg_s!=ext_spi_device_inner_cfg_s\n");
        }
    }

    return ret;
}
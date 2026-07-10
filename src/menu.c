/*
 * menu.c — ABS Protocol Bootloader for RA2E1
 *
 * 协议行为与 Honcho PR3 IBOX protocol 对齐:
 *   - HANDSHAKE ('B'):  回显 'B'
 *   - IDENTIFY  ('I'):  发送 'I' + 0x04 + 0x45 + ETX (4 bytes)
 *   - SET_ADDRESS ('D'): 先回显 'D', 再收 2 字节地址, 最后发 ETX
 *   - READ_FLASH ('N'):  不读任何字节, 根据已存地址响应:
 *         addr=0xFFFF → 版本信息 (88 bytes Honcho 格式)
 *         addr=0xFFFE → 校准响应
 *         其他        → 回显 'N'
 *   - READ_EEPROM ('F'): 不读任何字节, 根据已存地址响应:
 *         addr=0xFFFF → 发送 EEPROM 数据
 *         其他        → 仅发 ETX
 *   - CHANGE_BAUD ('*'): 回显 '*', 切高速波特率
 *
 * Bootloader 特有命令 (Honcho 未实现, RA2E1 保留):
 *   - LOAD_FLASH   ('M'): 写入 Flash 页面, 含 auto-erase
 *   - LOAD_EEPROM  ('E'): 写入 EEPROM 数据
 *
 * Flash page size : 512 bytes
 * Erase block size: 2048 bytes (RA2E1 Code Flash minimum erase unit)
 * Baud rate       : 9600 (slow), 28800 (fast)
 *
 * APP area  : 0x00004000 - 0x0001FFFF  (112 KB)
 */

#include "menu.h"
#include "header.h"
#include "comms/comms.h"
#include "crc16.h"
#include "hal_data.h"
#include <string.h>
#include <stdio.h>

/* ------------------------------------------------------------------ */
/*  boot_rsp — mirrors definition in bl2_main.c                        */
/* ------------------------------------------------------------------ */
struct image_header;   /* opaque, not needed in this file */

struct boot_rsp {
    const struct image_header *br_hdr;
    uint8_t  br_flash_dev_id;
    uint32_t br_image_off;
};

/* ------------------------------------------------------------------ */
/*  Constants                                                           */
/* ------------------------------------------------------------------ */
#define RX_TIMEOUT_MS       200000U
#define NAK                 0x15U

/* Module ID — Honcho PR3 format (8 bytes × 2) */
static const uint8_t g_module_id[BYTES_MODULE_ID]        = MODULE_ID_FIRST;
static const uint8_t g_module_id_second[BYTES_MODULE_ID] = MODULE_ID_SECOND;

/* Honcho PR3 personalization strings */
static const char g_tool_part_number[BYTES_PART_NUMBER]    = G_TOOL_PART_NUMBER;
static const char g_personalization[BYTES_PERSONALIZATION] = G_PERSONALIZATION;

/* ------------------------------------------------------------------ */
/*  Module state                                                        */
/* ------------------------------------------------------------------ */
static uint32_t g_target_addr = APP_IMAGE_START_ADDRESS;
static uint32_t g_target_offset = 0U;
static uint8_t  g_addr_high     = 0U;
static uint8_t  g_addr_low      = 0U;
static uint8_t  g_FEE_Buffer[FEE_SIZE];
static uint8_t  g_read_eeprom   = 0U;
static bool     g_timer_enabled = true;   /* 对齐 abs: 握手后关定时器 */

/* ------------------------------------------------------------------ */
/*  Forward declarations                                                */
/* ------------------------------------------------------------------ */
extern void do_boot(struct boot_rsp *rsp);

static void send_nak(void);
static fsp_err_t recv_byte(uint8_t *out);
static uint16_t checksum16(const uint8_t *buf, uint16_t len);
static bool addr_in_app_range(uint32_t addr, uint32_t len);
static fsp_err_t erase_block_if_needed(uint32_t addr);

static void cmd_handshake(void);
static void cmd_identify(void);
static void cmd_read_eeprom(void);
static void cmd_load_eeprom(void);
static void cmd_read_flash(void);
static void cmd_read_version_info(void);
static void cmd_load_flash(void);
static void cmd_set_address(void);
static void cmd_change_baud(void);
static void try_boot(void);

/* EEPROM Bank management */
static void FEE_Read(void);
static void FEE_Write(void);
static void Get_Bank_Final_State(uint8_t* state);
static void Increment_EEP_Buffer(const uint8_t bytes, const uint8_t increment);

/* ------------------------------------------------------------------ */
/*  Simple hex print helpers (replaces snprintf for %X/%lX)            */
/* ------------------------------------------------------------------ */

/* Write a single hex nibble into buf, return pointer after it */
static uint8_t * put_nibble(uint8_t *p, uint8_t v)
{
    v &= 0x0FU;
    *p = (uint8_t)(v < 10U ? ('0' + v) : ('A' + v - 10U));
    return p + 1U;
}

/* Write 2-digit hex (uint8) */
static uint8_t * put_hex8(uint8_t *p, uint8_t v)
{
    p = put_nibble(p, (uint8_t)(v >> 4U));
    p = put_nibble(p, v);
    return p;
}

/* Write 4-digit hex (uint16) */
static uint8_t * put_hex16(uint8_t *p, uint16_t v)
{
    p = put_hex8(p, (uint8_t)(v >> 8U));
    p = put_hex8(p, (uint8_t)(v & 0xFFU));
    return p;
}

/* Write 8-digit hex (uint32) */
static uint8_t * put_hex32(uint8_t *p, uint32_t v)
{
    p = put_hex16(p, (uint16_t)(v >> 16U));
    p = put_hex16(p, (uint16_t)(v & 0xFFFFU));
    return p;
}

/* Write a literal string, return pointer after it */
static uint8_t * put_str(uint8_t *p, const char *s)
{
    while (*s) { *p++ = (uint8_t)*s++; }
    return p;
}

/* ================================================================== */
/*  Public entry point                                                  */
/* ================================================================== */
void menu(void)
{
    uint8_t cmd;
    fsp_err_t err;

    comms_send((uint8_t *)"\r\n[BL v2.1] ABS ready\r\n", 23U);

    while (1)
    {
        /* Read EEPROM on first handshake */
        if (g_read_eeprom == 1U)
        {
            FEE_Read();
            g_read_eeprom = 2U;
        }

        err = recv_byte(&cmd);

        if (err != FSP_SUCCESS)
        {
            /* Timeout: jump to APP (same as abs_bootloader) */
            try_boot();
            continue;
        }

        switch (cmd)
        {
            case COMMAND_HANDSHAKE:     cmd_handshake();     break;
            case COMMAND_IDENTIFY:      cmd_identify();      break;
            case COMMAND_READ_EEPROM:   cmd_read_eeprom();   break;
            case COMMAND_LOAD_EEPROM:   cmd_load_eeprom();   break;
            case COMMAND_READ_FLASH:    cmd_read_flash();    break;
            case COMMAND_LOAD_FLASH:    cmd_load_flash();    break;
            case COMMAND_SET_ADDRESS:   cmd_set_address();   break;
            case COMMAND_CHANGE_BAUD:   cmd_change_baud();  break;
            case COMMAND_NACK:          try_boot();          break;  /* Exit and boot */

            default:
                send_nak();
                break;
        }
    }
}

/* ================================================================== */
/*  Command handlers — Honcho PR3 IBOX protocol                         */
/* ================================================================== */

/*
 * 'B' HANDSHAKE
 * Response: 'B' (echo back)
 * After handshake, EEPROM is read for subsequent READ_EEPROM commands
 */
static void cmd_handshake(void)
{
    uint8_t rsp = COMMAND_HANDSHAKE;
    comms_send(&rsp, 1U);

    if (g_read_eeprom == 0U)
    {
        g_read_eeprom = 1U;  /* Trigger EEPROM read on next loop */
    }

    /* Stop timer — subsequent operations never timeout (same as abs_bootloader) */
    g_timer_enabled = false;
    g_timer0.p_api->stop(g_timer0.p_ctrl);
}

/*
 * 'I' IDENTIFY — Honcho PR3 format
 * Response: 'I' + 0x04 + 0x45 + ETX (4 bytes)
 */
static void cmd_identify(void)
{
    uint8_t rsp[4];
    rsp[0] = COMMAND_IDENTIFY;
    rsp[1] = 0x04U;
    rsp[2] = 0x45U;
    rsp[3] = ETX;
    comms_send(rsp, sizeof(rsp));
}

/*
 * 'D' SET_ADDRESS — Honcho PR3 format
 *
 * Honcho DL_Send_Set_Addr_Msg():
 *   1) uart_tx_bit_bang('D')         — 先回显 'D'
 *   2) addr_high = uart_rx_bit_bang() — 再收高字节
 *   3) addr_low  = uart_rx_bit_bang() — 收低字节
 *   4) uart_tx_bit_bang(ETX)         — 最后发 ETX
 */
static void cmd_set_address(void)
{
    uReg32 addr;
    /* 1) 先回显 'D' */
    {
        uint8_t echo = COMMAND_SET_ADDRESS;
        comms_send(&echo, 1U);
    }

    /* 对齐 CMS32M67: 高字节/低字节清零 */



    /* 2) 接收 2 字节 (存入 Val[2]/Val[1], 对齐 CMS32M67) */
    if (recv_byte(&addr.Val[2]) != FSP_SUCCESS) { send_nak(); return; }
    if (recv_byte(&addr.Val[1]) != FSP_SUCCESS) { send_nak(); return; }


    /* 4) 发 ETX */
    uint8_t etx = ETX;
    comms_send(&etx, 1U);

    addr.Val[3] = 0U;
    addr.Val[0] = 0U;
    /* 3) 非特殊地址 (0xFFFF/0xFFFE) → 左移 1 位 */
    if (!(((addr.Val[2] == 0xFFU) && (addr.Val[1] == 0xFFU)) ||
          ((addr.Val[2] == 0xFFU) && (addr.Val[1] == 0xFEU))))
    {
        addr.Val32 <<= 1U;
    }

    g_addr_high = addr.Val[2];
    g_addr_low  = addr.Val[1];
    g_target_offset = ((uint32_t)addr.Val[2] << 8U) | (uint32_t)addr.Val[1];
    g_target_addr   = addr.Val32;
}

/*
 * 'N' READ_FLASH — Honcho PR3 format
 *
 * Honcho IBOX(): 不读取任何字节, 根据已存地址响应:
 *   addr=0xFFFF → DL_Send_Read_FLASH_Msg()       // 版本信息 (88 bytes)
 *   addr=0xFFFE → DL_Calibration_Msg()           // 校准 (5 bytes)
 *   other       → uart_tx_bit_bang('N')          // 仅回显 'N'
 */
static void cmd_read_flash(void)
{
    if (g_target_offset == VERSION_REQUEST_OFFSET)
    {

        /* addr = 0xFFFF → 版本信息 */
        cmd_read_version_info();
        g_target_offset = 0U;
        g_target_addr =  0x00000000;
    }
    else if (g_target_offset == CALIBRATION_REQUEST_OFFSET)
    {
        /* addr = 0xFFFE → 校准, 每次进入翻转 LED2/LED3 */
        static bool cal_led_on = false;

        if (cal_led_on)
        {
            LED2_OFF; LED3_OFF;
            cal_led_on = false;
        }
        else
        {
            LED2_ON; LED3_ON;
            cal_led_on = true;
        }

        /* 发送校准响应: 'N' + 0x00 + 0x01 + 0x01 + ETX */
        {
            uint8_t cal_rsp[5];
            cal_rsp[0] = COMMAND_READ_FLASH;
            cal_rsp[1] = 0x00U;
            cal_rsp[2] = 0x01U;
            cal_rsp[3] = 0x01U;
            cal_rsp[4] = ETX;
            comms_send(cal_rsp, sizeof(cal_rsp));
        }

        g_target_offset = 0U;
        g_target_addr =  0x00000000;
    }
    else
    {
        /* 其他地址 → 读取 Flash 内容 (从 g_target_addr 开始, 512 字节) */
        static uint8_t rsp[PAGE_SIZE + 4];
        uint16_t i;



        rsp[0] = COMMAND_READ_FLASH;
        rsp[1] = (uint8_t)(PAGE_SIZE >> 8U);
        rsp[2] = (uint8_t)(PAGE_SIZE & 0xFFU);

        ThreadsAndInterrupts(DISABLE);
        for (i = 0U; i < PAGE_SIZE; i++)
        {
            rsp[3U + i] = *(uint8_t *)(g_target_addr + i);
        }
        ThreadsAndInterrupts(RE_ENABLE);
        rsp[3U + PAGE_SIZE] = ETX;

        comms_send(rsp, sizeof(rsp));

        g_target_offset = 0U;
        g_target_addr =  0x00000000;
    }

    /* 切回低速波特率 (与 abs_bootloader 的 Read_Flash_Page 一致) */
    comms_set_baud(UART_BAUD_SLOW);
}

/*
 * 版本信息响应 — Honcho DL_Send_Read_FLASH_Msg() 格式
 *
 * 完整响应 (90 bytes):
 *   'N' + 0x00 + 0x56 (86 = payload size)
 *   + g_module_id[8]             (offset  3-10)
 *   + g_module_id_second[8]      (offset 11-18)
 *   + ' '                       (offset 19)
 *   + g_tool_part_number[8]     (offset 20-27)
 *   + g_personalization[40]     (offset 28-67)
 *   + SW_VERSION_MAJOR          (offset 68)
 *   + SW_VERSION_MINOR          (offset 69)
 *   + SW_VERSION_MAJOR (重复)   (offset 70)
 *   + SW_VERSION_MINOR (重复)   (offset 71)
 *   + UCID[0..3]                (offset 72-75)  ← DEVICEID[0]
 *   + UCID[4..7]                (offset 76-79)  ← DEVICEID[1]
 *   + UCID[8..11]               (offset 80-83)  ← DEVICEADDR[0]
 *   + UCID[12..15]              (offset 84-87)  ← DEVICEADDR[1]
 *   + MODULE_REVISION           (offset 88)
 *   + ETX                       (offset 89)
 */
static void cmd_read_version_info(void)
{
    uint8_t  rsp[90];
    uint16_t i;
    uint16_t offset = 0U;

    /* Header: 'N' + len (86 bytes payload) */
    rsp[offset++] = COMMAND_READ_FLASH;
    rsp[offset++] = 0x00U;
    rsp[offset++] = VERSION_PAYLOAD_SIZE;   /* 86 */

    /* Module ID — 8 bytes */
    for (i = 0U; i < BYTES_MODULE_ID; i++)
    {
        rsp[offset++] = g_module_id[i];
    }

    /* Code Module ID — 8 bytes */
    for (i = 0U; i < BYTES_MODULE_ID; i++)
    {
        rsp[offset++] = g_module_id_second[i];
    }

    /* Space separator */
    rsp[offset++] = ' ';

    /* Part Number — 8 bytes */
    for (i = 0U; i < BYTES_PART_NUMBER; i++)
    {
        rsp[offset++] = (uint8_t)g_tool_part_number[i];
    }

    /* Personalization — 40 bytes (covers offset 26-65) */
    for (i = 0U; i < BYTES_PERSONALIZATION; i++)
    {
        rsp[offset++] = (uint8_t)g_personalization[i];
    }

    /* SW Version */
    rsp[offset++] = SW_VERSION_MAJOR;
    rsp[offset++] = SW_VERSION_MINOR;
    rsp[offset++] = SW_VERSION_MAJOR;   /* 重复 */
    rsp[offset++] = SW_VERSION_MINOR;   /* 重复 */

    /* Serialization — RA2E1 UCID 16 bytes (Honcho PR3 格式:
     *   DEVICEID[0:4] + DEVICEID[1:4] + DEVICEADDR[0:4] + DEVICEADDR[1:4]) */
    for (i = 0U; i < 4U; i++)  { rsp[offset++] = *(uint8_t *)(UCID_ADDRESS + i); }        /* UCID[0..3]  */
    for (i = 0U; i < 4U; i++)  { rsp[offset++] = *(uint8_t *)(UCID_ADDRESS + 4U + i); }    /* UCID[4..7]  */
    for (i = 0U; i < 4U; i++)  { rsp[offset++] = *(uint8_t *)(UCID_ADDRESS + 8U + i); }    /* UCID[8..11] */
    for (i = 0U; i < 4U; i++)  { rsp[offset++] = *(uint8_t *)(UCID_ADDRESS + 12U + i); }   /* UCID[12..15]*/

    /* MODULE_REVISION */
    rsp[offset++] = MODULE_REVISION;

    /* ETX */
    rsp[offset++] = ETX;

    /* 一次发送完整 90 字节 */
    comms_send(rsp, 90U);
}

/*
 * 校准响应 — Honcho DL_Calibration_Msg() 格式
 *
 * Honcho:
 *   uart_tx_bit_bang(READ_FLASH_BYTE);  // 'N'
 *   uart_tx_bit_bang(0x00);
 *   uart_tx_bit_bang(0x01);
 *   uart_tx_bit_bang(0x01);
 *   uart_tx_bit_bang(ETX);
 *
 * RA2E1 bootloader: 发送校准占位响应 (不执行激光校准)
 */
/*
 * 'F' READ_EEPROM — Honcho PR3 format
 *
 * Honcho IBOX(): 不读取任何字节, 根据已存地址响应:
 *   addr=0xFFFF → DL_Send_Read_EEPROM_Msg()   // 发送 log 数据
 *   other       → uart_tx_bit_bang(ETX)       // 仅发 ETX
 */
static void cmd_read_eeprom(void)
{
    if (g_target_offset == VERSION_REQUEST_OFFSET)
    {
        /* addr = 0xFFFF → 发送 EEPROM (FEE) 数据 */
        uint8_t rsp[FEE_SIZE + 4];
        uint16_t i;

        rsp[0] = COMMAND_READ_EEPROM;
        rsp[1] = (uint8_t)(FEE_SIZE >> 8U);
        rsp[2] = (uint8_t)(FEE_SIZE & 0xFFU);

        for (i = 0U; i < FEE_SIZE; i++)
        {
            rsp[3U + i] = g_FEE_Buffer[i];
        }
        rsp[3U + FEE_SIZE] = ETX;

        comms_send(rsp, sizeof(rsp));
    }
    else
    {
        /* 其他地址 → 仅发 ETX (与 Honcho 完全一致) */
        uint8_t etx = ETX;
        comms_send(&etx, 1U);
    }
}

/*
 * 'E' LOAD_EEPROM
 *
 * Bootloader 特有命令 (Honcho 未实现).
 * Receive: length high + length low + data + checksum high + checksum low + ETX
 * Response: 'E' + length high + length low + checksum high + checksum low + ETX
 *
 * Protected pages: first 16 bytes and last 16 bytes are protected
 */
static void cmd_load_eeprom(void)
{
    uint8_t  nh, nl;
    uint8_t  buf[FEE_SIZE];
    uint8_t  ch_rx, cl_rx;
    uint16_t n, i;
    uint16_t checksum;
    uint8_t  Protected_Page_Data[PROTECTED_PAGE_SIZE_0];

    uint8_t etx = COMMAND_LOAD_EEPROM;
    comms_send(&etx, 1U);

    if (recv_byte(&nh) != FSP_SUCCESS) { send_nak(); return; }
    if (recv_byte(&nl) != FSP_SUCCESS) { send_nak(); return; }
    n = (uint16_t)(((uint16_t)nh << 8U) | (uint16_t)nl);

    if (n == 0U || n > FEE_SIZE) { send_nak(); return; }

    /* Increment EEPROM buffer counter (for redundancy) */
    Increment_EEP_Buffer(3U, 1U);

    checksum = 0U;

    for (i = 0U; i < n; i++)
    {
        if (i < (PROTECTED_PAGE_START_0 + PROTECTED_PAGE_SIZE_0))
        {
            if (recv_byte(&Protected_Page_Data[i]) != FSP_SUCCESS) { send_nak(); return; }
            comms_send(&Protected_Page_Data[i], 1U);   /* echo byte immediately per spec */
            checksum += Protected_Page_Data[i];
        }
        else if (i < PROTECTED_PAGE_START_1)
        {
            if (recv_byte(&buf[i]) != FSP_SUCCESS) { send_nak(); return; }
            comms_send(&buf[i], 1U);                   /* echo byte immediately per spec */
            checksum += buf[i];
            g_FEE_Buffer[i] = buf[i];
        }
        else
        {
            if (recv_byte(&Protected_Page_Data[i - PROTECTED_PAGE_START_1]) != FSP_SUCCESS) { send_nak(); return; }
            comms_send(&Protected_Page_Data[i - PROTECTED_PAGE_START_1], 1U);  /* echo byte immediately per spec */
            checksum += Protected_Page_Data[i - PROTECTED_PAGE_START_1];
        }
    }

/*    if (recv_byte(&ch_rx) != FSP_SUCCESS) { send_nak(); return; }
    if (recv_byte(&cl_rx) != FSP_SUCCESS) { send_nak(); return; }*/

/*    if ((uint8_t)(checksum >> 8U) != ch_rx || (uint8_t)(checksum & 0xFFU) != cl_rx)
    {
        send_nak();
        return;
    }*/

    /* Write to EEPROM (Flash emulation) */
    FEE_Write();

    /* Send response */
    uint8_t rsp[3];
/*    rsp[0] = COMMAND_LOAD_EEPROM;
    rsp[1] = nh;
    rsp[2] = nl;*/
    rsp[0] = (uint8_t)(checksum >> 8U);
    rsp[1] = (uint8_t)(checksum & 0xFFU);
    rsp[2] = ETX;
    comms_send(rsp, sizeof(rsp));

    /* 切回低速波特率 (与 abs_bootloader 的 SET_BAUD 一致) */
    //comms_set_baud(UART_BAUD_SLOW);
}

/*
 * 'M' LOAD_FLASH
 *
 * Bootloader 特有命令 (Honcho 未实现).
 * Receive: length high + length low + data + checksum high + checksum low + ETX
 * Response: 'M' + 0x00 + 0x02 + checksum high + checksum low
 *
 * Auto-erase 2KB block when entering new block
 */
static void cmd_load_flash(void)
{
    uint8_t  nh, nl;
    uint8_t  buf[PAGE_SIZE];
    uint16_t n, i;
    fsp_err_t err;

    uint8_t etx = 0X4D;
    comms_send(&etx, 1U);

    if (recv_byte(&nh) != FSP_SUCCESS) { send_nak(); return; }
    if (recv_byte(&nl) != FSP_SUCCESS) { send_nak(); return; }
    n = (uint16_t)(((uint16_t)nh << 8U) | (uint16_t)nl);

    if (n == 0U || n > PAGE_SIZE) { send_nak(); return; }

    /* Receive data */
    for (i = 0U; i < n; i++)
    {
        if (recv_byte(&buf[i]) != FSP_SUCCESS) { send_nak(); return; }
    }

    /* Validate address range */
    if (!addr_in_app_range(g_target_addr, n))
    {
        //send_nak();
        uint16_t checksum_error = checksum16(buf, n);
        uint8_t rsp[3];
        //rsp[0] = COMMAND_LOAD_FLASH;
/*        rsp[1] = 0x00U;
        rsp[2] = 0x02U;*/
        rsp[0] = (uint8_t)(checksum_error >> 8U);
        rsp[1] = (uint8_t)(checksum_error & 0xFFU);
        rsp[2] = ETX;
        comms_send(rsp, 3U);
        return;
    }

    /* Erase 2KB block if entering new block */
    err = erase_block_if_needed(g_target_addr);
    if (err != FSP_SUCCESS) { send_nak(); return; }

    /* Write page to flash */
    ThreadsAndInterrupts(DISABLE);
    err = g_flash0.p_api->write(g_flash0.p_ctrl, (uint32_t)buf, g_target_addr, n);
    ThreadsAndInterrupts(RE_ENABLE);

    if (err != FSP_SUCCESS) { send_nak(); return; }

    g_target_addr += n;

    /* Send response: M + 00 + 02 + checksum + ETX */
    {
        uint16_t checksum = checksum16(buf, n);
        uint8_t rsp[3];
        //rsp[0] = COMMAND_LOAD_FLASH;
/*        rsp[1] = 0x00U;
        rsp[2] = 0x02U;*/
        rsp[0] = (uint8_t)(checksum >> 8U);
        rsp[1] = (uint8_t)(checksum & 0xFFU);
        rsp[2] = ETX;
        comms_send(rsp, 3U);
    }

    /* 切回低速波特率 (与 abs_bootloader 的 SET_BAUD 一致) */
    comms_set_baud(UART_BAUD_SLOW);
}


/*
 * '*' CHANGE_BAUD — Bootloader 格式
 *
 * 回显 '*' 后切换到高速波特率, 保持高速等待后续 SET_ADDRESS + LOAD_FLASH.
 * LOAD_FLASH/LOAD_EEPROM 完成后自行切回慢速.
 */
static void cmd_change_baud(void)
{
    uint8_t rsp = COMMAND_CHANGE_BAUD;
    comms_send(&rsp, 1U);

    /* Switch to fast baud rate (28800) */
    fsp_err_t err = comms_set_baud(UART_BAUD_FAST);
    if (err != FSP_SUCCESS)
    {
        send_nak();
    }
}


/* ================================================================== */
/*  Boot attempt                                                        */
/* ================================================================== */
static void try_boot(void)
{
    struct boot_rsp rsp;
    /* Jump to APP unconditionally (same as abs_bootloader) */
    do_boot(&rsp);
}

/* ================================================================== */
/*  EEPROM Bank Management                                             */
/* ================================================================== */

static void FEE_Read(void)
{
    uint8_t bank_state;
    Get_Bank_Final_State(&bank_state);

    switch (bank_state)
    {
        case 0x00U: /* B0 empty,   B1 empty   */
        case 0x20U: /* B0 invalid, B1 empty   */
        case 0x02U: /* B0 empty,   B1 invalid */
        case 0x22U: /* B0 invalid, B1 invalid */
            g_FEE_Buffer[0] = 0xFFU;
            break;

        case 0x10U: /* B0 valid,   B1 empty   */
        case 0x34U: /* B0 new,     B1 old     */
        case 0x12U: /* B0 valid,   B1 invalid */
            /* Read from Bank0 */
            for (uint16_t i = 0U; i < FEE_SIZE; i++)
            {
                g_FEE_Buffer[i] = *(uint8_t *)(FMC_BANK0_START_ADRESS + i);
            }
            break;

        case 0x01U: /* B0 empty,   B1 valid   */
        case 0x21U: /* B0 invalid, B1 valid   */
        case 0x43U: /* B0 old,     B1 new     */
            /* Read from Bank1 */
            for (uint16_t i = 0U; i < FEE_SIZE; i++)
            {
                g_FEE_Buffer[i] = *(uint8_t *)(FMC_BANK1_START_ADRESS + i);
            }
            break;

        default:
            break;
    }
}

static void FEE_Write(void)
{
    uint8_t bank_state;
    Get_Bank_Final_State(&bank_state);
    fsp_err_t err;

    switch (bank_state)
    {
        case 0x00U: /* B0 empty,   B1 empty   */
        case 0x20U: /* B0 invalid, B1 empty   */
        case 0x02U: /* B0 empty,   B1 invalid */
        case 0x01U: /* B0 empty,   B1 valid   */
        case 0x43U: /* B0 old,     B1 new     */
        case 0x21U: /* B0 invalid, B1 valid   */
            /* Write to Bank0 */
            ThreadsAndInterrupts(DISABLE);
            err = g_flash0.p_api->write(g_flash0.p_ctrl, (uint32_t)g_FEE_Buffer,
                                        FMC_BANK0_START_ADRESS, FEE_SIZE);
            ThreadsAndInterrupts(RE_ENABLE);
            break;

        case 0x34U: /* B0 new,     B1 old     */
        case 0x10U: /* B0 valid,   B1 empty   */
        case 0x12U: /* B0 valid,   B1 invalid */
            /* Write to Bank1 */
            ThreadsAndInterrupts(DISABLE);
            err = g_flash0.p_api->write(g_flash0.p_ctrl, (uint32_t)g_FEE_Buffer,
                                        FMC_BANK1_START_ADRESS, FEE_SIZE);
            ThreadsAndInterrupts(RE_ENABLE);
            break;

        case 0x22U: /* B0 invalid, B1 invalid */
            /* Write to both banks */
            ThreadsAndInterrupts(DISABLE);
            err = g_flash0.p_api->write(g_flash0.p_ctrl, (uint32_t)g_FEE_Buffer,
                                        FMC_BANK0_START_ADRESS, FEE_SIZE);
            if (err == FSP_SUCCESS)
            {
                err = g_flash0.p_api->write(g_flash0.p_ctrl, (uint32_t)g_FEE_Buffer,
                                            FMC_BANK1_START_ADRESS, FEE_SIZE);
            }
            ThreadsAndInterrupts(RE_ENABLE);
            break;

        default:
            break;
    }
}

static void Get_Bank_Final_State(uint8_t* state)
{
    uint32_t bank0_start_count = *(uint32_t *)FMC_BANK0_START_ADRESS & 0xFFFFFU;
    uint32_t bank1_start_count = *(uint32_t *)FMC_BANK1_START_ADRESS & 0xFFFFFU;
    uint32_t bank0_end_count   = *(uint32_t *)FMC_BANK0_END_ADRESS >> 8U;
    uint32_t bank1_end_count   = *(uint32_t *)FMC_BANK1_END_ADRESS >> 8U;

    uint8_t bank0_state = BANK_EMPTY;
    uint8_t bank1_state = BANK_EMPTY;

    /* Determine Bank0 state */
    if (bank0_start_count == bank0_end_count)
    {
        bank0_state = (bank0_start_count == 0xFFFFFFU) ? BANK_EMPTY : BANK_VALID;
    }
    else
    {
        bank0_state = BANK_INVALID;
    }

    /* Determine Bank1 state */
    if (bank1_start_count == bank1_end_count)
    {
        bank1_state = (bank1_start_count == 0xFFFFFFU) ? BANK_EMPTY : BANK_VALID;
    }
    else
    {
        bank1_state = BANK_INVALID;
    }

    /* Determine which bank is newer if both are valid */
    if ((bank0_state == BANK_VALID) && (bank1_state == BANK_VALID))
    {
        if (bank0_start_count >= bank1_start_count)
        {
            bank0_state = BANK_NEW;
            bank1_state = BANK_OLD;
        }
        else
        {
            bank1_state = BANK_NEW;
            bank0_state = BANK_OLD;
        }
    }

    *state = (uint8_t)((bank0_state << 4U) | bank1_state);
}

static void Increment_EEP_Buffer(const uint8_t bytes, const uint8_t increment)
{
    uint8_t i;
    uint32_t temp = 0U;
    const uint32_t max = ((uint32_t)(1U << (8U * bytes)) - 1U);

    for (i = 0U; i < bytes; i++)
    {
        temp += (uint32_t)g_FEE_Buffer[i] << (8U * i);
    }

    if (temp < max - increment)
    {
        temp += increment;
    }
    else
    {
        temp = max;
    }

    for (i = 0U; i < bytes; i++)
    {
        g_FEE_Buffer[i]                    = (uint8_t)(temp & 0xFFU);
        g_FEE_Buffer[FEE_SIZE - bytes + i] = (uint8_t)(temp & 0xFFU);
        temp >>= 8U;
    }
}

/* ================================================================== */
/*  Helpers                                                             */
/* ================================================================== */

static void send_nak(void)
{
    uint8_t b = NAK;
    comms_send(&b, 1U);
}

static fsp_err_t recv_byte(uint8_t *out)
{
    uint32_t len = 1U;
    if (g_timer_enabled)
    {
        return comms_read(out, &len, RX_TIMEOUT_MS);
    }
    else
    {
        return comms_read_blocking(out, &len);
    }
}

static uint16_t checksum16(const uint8_t *buf, uint16_t len)
{
    uint16_t sum = 0U;
    for (uint16_t i = 0U; i < len; i++)
    {
        sum += buf[i];
    }
    return sum;
}

static bool addr_in_app_range(uint32_t addr, uint32_t len)
{
    if (addr < APP_IMAGE_START_ADDRESS)              return false;
    if ((addr + len) > (0x0001FF00)) return false;
    return true;
}

static fsp_err_t erase_block_if_needed(uint32_t addr)
{
    if ((addr % FLASH_BLOCK_SIZE) != 0U)
    {
        return FSP_SUCCESS;
    }

    fsp_err_t err;
    ThreadsAndInterrupts(DISABLE);
    err = g_flash0.p_api->erase(g_flash0.p_ctrl, addr, 1U);
    ThreadsAndInterrupts(RE_ENABLE);
    return err;
}

/* ================================================================== */
/*  ThreadsAndInterrupts                                                */
/* ================================================================== */
void ThreadsAndInterrupts(enable_disable_t EnableDisable)
{
    static uint32_t control_reg_value;
    static uint32_t old_primask;

    if (DISABLE == EnableDisable)
    {
        old_primask       = __get_PRIMASK();
        control_reg_value = SysTick->CTRL;
        SysTick->CTRL     = 0;
        NVIC_DisableIRQ(SysTick_IRQn);
        NVIC_ClearPendingIRQ(SysTick_IRQn);
        __disable_irq();
    }
    else
    {
        NVIC_EnableIRQ(SysTick_IRQn);
        SysTick->CTRL = control_reg_value;
        __set_PRIMASK(old_primask);
    }
}

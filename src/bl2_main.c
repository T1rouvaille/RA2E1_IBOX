/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 * Copyright (c) 2017-2020 Arm Limited.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "hal_data.h"
#include "comms/comms.h"
#include "menu.h"
#include "header.h"

/* Avoids the semihosting issue */
#if defined (__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
__asm("  .global __ARM_use_no_argv\n");
#endif

#if defined(__ARM_ARCH_8M_MAIN__) || defined(__ARM_ARCH_8M_BASE__)
/* Macros to pick linker symbols */
#define REGION(a, b, c) a##b##c
#define REGION_NAME(a, b, c) REGION(a, b, c)
#define REGION_DECLARE(a, b, c) extern uint32_t REGION_NAME(a, b, c)

REGION_DECLARE(Image$$, ARM_LIB_STACK, $$ZI$$Base);
#endif

struct arm_vector_table {
    uint32_t msp;
    uint32_t reset;
};

struct boot_rsp {
    const struct image_header *br_hdr;
    uint8_t br_flash_dev_id;
    uint32_t br_image_off;
};

int bl2_main(void);
void do_boot(struct boot_rsp *rsp);

/*!
 * \brief Chain-loading the next image in the boot sequence.
 *
 * 跳转流程:
 *   1) 读取 APP 起始地址的向量表 (MSP, Reset_Handler)
 *   2) 设置 VTOR 指向 APP 向量表 (使 APP 中断能正常工作)
 *   3) 设置 MSP 并跳转到 APP Reset_Handler
 */
void do_boot(struct boot_rsp *rsp)
{
    FSP_PARAMETER_NOT_USED(rsp);

    uint32_t app_msp;
    uint32_t app_reset;
    uint32_t flash_base;

    flash_base = APP_IMAGE_START_ADDRESS;

    /* 从 APP 的 Flash 区域读取向量表 */
    app_msp   = *(uint32_t *)(flash_base);
    app_reset = *(uint32_t *)(flash_base + 4);

#if MCUBOOT_LOG_LEVEL > MCUBOOT_LOG_LEVEL_OFF
    stdio_uninit();
#endif

#if defined(__ARM_ARCH_8M_MAIN__) || defined(__ARM_ARCH_8M_BASE__)
    __set_MSPLIM(0);
#endif

    /* 设置 VTOR 指向 APP 向量表 (修复: 原来指向了栈上的局部变量) */
    SCB->VTOR = (flash_base & SCB_VTOR_TBLOFF_Msk);
    __DSB();

#if BSP_FEATURE_BSP_HAS_SP_MON
    R_MPU_SPMON->SP[0].CTL = 0;
    while (R_MPU_SPMON->SP[0].CTL != 0) { ; }
#endif

    /* 设置 MSP 并跳转到 APP */
    __set_MSP(app_msp);
    ((void (*)()) app_reset)();

    /* 不应到达此处 */
    while (1) { ; }
}

/*!
 * \brief Bootloader 第二阶段主入口.
 *
 * 所有模块 (UART, Timer, Flash) 已在 hal_entry() 中完成初始化,
 * 此处直接进入命令处理循环.
 */
int bl2_main(void)
{
    struct boot_rsp rsp;

    /* 进入协议命令处理主循环, 超时或收到 NACK 后尝试跳转 APP */
    menu();

    do_boot(&rsp);
    return 0;
}

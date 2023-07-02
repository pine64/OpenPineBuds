/***************************************************************************
 *
 * Copyright 2015-2019 BES.
 * All rights reserved. All unpublished rights reserved.
 *
 * No part of this work may be used or reproduced in any form or by any
 * means, or stored in a database or retrieval system, without prior written
 * permission of BES.
 *
 * Use of this work is governed by a license granted by BES.
 * This work contains confidential and proprietary information of
 * BES. which is protected by copyright, trade secret,
 * trademark and other intellectual property rights.
 *
 ****************************************************************************/

#include "plat_addr_map.h"

#define __TTB_SIZE                  0x5000

#define __HEAP_SIZE                 0x1000
#define __STACK_SIZE                0x1000
#define __FIQ_STACK_SIZE            0
#define __IRQ_STACK_SIZE            0x1000
#define __SVC_STACK_SIZE            0
#define __ABT_STACK_SIZE            0
#define __UND_STACK_SIZE            0

/* TODO: Add FLASH_REGION_OFFSET if there are multiple levels of boot loaders */
#if !defined(OTA_BOOT_SIZE) && defined(OTA_CODE_OFFSET)
#define FLASH_REGION_BASE           (FLASH_BASE + OTA_CODE_OFFSET)
#else
#define FLASH_REGION_BASE           (FLASH_BASE)
#endif

#ifndef FLASH_REGION_SIZE
#define FLASH_REGION_SIZE           (FLASH_SIZE - (FLASH_REGION_BASE - FLASH_BASE))
#endif

#ifdef EXEC_IN_RAM
#define EXEC_REGION                 RAM
#elif defined(EXEC_IN_PSRAM)
#define EXEC_REGION                 PSRAM
#else
#define EXEC_REGION                 PSRAMUHS
#endif

#define PSRAMUHSX_SECTION_SIZE      0x100000

MEMORY
{
    BOOT        (rwx)   : ORIGIN = DSP_BOOT_BASE,           LENGTH = DSP_BOOT_SIZE
    FLASH       (rx)    : ORIGIN = FLASH_REGION_BASE,       LENGTH = FLASH_REGION_SIZE
    RAM         (rwx)   : ORIGIN = DSP_RAM_BASE,            LENGTH = DSP_RAM_SIZE - __TTB_SIZE
    L_TTB       (rw)    : ORIGIN = DSP_RAM_BASE + DSP_RAM_SIZE - __TTB_SIZE,    LENGTH = __TTB_SIZE
#ifdef PSRAM_BASE
    PSRAM       (rwx)   : ORIGIN = PSRAM_BASE,              LENGTH = PSRAM_SIZE
#endif
#ifdef PSRAMUHS_BASE
    PSRAMUHS    (rwx)   : ORIGIN = PSRAMUHS_BASE,           LENGTH = PSRAMUHS_SIZE - PSRAMUHSX_SECTION_SIZE
    PSRAMUHSX   (rx)    : ORIGIN = PSRAMUHSX_BASE+PSRAMUHS_SIZE-PSRAMUHSX_SECTION_SIZE,         LENGTH = PSRAMUHSX_SECTION_SIZE
#endif
}

/* Library configurations */
GROUP(libgcc.a)

#ifdef GEN_BOOT_SECTION
ENTRY(Vectors)
#else
ENTRY(Reset_Handler)
#endif

SECTIONS
{
#ifdef GEN_BOOT_SECTION

    .boot :
    {
        KEEP(*(.vectors))
    } > BOOT

    /DISCARD/ :
    {
        *(*)
    }

#else // !GEN_BOOT_SECTION

    .boot_loader :
    {
        *(.boot_loader)
        *(.boot_text_flash*)

        . = ALIGN(4);
    } > EXEC_REGION

    .sram_text :
    {
        __sram_text_data_start__ = .;
        /* memcpy.o or libc_nano.a(lib_a-memcpy.o/lib_a-memcpy-stub.o) */
        *:memcpy.o(.text*)
        *:lib_a-memcpy*.o(.text*)
        *:libc_rom.o(.text*)

        /* memset.o or libc_nano.a(lib_a-memset.o) */
        *:memset.o(.text*)
        *:lib_a-memset*.o(.text*)

        *(.boot_text_sram*)
        *(.sram_text*)
        *(.fast_text_sram*)

        . = ALIGN(4);
    } > RAM AT > EXEC_REGION

    __sram_text_data_start_load__ = LOADADDR(.sram_text);

    .sram_data :
    {
        *(.boot_data*)
        . = ALIGN(4);
        *(.sram_data*)
        . = ALIGN(4);
    } > RAM AT > EXEC_REGION

    __sram_text_data_end_load__ = LOADADDR(.sram_data) + SIZEOF(.sram_data);

    .text :
    {
        *(SVC_TABLE)
        *(.text*)
        *(.flash_text*)

        KEEP(*(.init))
        KEEP(*(.fini))

        /* .ctors */
        *crtbegin.o(.ctors)
        *crtbegin?.o(.ctors)
        *(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
        *(SORT(.ctors.*))
        *(.ctors)

        /* .dtors */
        *crtbegin.o(.dtors)
        *crtbegin?.o(.dtors)
        *(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
        *(SORT(.dtors.*))
        *(.dtors)

        . = ALIGN(4);

        __rodata_start = .;
        *(.rodata*)
        *(.boot_rodata*)
        __rodata_end = .;

        . = ALIGN(4);

        KEEP(*(.eh_frame*))
        KEEP(*(.build_info))

        *(.note.gnu.build-id)

        . = ALIGN(4);
    } > EXEC_REGION

    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > EXEC_REGION

    __exidx_start = .;
    .ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > EXEC_REGION
    __exidx_end = .;

    .copy.table :
    {
        . = ALIGN(4);
        __copy_table_start__ = .;
        LONG (__etext)
        LONG (__data_start__)
        LONG (__data_end__ - __data_start__)
        __copy_table_end__ = .;
    } > EXEC_REGION

    .zero.table :
    {
        . = ALIGN(4);
        __zero_table_start__ = .;
        LONG (__bss_start__)
        LONG (__bss_end__ - __bss_start__)
        __zero_table_end__ = .;
    } > EXEC_REGION

    __etext = .;

    .data :
    {
        Image$$RW_DATA$$Base = .;
        __data_start__ = .;
        *(.data*)
        Image$$RW_DATA$$Limit = .;

        . = ALIGN(4);
        /* preinit data */
        PROVIDE (__preinit_array_start = .);
        KEEP(*(.preinit_array))
        PROVIDE (__preinit_array_end = .);

        . = ALIGN(4);
        /* init data */
        PROVIDE (__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE (__init_array_end = .);


        . = ALIGN(4);
        /* finit data */
        PROVIDE (__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array))
        PROVIDE (__fini_array_end = .);

        . = ALIGN(4);
        /* All data end */
        __data_end__ = .;

    } > EXEC_REGION

    .sram_bss (NOLOAD) : AT (ADDR(.sram_bss))
    {
        __sram_bss_start__ = .;
        *(.boot_bss*)
        . = ALIGN(4);
        *(.sram_bss*)
        . = ALIGN(4);
        __sram_bss_end__ = .;
    } > RAM

#ifndef EXEC_IN_RAM
    __ram_end = .;
#endif

    .bss ALIGN(0x400) (NOLOAD) :
    {
        Image$$ZI_DATA$$Base = .;
        __bss_start__ = .;
        *(.bss*)
        *(COMMON)
        __bss_end__ = .;
        Image$$ZI_DATA$$Limit = .;
        __end__ = .;
        end = __end__;
    } > EXEC_REGION

#if defined(__HEAP_SIZE) && (__HEAP_SIZE > 0)
    .heap (NOLOAD) :
    {
        . = ALIGN(8);
        Image$$HEAP$$ZI$$Base = .;
        . += __HEAP_SIZE;
        Image$$HEAP$$ZI$$Limit = .;
        __HeapLimit = .;
    } > EXEC_REGION
#endif

    .sync_flags ALIGN(0x1000) (NOLOAD) :
    {
        __sync_flags_start = .;
        *(.sync_flags*)
        . = ALIGN(0x1000);
        __sync_flags_end = .;
    } > EXEC_REGION

#ifdef EXEC_IN_RAM
    __ram_end = .;
#else
    __free_exec_region = ORIGIN(EXEC_REGION) + LENGTH(EXEC_REGION) - .;
#endif

    __psramuhs_text_data_start_load__ = LOADADDR(.data) + SIZEOF(.data);
    .psramuhs_text (ORIGIN(PSRAMUHSX)) : AT (__psramuhs_text_data_start_load__)
    {
        __psramuhs_text_start = PSRAMUHSX_TO_PSRAMUHS(.);
        *(.psramuhs_text*)
        . = ALIGN(4);
        __psramuhs_text_end = PSRAMUHSX_TO_PSRAMUHS(.);
    } > PSRAMUHSX
    __psramuhs_text_data_end_load__ = __psramuhs_text_data_start_load__ + SIZEOF(.psramuhs_text);

    /* The following section MUST be the last loaded section */
    .code_start_addr : AT (__psramuhs_text_data_end_load__)
    {
        LONG(ADDR(.boot_loader));
    } > EXEC_REGION

    .stack (ORIGIN(RAM) + LENGTH(RAM) - __STACK_SIZE - __FIQ_STACK_SIZE - __IRQ_STACK_SIZE - __SVC_STACK_SIZE - __ABT_STACK_SIZE - __UND_STACK_SIZE) (NOLOAD) : AT (ADDR(.stack))
    {
        . = ALIGN(8);

        __StackTop = .;
        Image$$SYS_STACK$$ZI$$Base = .;
        . += __STACK_SIZE;
        Image$$SYS_STACK$$ZI$$Limit = .;
        __stack = .;

        Image$$FIQ_STACK$$ZI$$Base = .;
        . += __FIQ_STACK_SIZE;
        Image$$FIQ_STACK$$ZI$$Limit = .;

        Image$$IRQ_STACK$$ZI$$Base = .;
        . += __IRQ_STACK_SIZE;
        Image$$IRQ_STACK$$ZI$$Limit = .;

        Image$$SVC_STACK$$ZI$$Base = .;
        . += __SVC_STACK_SIZE;
        Image$$SVC_STACK$$ZI$$Limit = .;

        Image$$ABT_STACK$$ZI$$Base = .;
        . += __ABT_STACK_SIZE;
        Image$$ABT_STACK$$ZI$$Limit = .;

        Image$$UND_STACK$$ZI$$Base = .;
        . += __UND_STACK_SIZE;
        Image$$UND_STACK$$ZI$$Limit = .;
    } > RAM

    __free_ram = ADDR(.stack) - __ram_end;
#ifdef EXEC_IN_RAM
    __free_exec_region = __free_ram;
#endif

    .ttb_l2 (NOLOAD) :
    {
        Image$$TTB$$ZI$$Base = .;
        *(.ttb_l2)
    } > L_TTB

    .ttb_l1 (NOLOAD) :
    {
        *(.ttb_l1)
        Image$$TTB$$ZI$$Limit = .;
    } > L_TTB

    __free_ttb = ORIGIN(L_TTB) + LENGTH(L_TTB) - .;

#endif // !GEN_BOOT_SECTION
}


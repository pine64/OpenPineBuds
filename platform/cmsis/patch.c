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
#ifdef __ARM_ARCH_8M_MAIN__
#include "patch.h"
#include "reg_patch.h"

#ifdef PATCH_CTRL_BASE

static struct PATCH_CTRL_T * const patch_ctrl = (struct PATCH_CTRL_T *)PATCH_CTRL_BASE;
static struct PATCH_DATA_T * const patch_data = (struct PATCH_DATA_T *)PATCH_DATA_BASE;

static uint32_t patch_map[(PATCH_ENTRY_NUM + 31) / 32];

int patch_open(uint32_t remap_addr)
{
    patch_ctrl->CTRL[0] |= PATCH_CTRL_GLOBAL_EN;
    return 0;
}

int patch_code_enable_id(uint32_t id, uint32_t addr, uint32_t data)
{
    if (id >= PATCH_ENTRY_NUM) {
        return 1;
    }

    patch_data->DATA[id] = data;
    patch_ctrl->CTRL[id] = (patch_ctrl->CTRL[id] & ~PATCH_CTRL_ADDR_MASK) | PATCH_CTRL_ADDR(addr) | PATCH_CTRL_ENTRY_EN;
    return 0;
}

int patch_code_disable_id(uint32_t id)
{
    if (id >= PATCH_ENTRY_NUM) {
        return 1;
    }

    patch_ctrl->CTRL[id] &= ~PATCH_CTRL_ENTRY_EN;
    return 0;
}

int patch_data_enable_id(uint32_t id, uint32_t addr, uint32_t data)
{
    return patch_code_enable_id(id, addr, data);
}

int patch_data_disable_id(uint32_t id)
{
    return patch_code_disable_id(id);
}

static uint32_t func_patch_ins(uint32_t old_func, uint32_t new_func)
{
#if defined(__ARM_ARCH_7EM__) || defined(__ARM_ARCH_8M_MAIN__)
    union {
        uint32_t d32;
        uint16_t d16[2];
    } ins;
    uint32_t immd;
    uint8_t j1, j2, s;

    ins.d32 = 0x9000F000;
    immd = (new_func & ~1) - ((old_func + 4) & ~1);
    s = immd >> 31;
    j1 = s ^ !((immd >> 23) & 1);
    j2 = s ^ !((immd >> 22) & 1);
    ins.d16[0] |= (s << 10) | ((immd >> 12) & 0x3FF);
    ins.d16[1] |= (j1 << 13) | (j2 << 11) | ((immd >> 1) & 0x7FF);

    return ins.d32;
#else
#error "Only ARMv7-M/ARMv8-M function can be patched"
#endif
}

PATCH_ID patch_enable(enum PATCH_TYPE_T type, uint32_t addr, uint32_t data)
{
    uint8_t id, idfunc;
    uint8_t cnt;
    uint32_t bit;
    uint32_t offset;

    if (addr < ROM_BASE || addr >= (ROM_BASE + 0x20000000)) {
        // Not in code region
        return -1;
    }
    if (type == PATCH_TYPE_FUNC) {
        if (data >= addr) {
            offset = data - addr;
        } else {
            offset = addr - data;
        }
        if (offset >= 0x01000000) {
            // Branch distance too long to fit in a 32-bit branch instruction
            return -2;
        }
    }

    if (!(type == PATCH_TYPE_CODE || type == PATCH_TYPE_FUNC || type == PATCH_TYPE_DATA)) {
        return -3;
    }

    for (id = 0; id < PATCH_ENTRY_NUM; id++) {
        cnt = id / 32;
        bit = (1 << (id % 32));
        if ((patch_map[cnt] & bit) == 0) {
            patch_map[cnt] |= bit;
            break;
        }
    }
    if (id >= PATCH_ENTRY_NUM) {
        return -4;
    }

    idfunc = 0xFF;

    if (type == PATCH_TYPE_FUNC && (addr & 2)) {
        for (idfunc = id; idfunc < PATCH_ENTRY_NUM; idfunc++) {
            cnt = idfunc / 32;
            bit = (1 << (idfunc % 32));
            if ((patch_map[cnt] & bit) == 0) {
                patch_map[cnt] |= bit;
                break;
            }
        }
        if (idfunc >= PATCH_ENTRY_NUM) {
            // Release previous patch
            patch_map[cnt] &= ~bit;
            return -5;
        }
    }

    if (type == PATCH_TYPE_CODE) {
        patch_code_enable_id(id, addr, data);
    } else if (type == PATCH_TYPE_DATA) {
        patch_data_enable_id(id, addr, data);
    } else if (type == PATCH_TYPE_FUNC) {
        uint32_t ins;

        ins = func_patch_ins(addr, data);

        if (addr & 2) {
            uint32_t real_addr[2];
            uint32_t real_data[2];

            real_addr[0] = addr & ~3;
            real_addr[1] = (addr + 4) & ~3;

            real_data[0] = *(volatile uint32_t *)real_addr[0];
            real_data[1] = *(volatile uint32_t *)real_addr[1];
            real_data[0] = (real_data[0] & 0x0000FFFF) | ((ins & 0xFFFF) << 16);
            real_data[1] = (real_data[1] & 0xFFFF0000) | ((ins >> 16) & 0xFFFF);

            patch_code_enable_id(id, real_addr[0], real_data[0]);
            patch_code_enable_id(idfunc, real_addr[1], real_data[1]);
        } else {
            patch_code_enable_id(id, addr, ins);
        }
    }

    return ((id + 1) & 0xFF) | (((idfunc + 1) & 0xFF) << 8);
}

int patch_disable(PATCH_ID patch_id)
{
    uint8_t id;
    uint8_t cnt, bit;
    int i;

    for (i = 0; i < 2; i++) {
        id = (uint8_t)(patch_id >> (8 * i)) - 1;

        if (id == 0xFF) {
            return 0;
        } else if (id >= PATCH_ENTRY_NUM) {
            return -1;
        }

        patch_code_disable_id(id);

        cnt = id / 32;
        bit = (1 << (id % 32));
        patch_map[cnt] &= ~bit;
    }

    return 0;
}

void patch_close(void)
{
    int i;

    patch_ctrl->CTRL[0] &= ~PATCH_CTRL_GLOBAL_EN;

    for (i = 0; i < PATCH_ENTRY_NUM; i++) {
        patch_code_disable_id(i);
    }

    for (i = 0; i < ARRAY_SIZE(patch_map); i++) {
        patch_map[i] = 0;
    }
}
#endif
#endif

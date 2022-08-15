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
#if !defined(NEW_NV_RECORD_ENABLED)

#include <assert.h>
#include "nvrecord.h"
#include "nvrecord_ble.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "co_math.h"
#include "tgt_hardware.h"

#define ble_nv_debug
#ifdef ble_nv_debug
#define ble_trace TRACE
#else
#define ble_trace
#endif

extern nv_record_struct nv_record_config;
static struct Ble_nvrecord *nvrecord_ble_p = NULL;

static bool blerec_specific_value_prepare(const BleDeviceinfo* param_rec)
{
    // Preparations before adding new ble record:
    // 1. If not existing. Check the record count. If it's BLE_RECORD_NUM,
    //     move 0-(BLE_RECORD_NUM-2) right side by one slot, to discard the last one and
    //     leave slot 0, decrease the record number.
    //     If it's smaller than BLE_RECORD_NUM, move 0-(count-1) right side by one slot, leave slot 0,
    //     don't change the record number.
    // 2. If existing already and the location is entryToFree , move 0-(entryToFree-1)
    //      right side by one slot, leave slot 0. Decrease the record number for adding the new one.

    bool isEntryExisting = false;
    uint8_t entryToFree = 0;
    struct Ble_nvrecord *dest_ptr;

    dest_ptr = nvrecord_ble_p;
    TRACE(3,"%s start search addr %08x  list_num=%d", __func__, (uint32_t)dest_ptr, dest_ptr->saved_list_num);
    if (dest_ptr->saved_list_num > 0)
    {
        for (uint8_t i = 0; i < dest_ptr->saved_list_num; i++)
        {
            if (0 == memcmp(dest_ptr->ble_nv[i].LTK, param_rec->LTK, BLE_LTK_SIZE))
            {
                ble_trace(2,"%s Find the existing entry %d", __func__, i);
                DUMP8("%02x ", (uint8_t *)param_rec, sizeof(BleDeviceinfo));
                DUMP8("%02x ", (uint8_t *)&dest_ptr->ble_nv[i], sizeof(BleDeviceinfo));
                if (!memcmp((uint8_t *)param_rec, (uint8_t *)&dest_ptr->ble_nv[i], sizeof(BleDeviceinfo)))
                {
                    ble_trace(0,"The new coming BLE device info is the same as the recorded.");
                    return false;
                }
                memset(&(dest_ptr->ble_nv[i]), 0, sizeof(BleDeviceinfo));
                entryToFree = i;
                dest_ptr->saved_list_num--;
                isEntryExisting = true;
                break;
            }
        }
    }
    else
    {
        return true;
    }

    if (!isEntryExisting)
    {
        if (BLE_RECORD_NUM == dest_ptr->saved_list_num)
        {
            TRACE(0,"<=====>blerec list is full,delete the oldest and add param_rec to list");
            for (uint8_t k = 0;k < BLE_RECORD_NUM - 1;k++)
            {
                memcpy(&(dest_ptr->ble_nv[BLE_RECORD_NUM - 1 - k]),
                    &(dest_ptr->ble_nv[BLE_RECORD_NUM - 2 - k]),
                    sizeof(BleDeviceinfo));
            }
            dest_ptr->saved_list_num--;
        }
        else
        {
            for (uint8_t k = 0;k < dest_ptr->saved_list_num;k++)
            {
                memcpy(&(dest_ptr->ble_nv[dest_ptr->saved_list_num - k]),
                    &(dest_ptr->ble_nv[dest_ptr->saved_list_num - 1 - k]),
                    sizeof(BleDeviceinfo));
            }
        }
        return true;
    }
    else
    {
        for(uint8_t list_updata = 0;list_updata < entryToFree; list_updata++)
        {
            memcpy(&(dest_ptr->ble_nv[entryToFree - list_updata]),
                &(dest_ptr->ble_nv[entryToFree - list_updata - 1]),
                sizeof(BleDeviceinfo));
        }
    }

    return true;
}

void nv_record_blerec_init(void)
{
    if (NULL == nvrecord_ble_p)
    {
        nvrecord_ble_p = (struct Ble_nvrecord *)nvrec_config_get_int(nv_record_config.config,
            (const char *)section_name_ble_nvds, (const char *)NV_RECORD_BLE_KEY, NVRAM_BLE_INVALID);
    }

    if (nvrecord_ble_p == (struct Ble_nvrecord *)NVRAM_BLE_INVALID){
        nvrecord_ble_p = (struct Ble_nvrecord *)pool_malloc(sizeof(struct Ble_nvrecord));
        if (!nvrecord_ble_p){
            ASSERT(0,"%s ble pool_malloc failed",__func__);
            return;
        }
        TRACE(1,"ble nv malloc addr %08x", (uint32_t)nvrecord_ble_p);

        memset(nvrecord_ble_p, 0, sizeof(struct Ble_nvrecord));
        nvrecord_ble_p->saved_list_num = 0;//init saved num

        uint8_t index;
        //avoid ble irk collision low probability
        uint32_t generatedSeed = hal_sys_timer_get();
        for (uint8_t index = 0; index < sizeof(bt_addr); index++)
        {
            generatedSeed ^= (((uint32_t)(bt_addr[index])) << (hal_sys_timer_get()&0xF));
        }
        srand(generatedSeed);

        // generate a new IRK
        for (index = 0; index < BLE_IRK_SIZE; index++)
        {
            nvrecord_ble_p->loc_irk[index] = (uint8_t)co_rand_word();
        }
        nvrec_config_set_int(nv_record_config.config, section_name_ble_nvds,
            (const char *)NV_RECORD_BLE_KEY,(int)nvrecord_ble_p);

    }
}

void nv_record_blerec_get_local_irk(uint8_t* pIrk)
{
    memcpy(pIrk, nvrecord_ble_p->loc_irk, BLE_IRK_SIZE);
}

bool nv_record_blerec_get_bd_addr_from_irk(uint8_t* pBdAddr, uint8_t* pIrk)
{
    if (nvrecord_ble_p->saved_list_num > 0)
    {
        for (uint8_t index = 0; index < nvrecord_ble_p->saved_list_num; index++)
        {
            if (!memcmp(pIrk, nvrecord_ble_p->ble_nv[index].IRK, BLE_IRK_SIZE))
            {
                memcpy(pBdAddr, nvrecord_ble_p->ble_nv[index].peer_bleAddr, BLE_ADDR_SIZE);
                return true;
            }
        }
        return false;
    }
    else
    {
        return false;
    }
}

int nv_record_blerec_add(const BleDeviceinfo *param_rec)
{
    int nRet = 0;

    uint8_t isNeedToUpdateNv = true;
    isNeedToUpdateNv = blerec_specific_value_prepare(param_rec);

    if (isNeedToUpdateNv)
    {
        //add device info into nv struct
        memcpy(nvrecord_ble_p->ble_nv[0].peer_bleAddr, param_rec->peer_bleAddr, BLE_ADDR_SIZE);//addr
        nvrecord_ble_p->ble_nv[0].EDIV = param_rec->EDIV;//EDIV
        memcpy(nvrecord_ble_p->ble_nv[0].RANDOM, param_rec->RANDOM, BLE_ENC_RANDOM_SIZE);//RANDOM
        memcpy(nvrecord_ble_p->ble_nv[0].LTK, param_rec->LTK, BLE_LTK_SIZE);//LTK
        memcpy(nvrecord_ble_p->ble_nv[0].IRK, param_rec->IRK, BLE_IRK_SIZE);//IRK
        nvrecord_ble_p->ble_nv[0].bonded = param_rec->bonded;//bond status
        nvrecord_ble_p->saved_list_num++;//updata saved num

        nv_record_update_runtime_userdata();
        TRACE(2,"%s CURRENT BLE LIST NUM=%d", __func__, nvrecord_ble_p->saved_list_num);
    }
#ifdef ble_nv_debug
    for (uint8_t k = 0;k < nvrecord_ble_p->saved_list_num;k++){
        TRACE(0,"=========================================");
        TRACE(1,"Num %d BLE record:", k);
        TRACE(0,"BLE addr:");
        DUMP8("%02x ", (uint8_t *)nvrecord_ble_p->ble_nv[k].peer_bleAddr, BLE_ADDR_SIZE);
        TRACE(1,"NV EDIV %d and random is:", nvrecord_ble_p->ble_nv[k].EDIV);
        DUMP8("%02x ", (uint8_t *)nvrecord_ble_p->ble_nv[k].RANDOM, BLE_ENC_RANDOM_SIZE);
        TRACE(0,"NV LTK:");
        DUMP8("%02x ", (uint8_t *)nvrecord_ble_p->ble_nv[k].LTK, BLE_LTK_SIZE);
        TRACE(0,"NV irk:");
        DUMP8("%02x ", (uint8_t *)nvrecord_ble_p->ble_nv[k].IRK, BLE_IRK_SIZE);
    }
#endif
    return nRet;
}

uint8_t nv_record_ble_fill_irk(uint8_t* irkToFill)
{
    struct Ble_nvrecord *find_ptr;

    find_ptr = (struct Ble_nvrecord *)nvrec_config_get_int(nv_record_config.config,
        (const char *)section_name_ble_nvds,
        (const char *)NV_RECORD_BLE_KEY,
        NVRAM_BLE_INVALID);

    TRACE(2,"%s find_ptr 0x%x", __FUNCTION__, (uint32_t)find_ptr);

    if (((struct Ble_nvrecord *)NVRAM_BLE_INVALID == find_ptr) ||
        (0 == find_ptr->saved_list_num))
    {
        return 0;
    }

    if (find_ptr->saved_list_num > 0)
    {
        for (uint8_t index = 0; index < find_ptr->saved_list_num; index++)
        {
            memcpy(irkToFill + index*BLE_IRK_SIZE, find_ptr->ble_nv[index].IRK, BLE_IRK_SIZE);
        }
        return find_ptr->saved_list_num;
    }
    else
    {
        return 0;
    }
}

//when master send encription req,if bonded,use ltk to bonding again(skip the pair step)
bool nv_record_ble_record_find_ltk_through_static_bd_addr(uint8_t* pBdAddr, uint8_t *ltk)
{
    struct Ble_nvrecord *find_ptr;
    find_ptr = (struct Ble_nvrecord *)nvrec_config_get_int(nv_record_config.config, (const char *)section_name_ble_nvds,(const char *)NV_RECORD_BLE_KEY, NVRAM_BLE_INVALID);
    if (find_ptr == (struct Ble_nvrecord *)NVRAM_BLE_INVALID)
    {
        TRACE(1,"%s find data failed",__func__);
        return false;
    }

    for (uint8_t find_index=0; find_index<find_ptr->saved_list_num; find_index++)
    {
        if (!memcmp(find_ptr->ble_nv[find_index].peer_bleAddr, pBdAddr, BLE_ADDR_SIZE))
        {
            ble_trace(2,"%s FIND LTK IN NV SUCCESS %08x", __func__, (uint32_t)&find_ptr->ble_nv[find_index].LTK);
            memcpy(ltk, find_ptr->ble_nv[find_index].LTK, BLE_LTK_SIZE);
            return true;
        }
    }
    return false;
}

bool nv_record_ble_record_Once_a_device_has_been_bonded(void)
{
    struct Ble_nvrecord *find_ptr;

    find_ptr=(struct Ble_nvrecord *)nvrec_config_get_int(nv_record_config.config, (const char *)section_name_ble_nvds,(const char *)NV_RECORD_BLE_KEY, NVRAM_BLE_INVALID);
    if (find_ptr == (struct Ble_nvrecord *)NVRAM_BLE_INVALID){
        return false;
    }
    for(uint8_t find_index=0;find_index<find_ptr->saved_list_num;find_index++){
        if(find_ptr->ble_nv[find_index].bonded==true){
            return true;
        }
    }
    return false;
}

void nv_record_ble_delete_entry(uint8_t* pBdAddr)
{
    struct Ble_nvrecord *find_ptr;

    find_ptr = (struct Ble_nvrecord *)nvrec_config_get_int(nv_record_config.config,
        (const char *)section_name_ble_nvds,
        (const char *)NV_RECORD_BLE_KEY,
        NVRAM_BLE_INVALID);

    if (((struct Ble_nvrecord *)NVRAM_BLE_INVALID == find_ptr) ||
        (0 == find_ptr->saved_list_num))
    {
        return;
    }

    int8_t indexToDelete = -1;

    for (uint8_t find_index=0; find_index < find_ptr->saved_list_num; find_index++)
    {
        if (!memcmp(find_ptr->ble_nv[find_index].peer_bleAddr, pBdAddr, BLE_ADDR_SIZE))
        {
            indexToDelete = find_index;
            break;
        }
    }

    if (indexToDelete >= 0)
    {
        uint8_t index;
        for (index = indexToDelete; index < find_ptr->saved_list_num - 1; index++)
        {
            memcpy(&(find_ptr->ble_nv[index]),
                &(find_ptr->ble_nv[index + 1]),
                sizeof(BleDeviceinfo));
        }

        memset((uint8_t *)&(find_ptr->ble_nv[index]), 0, sizeof(BleDeviceinfo));
        find_ptr->saved_list_num--;
        nv_record_update_runtime_userdata();
    }
}
#endif // !defined(NEW_NV_RECORD_ENABLED)


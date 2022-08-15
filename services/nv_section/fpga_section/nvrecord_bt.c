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
#if defined(NEW_NV_RECORD_ENABLED)
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "nvrecord_extension.h"
#include "nvrecord_bt.h"
#include "hal_trace.h"

nvrec_btdevicerecord g_fpga_ram_record;
void ram_record_ddbrec_init(void)
{
    g_fpga_ram_record.record.trusted = false;
}

bt_status_t ram_record_ddbrec_find(const bt_bdaddr_t* bd_ddr, nvrec_btdevicerecord **record)
{
    if (g_fpga_ram_record.record.trusted && \
        !memcmp(&g_fpga_ram_record.record.bdAddr.address[0], &bd_ddr->address[0], 6))
    {
        *record = &g_fpga_ram_record;
        return BT_STS_SUCCESS;
    }
    else
    {
        return BT_STS_FAILED;
    }
}

bt_status_t ram_record_ddbrec_add(const nvrec_btdevicerecord* param_rec)
{
    g_fpga_ram_record = *param_rec;
    g_fpga_ram_record.record.trusted = true;
    return BT_STS_SUCCESS;
}

bt_status_t ram_record_ddbrec_delete(const bt_bdaddr_t *bdaddr)
{
    if (g_fpga_ram_record.record.trusted && \
        !memcmp(&g_fpga_ram_record.record.bdAddr.address[0], &bdaddr->address[0], 6))
    {
        g_fpga_ram_record.record.trusted = false;
    }
    return BT_STS_SUCCESS;
}

void nvrecord_rebuild_paired_bt_dev_info(NV_RECORD_PAIRED_BT_DEV_INFO_T* pPairedBtInfo)
{
    memset((uint8_t *)pPairedBtInfo, 0, sizeof(NV_RECORD_PAIRED_BT_DEV_INFO_T));
    pPairedBtInfo->pairedDevNum = 0;
}

void nv_record_btdevicerecord_set_a2dp_profile_active_state(btdevice_profile* device_plf, bool isActive)
{
    device_plf->hsp_act = isActive;
}

void nv_record_btdevicerecord_set_hfp_profile_active_state(btdevice_profile* device_plf, bool isActive)
{
    device_plf->hsp_act = isActive;
}

void nv_record_btdevicerecord_set_hsp_profile_active_state(btdevice_profile* device_plf, bool isActive)
{
    device_plf->hsp_act = isActive;
}

void nv_record_btdevicerecord_set_a2dp_profile_codec(btdevice_profile* device_plf, uint8_t a2dpCodec)
{
    device_plf->a2dp_codectype = a2dpCodec;
}

int nv_record_get_paired_dev_count(void)
{
    if (NULL == nvrecord_extension_p)
    {
        return 0;
    }

    return nvrecord_extension_p->bt_pair_info.pairedDevNum;
}

/*
return:
    -1:     enum dev failure.
    0:      without paired dev.
    1:      only 1 paired dev,store@record1.
    2:      get 2 paired dev.notice:record1 is the latest record.
*/
int nv_record_enum_latest_two_paired_dev(btif_device_record_t* record1,btif_device_record_t* record2)
{
    if((NULL == record1) || (NULL == record2) || (NULL == nvrecord_extension_p))
    {
        return -1;
    }

    if (nvrecord_extension_p->bt_pair_info.pairedDevNum > 0)
    {
        if (1 == nvrecord_extension_p->bt_pair_info.pairedDevNum)
        {
            memcpy((uint8_t *)record1, (uint8_t *)&(nvrecord_extension_p->bt_pair_info.pairedBtDevInfo[0]),
                   sizeof(btif_device_record_t));
            return 1;
        }
        else
        {
            memcpy((uint8_t *)record1, (uint8_t *)&(nvrecord_extension_p->bt_pair_info.pairedBtDevInfo[0]),
                   sizeof(btif_device_record_t));
            memcpy((uint8_t *)record2, (uint8_t *)&(nvrecord_extension_p->bt_pair_info.pairedBtDevInfo[1]),
                   sizeof(btif_device_record_t));
            return 2;
        }
    }
    else
    {
        return 0;
    }
}

static void nv_record_print_dev_record(const btif_device_record_t* record)
{

}

void nv_record_all_ddbrec_print(void)
{
    if (NULL == nvrecord_extension_p)
    {
        TRACE(0,"No BT paired dev.");
        return;
    }

    if (nvrecord_extension_p->bt_pair_info.pairedDevNum > 0)
    {
        for(uint8_t tmp_i=0; tmp_i < nvrecord_extension_p->bt_pair_info.pairedDevNum; tmp_i++)
        {
            btif_device_record_t record;
            bt_status_t ret_status;
            ret_status = nv_record_enum_dev_records(tmp_i, &record);
            if (BT_STS_SUCCESS == ret_status)
            {
                nv_record_print_dev_record(&record);
            }
        }
    }
    else
    {
        TRACE(0,"No BT paired dev.");
    }
}

/*
this function should be surrounded by OS_LockStack and OS_UnlockStack when call.
*/
bt_status_t nv_record_enum_dev_records(unsigned short index,btif_device_record_t* record)
{
    btif_device_record_t *recaddr = NULL;

    if((index >= nvrecord_extension_p->bt_pair_info.pairedDevNum) || (NULL == nvrecord_extension_p))
    {
        return BT_STS_FAILED;
    }

    recaddr = (btif_device_record_t *)&(nvrecord_extension_p->bt_pair_info.pairedBtDevInfo[index].record);
    memcpy(record, recaddr, sizeof(btif_device_record_t));
    nv_record_print_dev_record(record);
    return BT_STS_SUCCESS;
}

static int8_t nv_record_get_bt_pairing_info_index(const uint8_t* btAddr)
{
    NV_RECORD_PAIRED_BT_DEV_INFO_T* pBtDevInfo =
        (NV_RECORD_PAIRED_BT_DEV_INFO_T *)(&(nvrecord_extension_p->bt_pair_info));

    for (uint8_t index = 0; index < pBtDevInfo->pairedDevNum; index++)
    {
        if (!memcmp(pBtDevInfo->pairedBtDevInfo[index].record.bdAddr.address,
                    btAddr, BTIF_BD_ADDR_SIZE))
        {
            return (int8_t)index;
        }
    }

    return -1;
}

/**********************************************
this function should be surrounded by OS_LockStack and OS_UnlockStack when call.
**********************************************/
static bt_status_t POSSIBLY_UNUSED nv_record_ddbrec_add(const btif_device_record_t* param_rec)
{
    if ((NULL == param_rec) || (NULL == nvrecord_extension_p))
    {
        return BT_STS_FAILED;
    }

    uint32_t lock = nv_record_pre_write_operation();

    bool isFlushNv = false;

    // try to find the entry
    int8_t indexOfEntry = -1;
    NV_RECORD_PAIRED_BT_DEV_INFO_T* pBtDevInfo =
        (NV_RECORD_PAIRED_BT_DEV_INFO_T *)(&(nvrecord_extension_p->bt_pair_info));
    indexOfEntry = nv_record_get_bt_pairing_info_index(param_rec->bdAddr.address);

    if (-1 == indexOfEntry)
    {
        // don't exist,  need to add to the head of the entry list
        if (MAX_BT_PAIRED_DEVICE_COUNT == pBtDevInfo->pairedDevNum)
        {
            for (uint8_t k = 0; k < MAX_BT_PAIRED_DEVICE_COUNT - 1; k++)
            {
                memcpy((uint8_t *)&(pBtDevInfo->pairedBtDevInfo[MAX_BT_PAIRED_DEVICE_COUNT - 1 - k]),
                       (uint8_t *)&(pBtDevInfo->pairedBtDevInfo[MAX_BT_PAIRED_DEVICE_COUNT - 2 - k]),
                       sizeof(nvrec_btdevicerecord));
            }
            pBtDevInfo->pairedDevNum--;
        }
        else
        {
            for (uint8_t k = 0; k < pBtDevInfo->pairedDevNum; k++)
            {
                memcpy((uint8_t *)&(pBtDevInfo->pairedBtDevInfo[pBtDevInfo->pairedDevNum - k]),
                       (uint8_t *)&(pBtDevInfo->pairedBtDevInfo[pBtDevInfo->pairedDevNum - 1 - k]),
                       sizeof(nvrec_btdevicerecord));
            }
        }

        // fill the default value
        nvrec_btdevicerecord* nvrec_pool_record = &(pBtDevInfo->pairedBtDevInfo[0]);
        memcpy((uint8_t *)&(nvrec_pool_record->record), (uint8_t *)param_rec,
               sizeof(btif_device_record_t));
        nvrec_pool_record->device_vol.a2dp_vol = NVRAM_ENV_STREAM_VOLUME_A2DP_VOL_DEFAULT;
        nvrec_pool_record->device_vol.hfp_vol = NVRAM_ENV_STREAM_VOLUME_HFP_VOL_DEFAULT;
        nvrec_pool_record->device_plf.hfp_act = false;
        nvrec_pool_record->device_plf.hsp_act = false;
        nvrec_pool_record->device_plf.a2dp_act = false;
#ifdef BTIF_DIP_DEVICE
        nvrec_pool_record->vend_id = 0;
        nvrec_pool_record->vend_id_source = 0;
#endif

        pBtDevInfo->pairedDevNum++;

        // need to flush the nv record
        isFlushNv = true;
    }
    else
    {
        // exist
        // check whether it's already at the head
        // if not, move it to the head
        if (indexOfEntry > 0)
        {
            nvrec_btdevicerecord record;
            memcpy((uint8_t *)&record, (uint8_t *)&(pBtDevInfo->pairedBtDevInfo[indexOfEntry]),
                   sizeof(record));

            // if not, move it to the head
            for (uint8_t k = 0; k < indexOfEntry; k++)
            {
                memcpy((uint8_t *)&(pBtDevInfo->pairedBtDevInfo[indexOfEntry - k]),
                       (uint8_t *)&(pBtDevInfo->pairedBtDevInfo[indexOfEntry - 1 - k]),
                       sizeof(nvrec_btdevicerecord));
            }

            memcpy((uint8_t *)&(pBtDevInfo->pairedBtDevInfo[0]), (uint8_t *)&record,
                   sizeof(record));

            // update the link info
            memcpy((uint8_t *)&(pBtDevInfo->pairedBtDevInfo[0].record), (uint8_t *)param_rec,
                   sizeof(btif_device_record_t));

            // need to flush the nv record
            isFlushNv = true;
        }
        // else, check whether the link info needs to be updated
        else
        {
            if (memcmp((uint8_t *)&(pBtDevInfo->pairedBtDevInfo[0].record),
                       (uint8_t *)param_rec, sizeof(btif_device_record_t)))
            {
                // update the link info
                memcpy((uint8_t *)&(pBtDevInfo->pairedBtDevInfo[0].record), (uint8_t *)param_rec,
                       sizeof(btif_device_record_t));

                // need to flush the nv record
                isFlushNv = true;
            }
        }
    }

    TRACE(1,"paired Bt dev:%d", pBtDevInfo->pairedDevNum);
    TRACE(1,"Is to flush nv: %d", isFlushNv);
    nv_record_all_ddbrec_print();

    if (isFlushNv)
    {
        nv_record_update_runtime_userdata();
    }

    nv_record_post_write_operation(lock);

    return BT_STS_SUCCESS;
}

/*
this function should be surrounded by OS_LockStack and OS_UnlockStack when call.
*/
bt_status_t nv_record_add(SECTIONS_ADP_ENUM type, void *record)
{
    bt_status_t retstatus = BT_STS_FAILED;

    if ((NULL == record) || (section_none == type))
    {
        return BT_STS_FAILED;
    }

    switch(type)
    {
        case section_usrdata_ddbrecord:
            retstatus = ram_record_ddbrec_add(record);          
            break;
        default:
            break;
    }

    return retstatus;
}

/*
this function should be surrounded by OS_LockStack and OS_UnlockStack when call.
*/
bt_status_t nv_record_ddbrec_find(const bt_bdaddr_t* bd_ddr, btif_device_record_t *record)
{
    if ((NULL == bd_ddr) || (NULL == record) || (NULL == nvrecord_extension_p))
    {
        return BT_STS_FAILED;
    }

    int8_t indexOfEntry = -1;
    NV_RECORD_PAIRED_BT_DEV_INFO_T* pBtDevInfo =
        (NV_RECORD_PAIRED_BT_DEV_INFO_T *)(&(nvrecord_extension_p->bt_pair_info));
    indexOfEntry = nv_record_get_bt_pairing_info_index(bd_ddr->address);

    if (-1 == indexOfEntry)
    {
        return BT_STS_FAILED;
    }
    else
    {
        memcpy((uint8_t *)record, (uint8_t *)&(pBtDevInfo->pairedBtDevInfo[indexOfEntry].record),
               sizeof(btif_device_record_t));
        return BT_STS_SUCCESS;
    }
}

/*
this function should be surrounded by OS_LockStack and OS_UnlockStack when call.
*/
bt_status_t nv_record_ddbrec_delete(const bt_bdaddr_t *bdaddr)
{
    ram_record_ddbrec_delete(bdaddr);
    return BT_STS_SUCCESS;
}

int nv_record_btdevicerecord_find(const bt_bdaddr_t *bd_ddr, nvrec_btdevicerecord **record)
{
    ram_record_ddbrec_find(bd_ddr, record);
    return 0;
}

void nv_record_btdevicerecord_set_a2dp_vol(nvrec_btdevicerecord* pRecord, int8_t vol)
{
    uint32_t lock = nv_record_pre_write_operation();
    if (vol != pRecord->device_vol.a2dp_vol)
    {
        nv_record_update_runtime_userdata();
        pRecord->device_vol.a2dp_vol = vol;
    }

    nv_record_post_write_operation(lock);
}

void nv_record_btdevicerecord_set_hfp_vol(nvrec_btdevicerecord* pRecord, int8_t vol)
{
    uint32_t lock = nv_record_pre_write_operation();
    if (vol != pRecord->device_vol.hfp_vol)
    {
        nv_record_update_runtime_userdata();
        pRecord->device_vol.hfp_vol = vol;
    }

    nv_record_post_write_operation(lock);
}

void nv_record_btdevicevolume_set_a2dp_vol(btdevice_volume* device_vol, int8_t vol)
{
    uint32_t lock = nv_record_pre_write_operation();
    if (vol != device_vol->a2dp_vol)
    {
        nv_record_update_runtime_userdata();
        device_vol->a2dp_vol = vol;
    }

    nv_record_post_write_operation(lock);

}

void nv_record_btdevicevolume_set_hfp_vol(btdevice_volume* device_vol, int8_t vol)
{
    uint32_t lock = nv_record_pre_write_operation();
    if (vol != device_vol->hfp_vol)
    {
        nv_record_update_runtime_userdata();
        device_vol->hfp_vol = vol;
    }

    nv_record_post_write_operation(lock);

}

void nv_record_btdevicerecord_set_vend_id_and_source(nvrec_btdevicerecord* pRecord, int16_t vend_id, int16_t vend_id_source)
{
#ifdef BTIF_DIP_DEVICE
    TRACE(2, "%s vend id 0x%x", __func__, vend_id);
    uint32_t lock = nv_record_pre_write_operation();
    if (vend_id != pRecord->vend_id)
    {
        nv_record_update_runtime_userdata();
        pRecord->vend_id = vend_id;
        pRecord->vend_id_source = vend_id_source;
    }

    nv_record_post_write_operation(lock);
#endif
}

#endif //#if defined(NEW_NV_RECORD_ENABLED)


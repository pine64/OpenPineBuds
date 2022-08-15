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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hal_trace.h"
#include "bluetooth.h"
#include "me_api.h"

#ifndef FPGA
#include "cmsis_os.h"
#include "cmsis.h"
#include "nvrecord.h"

//static int DDB_Print_Node(BtDeviceRecord* record)
//{
//    nvrec_trace(0,"record_bdAddr = ");
//    OS_DUMP8("%x ",record->bdAddr.addr,sizeof(record->bdAddr.addr));
//    nvrec_trace(0,"record_trusted = ");
//    OS_DUMP8("%d ",&record->trusted,sizeof((uint8_t)record->trusted));
//    nvrec_trace(0,"record_linkKey = ");
//    OS_DUMP8("%x ",record->linkKey,sizeof(record->linkKey));
//    nvrec_trace(0,"record_keyType = ");
//    OS_DUMP8("%x ",&record->keyType,sizeof(record->keyType));
//    nvrec_trace(0,"record_pinLen = ");
//    OS_DUMP8("%x ",&record->pinLen,sizeof(record->pinLen));
//    return 1;
//}

void ddbif_list_saved_flash(void)
{
    nv_record_flash_flush();
}

bt_status_t ddbif_close(void)
{
    ddbif_list_saved_flash();
    return BT_STS_SUCCESS;
}

bt_status_t ddbif_add_record(btif_device_record_t *record)
{
    return nv_record_add(section_usrdata_ddbrecord,(void *)record);
}

bt_status_t ddbif_open(const bt_bdaddr_t *bdAddr)
{
    bt_status_t status = BT_STS_FAILED;
    status = nv_record_open(section_usrdata_ddbrecord);

    return status;
}

bt_status_t ddbif_find_record(const bt_bdaddr_t *bdAddr, btif_device_record_t *record)
{
    return nv_record_ddbrec_find(bdAddr,record);
}

bt_status_t ddbif_delete_record(const bt_bdaddr_t *bdAddr)
{
    return nv_record_ddbrec_delete(bdAddr);
}

bt_status_t ddbif_enum_device_records(I16 index, btif_device_record_t *record)
{
    return nv_record_enum_dev_records((unsigned short)index,record);
}
#else
typedef struct _DDB_List {
    struct _DDB_List *next;
    btif_device_record_t *record;
} DDB_List;

static DDB_List *head = NULL;
static bt_bdaddr_t remDev_addr;
static int DDB_Print_Node(btif_device_record_t *record);
static int DDB_Print();

static void *DDB_Malloc (uint32_t size)
{
    return malloc(size);
}

static void DDB_Free (void *p_mem)
{
    free(p_mem);
}

static char POSSIBLY_UNUSED Hex2String(unsigned char *src, char* dest,
                        unsigned int srcLen,
                        unsigned int destLen)
{
    unsigned int i = 0;
    unsigned char hb = 0, lb = 0;

    memset((char *)dest, 0, destLen);
    for(i = 0 ; i < srcLen ; i++) {
        hb = (src[i]&0xf0)>>4;
        if( hb >= 0 && hb <= 9)
            hb += 0x30;
        else if( hb>=10 &&hb <=15 )
            hb = hb -10 + 'A';
        else
            return 0;

        lb = src[i]&0x0f;
        if( lb>=0 && lb<=9 )
            lb += 0x30;
        else if( lb>=10 && lb<=15 )
            lb = lb - 10 + 'A';
        else
            return 0;

        dest[i*2+0] = hb;
        dest[i*2+1] = lb;
    }

    return 1;
}

static void DDB_List_delete(DDB_List *list_head)
{
    DDB_List *list = list_head,*list_tmp = NULL;

    while(NULL != list) {
        list_tmp = list;
        if(NULL != list->record)
            DDB_Free(list->record);

        list = list->next;
        DDB_Free(list_tmp);
    }
}

bt_status_t ddbif_close(void)
{
    DDB_List *Close_tmp = NULL;
    Close_tmp = head;

    while(1) {
        DDB_Print();

        Close_tmp = Close_tmp->next;
        if(Close_tmp == NULL) {
            TRACE(0,"DDB_Close.\n");
            break;
        }
    }

    DDB_List_delete(head);
    return BT_STS_SUCCESS;
}

bt_status_t ddbif_add_record(btif_device_record_t *record)
{
    DDB_List *ptr = NULL;
    DDB_List *tmp = NULL;
    ptr = head;

    tmp = (DDB_List *)DDB_Malloc(sizeof(DDB_List));
    if(tmp == NULL) {
        TRACE(0,"DDB_AddRecord head == NULL:There is no enough space for DDB_List.\n");
        return BT_STS_FAILED;
    }
    tmp->next = NULL;

    tmp->record = (btif_device_record_t *)DDB_Malloc(sizeof(btif_device_record_t));
    if(NULL == tmp->record) {
        TRACE(0,"DDB_AddRecord head == NULL:There is no enough space for DDB_List.record.\n");
        return BT_STS_FAILED;
    }

    if(head == NULL) {
        memcpy(tmp->record->bdAddr.address,record->bdAddr.address,sizeof(record->bdAddr.address));
        tmp->record->keyType = record->keyType;
        memcpy(tmp->record->linkKey,record->linkKey,sizeof(record->linkKey));
        tmp->record->pinLen = record->pinLen;
        tmp->record->trusted = record->trusted;

        head = tmp;
        memcpy(remDev_addr.address,tmp->record->bdAddr.address,sizeof(tmp->record->bdAddr.address));
        TRACE(0,"DDB_AddRecord head:\n");
        DDB_Print_Node(tmp->record);
        DDB_Print();

        return BT_STS_SUCCESS;
    }

    while((ptr->next != NULL) &&
            (memcmp(ptr->record->bdAddr.address, record->bdAddr.address,
                                sizeof(ptr->record->bdAddr.address)))) {
        ptr = ptr->next;
    }

    if(!memcmp(ptr->record->bdAddr.address, record->bdAddr.address,
                                sizeof(ptr->record->bdAddr.address))) {
        ptr->record->keyType = record->keyType;
        memcpy(ptr->record->linkKey,record->linkKey,sizeof(record->linkKey));
        ptr->record->pinLen = record->pinLen;
        ptr->record->trusted = record->trusted;
        TRACE(0,"DDB_AddRecord head::\n");
        DDB_Print_Node(ptr->record);
        DDB_Print();
        free(tmp->record);
        free(tmp);
        return BT_STS_SUCCESS;
    }

    if(ptr->next == NULL) {
        tmp->next = NULL;
        memcpy(tmp->record->bdAddr.address,record->bdAddr.address,sizeof(record->bdAddr.address));
        tmp->record->keyType = record->keyType;
        memcpy(tmp->record->linkKey,record->linkKey,sizeof(record->linkKey));
        tmp->record->pinLen = record->pinLen;
        tmp->record->trusted = record->trusted;

        memcpy(remDev_addr.address,tmp->record->bdAddr.address,sizeof(tmp->record->bdAddr.address));

        ptr->next = tmp;
        TRACE(0,"DDB_AddRecord head:::\n");
        DDB_Print_Node(tmp->record);
        DDB_Print();

        return BT_STS_SUCCESS;
    }

    return BT_STS_FAILED;
}


bt_status_t ddbif_open(const bt_bdaddr_t *bdAddr)
{
//    DDB_List *head = NULL;
    return BT_STS_SUCCESS;
}

static int DDB_Print_Node(btif_device_record_t *record)
{
    int i;

    for(i = 0; i < 6; i++) {
        TRACE(1,"%x ",record->bdAddr.address[i]);
    }
    TRACE(0,"\n");
/*
    for(i=0; i<16; i++)
    {
        TRACE(1,"%x ",record->linkKey[i]);
    }
    TRACE(0,"\n");

    TRACE(3,"%d,%d,%d\n",record->keyType,record->pinLen,record->trusted);
    */
    return 1;

}

static int DDB_Print()
{
    DDB_List *Print_tmp = NULL;
    Print_tmp = head;

    if(Print_tmp == NULL) {
        TRACE(0,"DDB list is null!\n");
        return 0;
    }

    TRACE(0,"-----------------DDB list-------------------\n");
    while(Print_tmp != NULL) {
        DDB_Print_Node(Print_tmp->record);
        Print_tmp = Print_tmp->next;
    }
    TRACE(0,"--------------------end---------------------\n");
    return 0;
}

bt_status_t ddbif_find_record(const bt_bdaddr_t *bdAddr, btif_device_record_t *record)
{
    DDB_List *FindRecord_tmp = NULL;
    FindRecord_tmp = head;

    if(head == NULL) {
        TRACE(0,"DDB_FindRecord:DDB is null!\n");
        return BT_STS_FAILED;
    }

    while(memcmp(FindRecord_tmp->record->bdAddr.address, bdAddr->address,
                sizeof(FindRecord_tmp->record->bdAddr.address)) && (FindRecord_tmp->next != NULL)) {
        FindRecord_tmp = FindRecord_tmp->next;
    }

    if(!memcmp(FindRecord_tmp->record->bdAddr.address,bdAddr->address,
                    sizeof(FindRecord_tmp->record->bdAddr.address))) {
        memcpy(record->bdAddr.address, FindRecord_tmp->record->bdAddr.address,
                            sizeof(FindRecord_tmp->record->bdAddr.address));
        record->keyType = FindRecord_tmp->record->keyType;
        memcpy(record->linkKey,FindRecord_tmp->record->linkKey,
                            sizeof(FindRecord_tmp->record->linkKey));
        record->pinLen = FindRecord_tmp->record->pinLen;
        record->trusted = FindRecord_tmp->record->trusted;
        TRACE(0,"DDB_FindRecord:\n");
        DDB_Print_Node(record);
        return BT_STS_SUCCESS;
    }

    if(FindRecord_tmp->next == NULL) {
        TRACE(0,"DDB_FindRecord ends.\n");
        return BT_STS_FAILED;
    }

    return BT_STS_FAILED;
}

bt_status_t ddbif_delete_record(const bt_bdaddr_t *bdAddr)
{
    DDB_List *checkingAddr = NULL;
    DDB_List *checkedAddr = NULL;
    checkingAddr = head;

    if(head == NULL) {
        TRACE(0,"DDB_DeleteRecord:list is null!\n");
        return BT_STS_FAILED;
    }

    while(memcmp(checkingAddr->record->bdAddr.address,bdAddr->address,
                sizeof(checkingAddr->record->bdAddr.address)) && (checkingAddr->next != NULL)) {
        checkedAddr = checkingAddr;
        checkingAddr = checkingAddr->next;
    }

    if(!memcmp(checkingAddr->record->bdAddr.address,bdAddr->address,
                        sizeof(checkingAddr->record->bdAddr.address))) {
        if(checkingAddr == head) {
            head = checkingAddr->next;
            free(checkingAddr->record);
            free(checkingAddr);

            DDB_Print();
            return BT_STS_SUCCESS;
        } else {
            checkedAddr->next = checkingAddr->next;
            free(checkingAddr->record);
            free(checkingAddr);

            DDB_Print();
            return BT_STS_SUCCESS;
        }
    }

    if(checkingAddr->next == NULL) {
        return BT_STS_FAILED;
    }

    return BT_STS_FAILED;
}

bt_status_t ddbif_enum_device_records(I16 index, btif_device_record_t *record)
{
    DDB_List *databaselist;
    databaselist = head;

    if(head == NULL) {
        TRACE(0,"DDB_EnumDeviceRecords:No records!\n");
        return BT_STS_FAILED;
    }

    TRACE(1,"index=%d\n",(uint32_t)index);
    while(databaselist != NULL) {
        if(!index) {
            break;
        }
        databaselist = databaselist->next;
        index--;
    }

    if((index == 0) && (databaselist != NULL)) {
        TRACE(0,"Enum record:\n");
        DDB_Print_Node(databaselist->record);
        TRACE(0,"Enumeration ends!\n\n");
        return BT_STS_SUCCESS;
    }

    if(databaselist == NULL) {
        TRACE(0,"Enumeration failed.\n");
        return BT_STS_FAILED;
    }

    return BT_STS_FAILED;
}
#endif



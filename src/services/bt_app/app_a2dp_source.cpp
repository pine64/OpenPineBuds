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
#if defined(APP_LINEIN_A2DP_SOURCE) || defined(APP_I2S_A2DP_SOURCE)

#include "app_a2dp_source.h"
#include "a2dp_api.h"
#include "app_overlay.h"

uint8_t *a2dp_linein_buff;
static char a2dp_transmit_buffer[A2DP_TRANS_SIZE];

sbcbank_t sbcbank;

A2DP_SOURCE_STRUCT a2dp_source;

static sbcpack_t *get_sbcPacket(void) {
  int index = sbcbank.free;
  sbcbank.free += 1;
  if (sbcbank.free == 1) {
    sbcbank.free = 0;
  }
  return &(sbcbank.sbcpacks[index]);
}

#if 0
typedef struct {
    AvrcpAdvancedPdu pdu;
    uint8_t para_buf[10];
}APP_A2DP_AVRCPADVANCEDPDU;



extern osPoolId   app_a2dp_avrcpadvancedpdu_mempool;

#define app_a2dp_avrcpadvancedpdu_mempool_init()                               \
  do {                                                                         \
    if (app_a2dp_avrcpadvancedpdu_mempool == NULL)                             \
      app_a2dp_avrcpadvancedpdu_mempool =                                      \
          osPoolCreate(osPool(app_a2dp_avrcpadvancedpdu_mempool));             \
  } while (0);

#define app_a2dp_avrcpadvancedpdu_mempool_calloc(buf)                          \
  do {                                                                         \
    APP_A2DP_AVRCPADVANCEDPDU *avrcpadvancedpdu;                               \
    avrcpadvancedpdu = (APP_A2DP_AVRCPADVANCEDPDU *)osPoolCAlloc(              \
        app_a2dp_avrcpadvancedpdu_mempool);                                    \
    buf = &(avrcpadvancedpdu->pdu);                                            \
    buf->parms = avrcpadvancedpdu->para_buf;                                   \
  } while (0);

#define app_a2dp_avrcpadvancedpdu_mempool_free(buf)                            \
  do {                                                                         \
    osPoolFree(app_a2dp_avrcpadvancedpdu_mempool, buf);                        \
  } while (0);


void a2dp_source_volume_local_set(int8_t vol)
{
    app_bt_stream_volume_get_ptr()->a2dp_vol = vol;
    nv_record_touch_cause_flush();
}

static int a2dp_volume_get(void)
{
    int vol = app_bt_stream_volume_get_ptr()->a2dp_vol;

    vol = 8*vol-1;
    if (vol > (0x7f-1))
        vol = 0x7f;

    return (vol);
}


static int a2dp_volume_set(U8 vol)
{
    int dest_vol;

    dest_vol = (((int)vol&0x7f)<<4)/0x7f + 1;

    if (dest_vol > TGT_VOLUME_LEVEL_15)
        dest_vol = TGT_VOLUME_LEVEL_15;
    if (dest_vol < TGT_VOLUME_LEVEL_0)
        dest_vol = TGT_VOLUME_LEVEL_0;

    a2dp_source_volume_local_set(dest_vol);
    app_bt_stream_volumeset(dest_vol);

    return (vol);
}



void avrcp_source_callback_TG(AvrcpChannel *chnl, const AvrcpCallbackParms *Parms)
{
    TRACE(3,"%s : chnl %p, Parms %p,Parms->event\n", __FUNCTION__,chnl, Parms,Parms->event);

    enum BT_DEVICE_ID_T device_id = BT_DEVICE_ID_1;
    switch(Parms->event)
    {
        case AVRCP_EVENT_CONNECT:
            if(0)//(chnl->avrcpVersion >=0x103)
            {
                TRACE(1,"::%s AVRCP_GET_CAPABILITY\n",__FUNCTION__);
                if (app_bt_device.avrcp_cmd1[device_id] == NULL)
                    app_a2dp_avrcpadvancedpdu_mempool_calloc(app_bt_device.avrcp_cmd1[device_id]);
                AVRCP_CtGetCapabilities(chnl,app_bt_device.avrcp_cmd1[device_id],AVRCP_CAPABILITY_EVENTS_SUPPORTED);
            }

            app_bt_device.avrcp_channel[device_id].avrcpState = AVRCP_STATE_CONNECTED;

            TRACE(3,"::%s  AVRCP_EVENT_CONNECT %x,device_id=%d\n",__FUNCTION__, chnl->avrcpVersion,device_id);
            break;
        case AVRCP_EVENT_DISCONNECT:
            TRACE(1,"::%s  AVRCP_EVENT_DISCONNECT",__FUNCTION__);
            app_bt_device.avrcp_channel[device_id].avrcpState = AVRCP_STATE_DISCONNECTED;
            if (app_bt_device.avrcp_get_capabilities_rsp[device_id]){
                app_a2dp_avrcpadvancedpdu_mempool_free(app_bt_device.avrcp_get_capabilities_rsp[device_id]);
                app_bt_device.avrcp_get_capabilities_rsp[device_id] = NULL;
            }
            if (app_bt_device.avrcp_control_rsp[device_id]){
                app_a2dp_avrcpadvancedpdu_mempool_free(app_bt_device.avrcp_control_rsp[device_id]);
                app_bt_device.avrcp_control_rsp[device_id] = NULL;
            }
            if (app_bt_device.avrcp_notify_rsp[device_id]){
                app_a2dp_avrcpadvancedpdu_mempool_free(app_bt_device.avrcp_notify_rsp[device_id]);
                app_bt_device.avrcp_notify_rsp[device_id] = NULL;
            }

            if (app_bt_device.avrcp_cmd1[device_id]){
                app_a2dp_avrcpadvancedpdu_mempool_free(app_bt_device.avrcp_cmd1[device_id]);
                app_bt_device.avrcp_cmd1[device_id] = NULL;
            }
            if (app_bt_device.avrcp_cmd2[device_id]){
                app_a2dp_avrcpadvancedpdu_mempool_free(app_bt_device.avrcp_cmd2[device_id]);
                app_bt_device.avrcp_cmd2[device_id] = NULL;
            }
            app_bt_device.volume_report[device_id] = 0;
            break;
        case AVRCP_EVENT_RESPONSE:
            TRACE(3,"::%s  AVRCP_EVENT_RESPONSE op=%x,status=%x\n",__FUNCTION__, Parms->advOp,Parms->status);

            break;
        case AVRCP_EVENT_PANEL_CNF:
            TRACE(4,"::%s AVRCP_EVENT_PANEL_CNF %x,%x,%x",__FUNCTION__,
                Parms->p.panelCnf.response,Parms->p.panelCnf.operation,Parms->p.panelCnf.press);
            break;
        case AVRCP_EVENT_ADV_TX_DONE:
            TRACE(4,"::%s AVRCP_EVENT_ADV_TX_DONE device_id=%d,status=%x,errorcode=%x\n",__FUNCTION__,device_id,Parms->status,Parms->errorCode);
            TRACE(3,"::%s AVRCP_EVENT_ADV_TX_DONE op:%d, transid:%x\n", __FUNCTION__,Parms->p.adv.txPdu->op,Parms->p.adv.txPdu->transId);
            if (Parms->p.adv.txPdu->op == AVRCP_OP_GET_CAPABILITIES){
                if (app_bt_device.avrcp_get_capabilities_rsp[device_id] == Parms->p.adv.txPdu){
                    app_bt_device.avrcp_get_capabilities_rsp[device_id] = NULL;
                    app_a2dp_avrcpadvancedpdu_mempool_free(Parms->p.adv.txPdu);
                }
            }

            break;
        case AVRCP_EVENT_COMMAND:
#ifndef __AVRCP_EVENT_COMMAND_VOLUME_SKIP__
            TRACE(3,"::%s AVRCP_EVENT_COMMAND device_id=%d,role=%x\n",__FUNCTION__,device_id,chnl->role);
            TRACE(3,"::%s AVRCP_EVENT_COMMAND ctype=%x,subunitype=%x\n",__FUNCTION__, Parms->p.cmdFrame->ctype,Parms->p.cmdFrame->subunitType);
            TRACE(3,"::%s AVRCP_EVENT_COMMAND subunitId=%x,opcode=%x\n",__FUNCTION__, Parms->p.cmdFrame->subunitId,Parms->p.cmdFrame->opcode);
            TRACE(3,"::%s AVRCP_EVENT_COMMAND operands=%x,operandLen=%x\n",__FUNCTION__, Parms->p.cmdFrame->operands,Parms->p.cmdFrame->operandLen);
            TRACE(2,"::%s AVRCP_EVENT_COMMAND more=%x\n", Parms->p.cmdFrame->more);
            if(Parms->p.cmdFrame->ctype == AVRCP_CTYPE_STATUS)
            {
                uint32_t company_id = *(Parms->p.cmdFrame->operands+2) + ((uint32_t)(*(Parms->p.cmdFrame->operands+1))<<8) + ((uint32_t)(*(Parms->p.cmdFrame->operands))<<16);
                TRACE(2,"::%s AVRCP_EVENT_COMMAND company_id=%x\n",__FUNCTION__, company_id);
                if(company_id == 0x001958)  //bt sig
                {
                    AvrcpOperation op = *(Parms->p.cmdFrame->operands+3);
                    uint8_t oplen =  *(Parms->p.cmdFrame->operands+6)+ ((uint32_t)(*(Parms->p.cmdFrame->operands+5))<<8);
                    TRACE(3,"::%s AVRCP_EVENT_COMMAND op=%x,oplen=%x\n",__FUNCTION__, op,oplen);
                    switch(op)
                    {
                        case AVRCP_OP_GET_CAPABILITIES:
                        {
                                uint8_t event = *(Parms->p.cmdFrame->operands+7);
                                if(event==AVRCP_CAPABILITY_COMPANY_ID)
                                {
                                    TRACE(1,"::%s AVRCP_EVENT_COMMAND send support compay id",__FUNCTION__);
                                }
                                else if(event == AVRCP_CAPABILITY_EVENTS_SUPPORTED)
                                {
                                    TRACE(2,"::%s AVRCP_EVENT_COMMAND send support event transId:%d", __FUNCTION__,Parms->p.cmdFrame->transId);
                                    chnl->adv.eventMask = AVRCP_ENABLE_VOLUME_CHANGED;   ///volume control
                                    if (app_bt_device.avrcp_get_capabilities_rsp[device_id] == NULL)
                                        app_a2dp_avrcpadvancedpdu_mempool_calloc(app_bt_device.avrcp_get_capabilities_rsp[device_id]);
                                    app_bt_device.avrcp_get_capabilities_rsp[device_id]->transId = Parms->p.cmdFrame->transId;
                                    app_bt_device.avrcp_get_capabilities_rsp[device_id]->ctype = AVCTP_RESPONSE_IMPLEMENTED_STABLE;
                                    TRACE(2,"::%s AVRCP_EVENT_COMMAND send support event transId:%d", __FUNCTION__,app_bt_device.avrcp_get_capabilities_rsp[device_id]->transId);
                                    AVRCP_CtGetCapabilities_Rsp(chnl,app_bt_device.avrcp_get_capabilities_rsp[device_id],AVRCP_CAPABILITY_EVENTS_SUPPORTED,chnl->adv.eventMask);
                                }
                                else
                                {
                                    TRACE(1,"::%s AVRCP_EVENT_COMMAND send error event value",__FUNCTION__);
                                }
                        }
                        break;
                    }

                }

            }else if(Parms->p.cmdFrame->ctype == AVCTP_CTYPE_CONTROL){
                TRACE(1,"::%s AVRCP_EVENT_COMMAND AVCTP_CTYPE_CONTROL\n",__FUNCTION__);
                DUMP8("%02x ", Parms->p.cmdFrame->operands, Parms->p.cmdFrame->operandLen);
                if (Parms->p.cmdFrame->operands[3] == AVRCP_OP_SET_ABSOLUTE_VOLUME){
                    TRACE(2,"::%s AVRCP_EID_VOLUME_CHANGED transId:%d\n", __FUNCTION__,Parms->p.cmdFrame->transId);
                    a2dp_volume_set(Parms->p.cmdFrame->operands[7]);
                    if (app_bt_device.avrcp_control_rsp[device_id] == NULL)
                        app_a2dp_avrcpadvancedpdu_mempool_calloc(app_bt_device.avrcp_control_rsp[device_id]);
                    app_bt_device.avrcp_control_rsp[device_id]->transId = Parms->p.cmdFrame->transId;
                    app_bt_device.avrcp_control_rsp[device_id]->ctype = AVCTP_RESPONSE_ACCEPTED;
                    DUMP8("%02x ", Parms->p.cmdFrame->operands, Parms->p.cmdFrame->operandLen);
                    AVRCP_CtAcceptAbsoluteVolume_Rsp(chnl, app_bt_device.avrcp_control_rsp[device_id], Parms->p.cmdFrame->operands[7]);
                }
            }else if (Parms->p.cmdFrame->ctype == AVCTP_CTYPE_NOTIFY){
                BtStatus status;
                TRACE(1,"::%s AVRCP_EVENT_COMMAND AVCTP_CTYPE_NOTIFY\n",__FUNCTION__);
                DUMP8("%02x ", Parms->p.cmdFrame->operands, Parms->p.cmdFrame->operandLen);
                if (Parms->p.cmdFrame->operands[7] == AVRCP_EID_VOLUME_CHANGED){
                    TRACE(2,"::%s AVRCP_EID_VOLUME_CHANGED transId:%d\n", __FUNCTION__,Parms->p.cmdFrame->transId);
                    if (app_bt_device.avrcp_notify_rsp[device_id] == NULL)
                        app_a2dp_avrcpadvancedpdu_mempool_calloc(app_bt_device.avrcp_notify_rsp[device_id]);
                    app_bt_device.avrcp_notify_rsp[device_id]->transId = Parms->p.cmdFrame->transId;
                    app_bt_device.avrcp_notify_rsp[device_id]->ctype = AVCTP_RESPONSE_INTERIM;
                    app_bt_device.volume_report[device_id] = AVCTP_RESPONSE_INTERIM;
                    status = AVRCP_CtGetAbsoluteVolume_Rsp(chnl, app_bt_device.avrcp_notify_rsp[device_id], a2dp_volume_get());
                    TRACE(2,"::%s AVRCP_EVENT_COMMAND AVRCP_EID_VOLUME_CHANGED nRet:%x\n",__FUNCTION__,status);
                }
            }
#endif
            break;
        case AVRCP_EVENT_ADV_CMD_TIMEOUT:
            TRACE(3,"::%s AVRCP_EVENT_ADV_CMD_TIMEOUT device_id=%d,role=%x\n",__FUNCTION__,device_id,chnl->role);
            break;
    }
}
#endif

osMutexId a2dp_source_mutex_id = NULL;
osMutexDef(a2dp_source_mutex);

static void a2dp_source_mutex_lock(void) {
  osMutexWait(a2dp_source_mutex_id, osWaitForever);
}

static void a2dp_source_mutex_unlock(void) {
  osMutexRelease(a2dp_source_mutex_id);
}

static void a2dp_source_sem_lock(a2dp_source_lock_t *lock) {
  osSemaphoreWait(lock->_osSemaphoreId, osWaitForever);
}

static void a2dp_source_sem_unlock(a2dp_source_lock_t *lock) {

  osSemaphoreRelease(lock->_osSemaphoreId);
}

static void a2dp_source_reset_send_lock(void) {
  PSCB p_scb = (PSCB)(a2dp_source.sbc_send_lock._osSemaphoreDef.semaphore);
  uint32_t lock = int_lock();
  p_scb->tokens = 0;
  int_unlock(lock);
}

static bool a2dp_source_is_send_wait(void) {
  bool ret = false;
  uint32_t lock = int_lock();
  PSCB p_scb = (PSCB)(a2dp_source.sbc_send_lock._osSemaphoreDef.semaphore);
  if (p_scb->p_lnk) {
    ret = true;
  }
  int_unlock(lock);
  return ret;
}

static void a2dp_source_wait_pcm_data(void) {

  a2dp_source_lock_t *lock = &(a2dp_source.data_lock);
  PSCB p_scb = (PSCB)(lock->_osSemaphoreDef.semaphore);
  uint32_t iflag = int_lock();
  p_scb->tokens = 0;
  int_unlock(iflag);
  a2dp_source_sem_lock(lock);
}

static void a2dp_source_put_data(void) {
  a2dp_source_lock_t *lock = &(a2dp_source.data_lock);
  a2dp_source_sem_unlock(lock);
}

#if 0
static void a2dp_source_lock_sbcsending(void * channel)
{
    tws_lock_t *lock = &(a2dp_source.sbc_send_lock);
    sem_lock(lock);
}
static void a2dp_source_unlock_sbcsending(void * channel)
{
    tws_lock_t *lock = &(a2dp_source.sbc_send_lock);
    sem_unlock(lock);
}
#endif

static int32_t a2dp_source_wait_sent(uint32_t timeout) {
  int32_t ret = 0;
  a2dp_source_lock_t *lock = &(a2dp_source.sbc_send_lock);
  a2dp_source_reset_send_lock();
  ret = osSemaphoreWait(lock->_osSemaphoreId, timeout);
  return ret;
}

void a2dp_source_notify_send(void) {
  if (a2dp_source_is_send_wait()) { // task wait lock
    //   TWS_DBLOG("\nNOTIFY SEND\n");
    a2dp_source_sem_unlock(&(a2dp_source.sbc_send_lock));
  }
}

// loop write
static int A2dpSourceEnCQueue(CQueue *Q, CQItemType *e, unsigned int len) {
  int status = CQ_OK;
  if (AvailableOfCQueue(Q) < (int)len) {
    Q->write = 0;
    Q->len = 0;
    Q->read = 0;
    status = CQ_ERR;
  } else {
    status = CQ_OK;
  }

  Q->len += len;

  while (len > 0) {
    Q->base[Q->write] = *e;

    ++Q->write;
    ++e;
    --len;

    if (Q->write >= Q->size)
      Q->write = 0;
  }

  // TRACE(1,"Q->len:%d ", Q->len);

  return status;
}

static int a2dp_source_pcm_buffer_write(uint8_t *pcm_buf, uint16_t len) {
  int status;
  // TWS_DBLOG("\nenter: %s %d\n",__FUNCTION__,__LINE__);
  a2dp_source_mutex_lock();
  status = A2dpSourceEnCQueue(&(a2dp_source.pcm_queue), pcm_buf, len);
  a2dp_source_mutex_unlock();
  // TWS_DBLOG("\nexit: %s %d\n",__FUNCTION__,__LINE__);
  return status;
}

static int a2dp_source_pcm_buffer_read(uint8_t *buff, uint16_t len) {
  uint8_t *e1 = NULL, *e2 = NULL;
  unsigned int len1 = 0, len2 = 0;
  int status;
  a2dp_source_mutex_lock();
  status = PeekCQueue(&(a2dp_source.pcm_queue), len, &e1, &len1, &e2, &len2);
  if (len == (len1 + len2)) {
    memcpy(buff, e1, len1);
    memcpy(buff + len1, e2, len2);
    DeCQueue(&(a2dp_source.pcm_queue), 0, len);
  } else {
    // SOURCE_DBLOG("memset buffer");
    memset(buff, 0x00, len);
    status = -1;
  }
  a2dp_source_mutex_unlock();
  return status;
}

uint32_t a2dp_source_linein_more_pcm_data(uint8_t *pcm_buf, uint32_t len) {
#if 1
  int status;
  status = a2dp_source_pcm_buffer_write(pcm_buf, len);
  // pcm data from adc
  // DUMP8("%02x ",pcm_buf, 10);
  if (status != CQ_OK) {
    SOURCE_DBLOG("linin buff overflow!");
  }
  a2dp_source_put_data();
  return len;
#else
  int status;
  status = a2dp_source_pcm_buffer_write(pcm_buf, len);
  // pcm data from adc
  // DUMP8("%02x ",pcm_buf, 10);
  if (status != CQ_OK) {
    SOURCE_DBLOG("linin buff overflow!");
  }
  if (((app_bt_device.input_onoff == 1) &&
       (a2dp_source.pcm_queue.len > 10 * 1024)) ||
      (app_bt_device.input_onoff == 2)) {
    a2dp_source_put_data();
    app_bt_device.input_onoff = 2;
  }

  return len;

#endif
}

static btif_handler a2dp_source_handler;
static uint8_t app_a2dp_source_find_process = 0;

void app_a2dp_source_stop_find(void) {
  app_a2dp_source_find_process = 0;
  // ME_UnregisterGlobalHandler(&a2dp_source_handler);
}

#if 0
static void bt_a2dp_source_call_back(const BtEvent* event)
{
    switch (event->eType) {
        case BTEVENT_HCI_COMMAND_SENT:
		case BTEVENT_ACL_DATA_NOT_ACTIVE:
            return;
        case BTEVENT_ACL_DATA_ACTIVE:
            CmgrHandler    *cmgrHandler;
            /* Start the sniff timer */
            cmgrHandler = CMGR_GetAclHandler(event->p.remDev);
            if (cmgrHandler)
                app_bt_CMGR_SetSniffTimer(cmgrHandler, NULL, CMGR_SNIFF_TIMER);
            return;
    }

    TRACE(1,"SRC app_bt_golbal_handle evt = %d",event->eType);

    switch(event->eType){
        case BTEVENT_NAME_RESULT:
            SOURCE_DBLOG("\n%s %d BTEVENT_NAME_RESULT\n",__FUNCTION__,__LINE__);
            break;
        case BTEVENT_INQUIRY_RESULT:
            SOURCE_DBLOG("\n%s %d BTEVENT_INQUIRY_RESULT\n",__FUNCTION__,__LINE__);
            DUMP8("%02x ", event->p.inqResult.bdAddr.addr, 6);
            SOURCE_DBLOG("inqmode = %x",event->p.inqResult.inqMode);
            DUMP8("%02x ", event->p.inqResult.extInqResp, 20);
            SOURCE_DBLOG("classdevice=%x",event->p.inqResult.classOfDevice);
            ///check the class of device to find the handfree device
            if(event->p.inqResult.classOfDevice & COD_MAJOR_AUDIO)
            //if((event->p.inqResult.classOfDevice & COD_MAJOR_AUDIO)&&(!memcmp(event->p.inqResult.bdAddr.addr,"\x36\x35\x34",3)))
            {
            	memcpy(app_bt_device.inquried_snk_bdAddr.addr,event->p.inqResult.bdAddr.addr,sizeof(event->p.inqResult.bdAddr.addr));
				DUMP8("%02x ",app_bt_device.inquried_snk_bdAddr.addr, 6);
                ME_CancelInquiry();
                app_a2dp_source_stop_find();
            }

            break;
        case BTEVENT_INQUIRY_COMPLETE:
            SOURCE_DBLOG("\n%s %d BTEVENT_INQUIRY_COMPLETE\n",__FUNCTION__,__LINE__);
            app_a2dp_source_stop_find();
            break;
            /** The Inquiry process is canceled. */
        case BTEVENT_INQUIRY_CANCELED:
            SOURCE_DBLOG("\n%s %d BTEVENT_INQUIRY_CANCELED\n",__FUNCTION__,__LINE__);
			SOURCE_DBLOG("start to connect peer device");
            A2DP_OpenStream(&app_bt_device.a2dp_stream[BT_DEVICE_ID_1], (BT_BD_ADDR *)&app_bt_device.inquried_snk_bdAddr);
            break;
        case BTEVENT_LINK_CONNECT_CNF:
        case BTEVENT_LINK_CONNECT_IND:
			SOURCE_DBLOG("CONNECT_IND/CNF evt:%d errCode:0x%0x newRole:%d activeCons:%d",event->eType, event->errCode, event->p.remDev->role, MEC(activeCons));
#if defined(__BTIF_EARPHONE__) && defined(__BTIF_AUTOPOWEROFF__) 
			if (MEC(activeCons) == 0){
			    app_start_10_second_timer(APP_POWEROFF_TIMER_ID);
			}else{
			    app_stop_10_second_timer(APP_POWEROFF_TIMER_ID);
			}
#endif
			if (MEC(activeCons) > 1){
			    app_bt_MeDisconnectLink(event->p.remDev);
			}
        default:
            //SOURCE_DBLOG("\n%s %d etype:%d\n",__FUNCTION__,__LINE__,event->eType);
            break;

    }

	app_bt_role_manager_process(event);
    app_bt_accessible_manager_process(event);
    app_bt_sniff_manager_process(event);
    app_bt_golbal_handle_hook(event);

    //SOURCE_DBLOG("\nexit: %s %d\n",__FUNCTION__,__LINE__);

}

#else
uint8_t source_bt_addr[6] = {0x88, 0xaa, 0x33, 0x22, 0x11, 0x11};
// uint8_t source_bt_addr[6]={0x85,0x7e,0xaa,0x3c,0xd0,0xe8};

static void bt_a2dp_source_call_back(const btif_event_t *Event) {

  uint8_t etype = btif_me_get_callback_event_type(Event);
  btif_remote_device_t *remDev;

  switch (etype) {
  case BTIF_BTEVENT_HCI_COMMAND_SENT:
  case BTIF_BTEVENT_ACL_DATA_NOT_ACTIVE:
    return;
  case BTIF_BTEVENT_ACL_DATA_ACTIVE:

    btif_cmgr_handler_t *cmgrHandler;
    /* Start the sniff timer */
    cmgrHandler =
        btif_cmgr_get_acl_handler(btif_me_get_callback_event_rem_dev(Event));
    if (cmgrHandler)
      app_bt_CMGR_SetSniffTimer(cmgrHandler, NULL, BTIF_CMGR_SNIFF_TIMER);
    return;
  }

  TRACE(1, "SRC app_bt_golbal_handle evt = %d", etype);

  switch (etype) {
  case BTIF_BTEVENT_NAME_RESULT:
    SOURCE_DBLOG("\n%s %d BTEVENT_NAME_RESULT\n", __FUNCTION__, __LINE__);
    break;
  case BTIF_BTEVENT_INQUIRY_RESULT:
    remDev = btif_me_get_callback_event_rem_dev(Event);
    SOURCE_DBLOG("\n%s %d BTEVENT_INQUIRY_RESULT\n", __FUNCTION__, __LINE__);
    DUMP8("%02x ", btif_me_get_callback_event_inq_result_bd_addr_addr(Event),
          6);
    SOURCE_DBLOG("inqmode = %x",
                 btif_me_get_callback_event_inq_result_inq_mode(Event));
    DUMP8("%02x ", btif_me_get_callback_event_inq_result_ext_inq_resp(Event),
          20);
    SOURCE_DBLOG("classdevice=%x",
                 btif_me_get_callback_event_inq_result_classofdevice(Event));
    /// check the class of device to find the handfree device
    // if(btif_me_get_callback_event_inq_result_classofdevice(Event) &
    // BTIF_COD_MAJOR_AUDIO)
    if ((btif_me_get_callback_event_inq_result_classofdevice(Event) &
         BTIF_COD_MAJOR_AUDIO) &&
        (!memcmp(btif_me_get_callback_event_inq_result_bd_addr_addr(Event),
                 source_bt_addr, 6))) {
      memcpy(app_bt_device.inquried_snk_bdAddr.addr,
             btif_me_get_callback_event_inq_result_bd_addr_addr(Event), 6);
      DUMP8("%02x ", app_bt_device.inquried_snk_bdAddr.addr, 6);
      btif_me_cancel_inquiry();
      app_a2dp_source_stop_find();
    }

    break;
  case BTIF_BTEVENT_INQUIRY_COMPLETE:
    SOURCE_DBLOG("\n%s %d BTEVENT_INQUIRY_COMPLETE\n", __FUNCTION__, __LINE__);
    app_a2dp_source_stop_find();
    break;
    /** The Inquiry process is canceled. */
  case BTIF_BTEVENT_INQUIRY_CANCELED:
    SOURCE_DBLOG("\n%s %d BTEVENT_INQUIRY_CANCELED\n", __FUNCTION__, __LINE__);
    SOURCE_DBLOG("start to connect peer device");
    btif_a2dp_open_stream(
        app_bt_device.a2dp_stream[BT_DEVICE_ID_1]->a2dp_stream,
        (bt_bdaddr_t *)&app_bt_device.inquried_snk_bdAddr);
    break;
  case BTIF_BTEVENT_LINK_CONNECT_CNF:
  case BTIF_BTEVENT_LINK_CONNECT_IND:
    SOURCE_DBLOG(
        "CONNECT_IND/CNF evt:%d errCode:0x%0x newRole:%d activeCons:%d", etype,
        btif_me_get_callback_event_err_code(Event),
        btif_me_get_remote_device_role(
            btif_me_get_callback_event_rem_dev(Event)),
        btif_me_get_activeCons());
#if defined(__BTIF_EARPHONE__) && defined(__BTIF_AUTOPOWEROFF__)
    if (btif_me_get_activeCons() == 0) {
      app_start_10_second_timer(APP_POWEROFF_TIMER_ID);
    } else {
      app_stop_10_second_timer(APP_POWEROFF_TIMER_ID);
    }
#endif
    if (btif_me_get_activeCons() > 1) {
      remDev = btif_me_get_callback_event_rem_dev(Event);
      app_bt_MeDisconnectLink(remDev);
    }
  default:
    // SOURCE_DBLOG("\n%s %d etype:%d\n",__FUNCTION__,__LINE__,event->eType);
    break;
  }

  app_bt_role_manager_process(Event);
  app_bt_accessible_manager_process(Event);
  app_bt_sniff_manager_process(Event);

  // SOURCE_DBLOG("\nexit: %s %d\n",__FUNCTION__,__LINE__);
}

#endif

void app_a2dp_start_stream(void) {
  btif_a2dp_start_stream(&app_bt_device.a2dp_stream[BT_DEVICE_ID_1]);
}

void app_a2dp_suspend_stream(void) {
  btif_a2dp_suspend_stream(&app_bt_device.a2dp_stream[BT_DEVICE_ID_1]);
}

static void _find_a2dp_sink_peer_device_start(void) {
  SOURCE_DBLOG("\nenter: %s %d\n", __FUNCTION__, __LINE__);
  bt_status_t state;
  if (app_a2dp_source_find_process == 0 && app_bt_device.a2dp_state[0] == 0) {
    app_a2dp_source_find_process = 1;

  again:
    state = btif_me_inquiry(BTIF_BT_IAC_GIAC, 30, 0);
    SOURCE_DBLOG("\n%s %d\n", __FUNCTION__, __LINE__);
    if (state != BT_STS_PENDING) {
      osDelay(500);
      goto again;
    }
    SOURCE_DBLOG("\n%s %d\n", __FUNCTION__, __LINE__);
  }
}

void app_a2dp_source_find_sink(void) { _find_a2dp_sink_peer_device_start(); }

static bool need_init_encoder = true;
//__PSRAMDATA static SbcStreamInfo StreamInfo = {0};
static btif_sbc_stream_info_short_t StreamInfo = {0};

static btif_sbc_encoder_t sbc_encoder;

static uint8_t
app_a2dp_source_samplerate_2_sbcenc_type(enum AUD_SAMPRATE_T sample_rate) {
  uint8_t rate = BTIF_SBC_CHNL_SAMPLE_FREQ_16;
  switch (sample_rate) {
  case AUD_SAMPRATE_16000:
    rate = BTIF_SBC_CHNL_SAMPLE_FREQ_16;
    break;
  case AUD_SAMPRATE_32000:
    rate = BTIF_SBC_CHNL_SAMPLE_FREQ_32;
    break;
  case AUD_SAMPRATE_44100:
    rate = BTIF_SBC_CHNL_SAMPLE_FREQ_44_1;
    break;
  case AUD_SAMPRATE_48000:
    rate = BTIF_SBC_CHNL_SAMPLE_FREQ_48;
    break;
  default:
    TRACE(0, "error!  sbc enc don't support other samplerate");
    break;
  }
  SOURCE_DBLOG("\n%s %d rate = %x\n", __FUNCTION__, __LINE__, rate);
  return rate;
}

#if 1
static void a2dp_source_send_sbc_packet(void) {
  uint32_t frame_size = 512;
  uint32_t frame_num = A2DP_TRANS_SIZE / frame_size;
  //    a2dp_source.lock_stream(&(tws.tws_source));
  uint16_t byte_encoded = 0;
  //    uint16_t pcm_frame_size = 512/2;
  unsigned short enc_len = 0;
  bt_status_t status = BT_STS_FAILED;

  int lock = int_lock();
  sbcpack_t *sbcpack = get_sbcPacket();
  btif_a2dp_sbc_packet_t *sbcPacket = &(sbcpack->sbcPacket);
  sbcPacket->data = (U8 *)sbcpack->buffer;
  memcpy(sbcpack->buffer, a2dp_transmit_buffer, A2DP_TRANS_SIZE);

  //    sbcPacket->dataLen = len;
  //    sbcPacket->frameSize = len/frame_num;

  btif_sbc_pcm_data_t PcmEncData;

  if (need_init_encoder) {
    btif_sbc_init_encoder(&sbc_encoder);
    sbc_encoder.streamInfo.numChannels = 2;
    sbc_encoder.streamInfo.channelMode = BTIF_SBC_CHNL_MODE_JOINT_STEREO;
    sbc_encoder.streamInfo.bitPool = A2DP_SBC_BITPOOL;
    sbc_encoder.streamInfo.sampleFreq =
        app_a2dp_source_samplerate_2_sbcenc_type(a2dp_source.sample_rate);
    sbc_encoder.streamInfo.allocMethod = BTIF_SBC_ALLOC_METHOD_SNR;
    sbc_encoder.streamInfo.numBlocks = 16;
    sbc_encoder.streamInfo.numSubBands = 8;
    sbc_encoder.streamInfo.mSbcFlag = 0;
    need_init_encoder = 0;
  }
  PcmEncData.data = (uint8_t *)a2dp_transmit_buffer;
  PcmEncData.dataLen = A2DP_TRANS_SIZE;
  PcmEncData.numChannels = 2;
  PcmEncData.sampleFreq = sbc_encoder.streamInfo.sampleFreq;

  btif_sbc_encode_frames(&sbc_encoder, &PcmEncData, &byte_encoded,
                         (unsigned char *)sbcpack->buffer, &enc_len,
                         A2DP_TRANS_SIZE);
  sbcPacket->dataLen = enc_len;
  sbcPacket->frameSize = enc_len / frame_num;

  int_unlock(lock);
  status = btif_a2dp_stream_send_sbc_packet(
      app_bt_device.a2dp_stream[BT_DEVICE_ID_1]->a2dp_stream, sbcPacket,
      &StreamInfo);
  if (status == BT_STS_PENDING)
    a2dp_source_wait_sent(osWaitForever);
}
#endif

// #define BT_A2DP_SOURCE_LINEIN_BUFF_SIZE    	(512*5*2*2)
#define BT_A2DP_SOURCE_LINEIN_BUFF_SIZE (A2DP_TRANS_SIZE * 2)

#if defined(APP_LINEIN_A2DP_SOURCE)
//////////start the audio linein stream for capure the pcm data
int app_a2dp_source_linein_on(bool on) {
  uint8_t *buff_play = NULL;
  struct AF_STREAM_CONFIG_T stream_cfg;
  static bool isRun = false;
  SOURCE_DBLOG("app_a2dp_source_linein_on work:%d op:%d", isRun, on);

  if (isRun == on)
    return 0;

  if (on) {
#if defined(SLAVE_USE_OPUS) || defined(MASTER_USE_OPUS) || defined(ALL_USE_OPUS)
    app_audio_mempool_init(app_audio_get_basebuf_ptr(APP_MEM_LINEIN_AUDIO),
                           app_audio_get_basebuf_size(APP_MEM_LINEIN_AUDIO));
#else
    app_audio_mempool_init();
#endif
    app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_104M);
    app_audio_mempool_get_buff(&buff_play, BT_A2DP_SOURCE_LINEIN_BUFF_SIZE);

    memset(&stream_cfg, 0, sizeof(stream_cfg));

    stream_cfg.bits = AUD_BITS_16;
    stream_cfg.channel_num = AUD_CHANNEL_NUM_2;
    stream_cfg.sample_rate = a2dp_source.sample_rate;
    stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
    stream_cfg.vol = 10;
    // stream_cfg.io_path = AUD_INPUT_PATH_HP_MIC;
    // stream_cfg.io_path =AUD_INPUT_PATH_MAINMIC;
    stream_cfg.io_path = AUD_INPUT_PATH_LINEIN;
    stream_cfg.handler = a2dp_source_linein_more_pcm_data;
    stream_cfg.data_ptr = buff_play;
    stream_cfg.data_size = BT_A2DP_SOURCE_LINEIN_BUFF_SIZE;
    af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE, &stream_cfg);
    af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
    SOURCE_DBLOG("app_source_linein_on on");
  } else {
    af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
    af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
    SOURCE_DBLOG("app_source_linein_on off");
    // clear buffer data
    a2dp_source.pcm_queue.write = 0;
    a2dp_source.pcm_queue.len = 0;
    a2dp_source.pcm_queue.read = 0;
    app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_32K);
  }

  isRun = on;
  return 0;
}
#endif

////////////////////////////creat the thread for send sbc data to a2dp sink
/// device ///////////////////
static void send_thread(const void *arg);

osThreadDef(send_thread, osPriorityHigh, 1, 1024 * 2, "a2dp_send");

#if 1
static void send_thread(const void *arg) {
  while (1) {
    a2dp_source_wait_pcm_data();
    while (a2dp_source_pcm_buffer_read((uint8_t *)a2dp_transmit_buffer,
                                       A2DP_TRANS_SIZE) == 0) {
      a2dp_source_send_sbc_packet();
    }
  }
}
#endif
void app_source_init(void) {
  // register the bt global handler
  a2dp_source_handler.callback = bt_a2dp_source_call_back;
  btif_me_register_global_handler(&a2dp_source_handler);
  btif_me_set_event_mask(
      &a2dp_source_handler,
      BTIF_BEM_LINK_DISCONNECT | BTIF_BEM_ROLE_CHANGE |
          BTIF_BEM_INQUIRY_RESULT | BTIF_BEM_INQUIRY_COMPLETE |
          BTIF_BEM_INQUIRY_CANCELED | BTIF_BEM_LINK_CONNECT_CNF |
          BTIF_BEM_LINK_CONNECT_IND);
}
///////init the a2dp source feature
void app_a2dp_source_init(void) {
  a2dp_source_lock_t *lock;
  // get heap from app_audio_buffer
  app_audio_mempool_get_buff(&a2dp_linein_buff, A2DP_LINEIN_SIZE);
  InitCQueue(&a2dp_source.pcm_queue, A2DP_LINEIN_SIZE,
             (CQItemType *)a2dp_linein_buff);
  if (a2dp_source_mutex_id == NULL) {
    a2dp_source_mutex_id = osMutexCreate((osMutex(a2dp_source_mutex)));
  }

  lock = &(a2dp_source.data_lock);
  memset(lock, 0, sizeof(a2dp_source_lock_t));
  lock->_osSemaphoreDef.semaphore = lock->_semaphore_data;
  lock->_osSemaphoreId = osSemaphoreCreate(&(lock->_osSemaphoreDef), 0);

  lock = &(a2dp_source.sbc_send_lock);
  memset(lock, 0, sizeof(a2dp_source_lock_t));
  lock->_osSemaphoreDef.semaphore = lock->_semaphore_data;
  lock->_osSemaphoreId = osSemaphoreCreate(&(lock->_osSemaphoreDef), 0);
  a2dp_source.sbc_send_id = osThreadCreate(osThread(send_thread), NULL);
  a2dp_source.sample_rate = AUD_SAMPRATE_44100;
}

#endif

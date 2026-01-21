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
#ifdef CHIP_HAS_CP

#include "hal_mcu2cp.h"
#include "cmsis_nvic.h"
#include "hal_location.h"
#include "hal_sleep.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "plat_addr_map.h"
#include CHIP_SPECIFIC_HDR(reg_cmu)

#define MAX_SEND_RECORD_COUNT 3

#define HAL_SYS_WAKE_LOCK_USER_MCU2CP HAL_SYS_WAKE_LOCK_USER_4

#ifdef CP_API
#define API_POSTFIX _cp
#define MCU2CP_TEXT_LOC CP_TEXT_SRAM_LOC
#define MCU2CP_RODATA_LOC CP_DATA_LOC
#define MCU2CP_BSS_LOC CP_BSS_LOC
#define MCU2CP_BSS_DEF CP_BSS_DEF
#else
#define API_POSTFIX _mcu
#define MCU2CP_TEXT_LOC
#define MCU2CP_RODATA_LOC
#define MCU2CP_BSS_LOC
#define MCU2CP_BSS_DEF CP_BSS_DEF
#endif

#define MCU2CP_API_A(n, p) MCU2CP_TEXT_LOC n##p
#define MCU2CP_API_B(n, p) MCU2CP_API_A(n, p)
#define MCU2CP_API(n) MCU2CP_API_B(n, API_POSTFIX)

enum HAL_MCU2CP_IRQ_TYPE_T {
  HAL_MCU2CP_IRQ_SEND_IND,
  HAL_MCU2CP_IRQ_RECV_DONE,

  HAL_MCU2CP_IRQ_TYPE_QTY
};

struct HAL_MCU2CP_MSG_T {
  struct HAL_MCU2CP_MSG_T *next;   // pointer to next element in the list
  enum HAL_MCU2CP_MSG_TYPE_T type; // message type
  unsigned int len;                // message data length in bytes
  const unsigned char *data;       // pointer to the message data
};

struct HAL_MCU2CP_SEND_RECORD_T {
  struct HAL_MCU2CP_MSG_T msg;
  bool in_use;
};

static const IRQn_Type MCU2CP_RODATA_LOC rx_irq_id[HAL_MCU2CP_ID_QTY] = {
    CP2MCU_DATA_IRQn,
    CP2MCU_DATA1_IRQn,
};

static const IRQn_Type MCU2CP_RODATA_LOC tx_irq_id[HAL_MCU2CP_ID_QTY] = {
    MCU2CP_DONE_IRQn,
    MCU2CP_DONE1_IRQn,
};

static const struct HAL_MCU2CP_MSG_T **MCU2CP_BSS_LOC recv_msg_list_p;

static struct HAL_MCU2CP_MSG_T *MCU2CP_BSS_LOC
    send_msg_list_p[HAL_MCU2CP_ID_QTY];
static struct HAL_MCU2CP_MSG_T *MCU2CP_BSS_LOC
    send_pending_list_p[HAL_MCU2CP_ID_QTY];

static struct HAL_MCU2CP_MSG_T MCU2CP_BSS_LOC
    recv_pending_head[HAL_MCU2CP_ID_QTY];

static struct HAL_MCU2CP_SEND_RECORD_T MCU2CP_BSS_LOC
    send_msgs[HAL_MCU2CP_ID_QTY][MAX_SEND_RECORD_COUNT];

static HAL_MCU2CP_RX_IRQ_HANDLER MCU2CP_BSS_LOC
    rx_irq_handler[HAL_MCU2CP_ID_QTY][HAL_MCU2CP_MSG_TYPE_QTY];
static HAL_MCU2CP_TX_IRQ_HANDLER MCU2CP_BSS_LOC
    tx_irq_handler[HAL_MCU2CP_ID_QTY][HAL_MCU2CP_MSG_TYPE_QTY];

static uint8_t MCU2CP_BSS_LOC chan_opened[HAL_MCU2CP_ID_QTY] = {
    0,
    0,
};
STATIC_ASSERT(sizeof(chan_opened[0]) * 8 >= HAL_MCU2CP_MSG_TYPE_QTY,
              "chan_opened size too small");

static bool MCU2CP_BSS_LOC need_flow_ctrl[HAL_MCU2CP_ID_QTY] = {
    false,
    false,
};

#ifndef CP_API
static bool MCU2CP_BSS_LOC chan_busy[HAL_MCU2CP_ID_QTY] = {
    false,
    false,
};

static bool MCU2CP_BSS_LOC busy_now = false;
#endif

static struct CMU_T *const MCU2CP_RODATA_LOC cmu = (struct CMU_T *)CMU_BASE;

static void MCU2CP_TEXT_LOC hal_mcu2cp_busy(enum HAL_MCU2CP_ID_T id,
                                            bool busy) {
#ifndef CP_API
  int i;
  bool new_state;

  if (chan_busy[id] == busy) {
    return;
  }

  chan_busy[id] = busy;

  if (busy_now == busy) {
    return;
  }

  if (busy) {
    hal_sys_wake_lock(HAL_SYS_WAKE_LOCK_USER_MCU2CP);
    busy_now = true;
  } else {
    new_state = false;
    for (i = 0; i < HAL_MCU2CP_ID_QTY; i++) {
      if (chan_busy[i]) {
        new_state = true;
        break;
      }
    }
    if (!new_state) {
      hal_sys_wake_unlock(HAL_SYS_WAKE_LOCK_USER_MCU2CP);
      busy_now = false;
    }
  }
#endif
}

static int MCU2CP_TEXT_LOC hal_mcu2cp_peer_irq_set(
    enum HAL_MCU2CP_ID_T id, enum HAL_MCU2CP_IRQ_TYPE_T type) {
  uint32_t value;

#ifdef CP_API
  if (id == HAL_MCU2CP_ID_0) {
    if (type == HAL_MCU2CP_IRQ_SEND_IND) {
      value = CMU_CP2MCU_DATA_IND_SET;
    } else {
      value = CMU_MCU2CP_DATA_DONE_SET;
    }
  } else {
    if (type == HAL_MCU2CP_IRQ_SEND_IND) {
      value = CMU_CP2MCU_DATA1_IND_SET;
    } else {
      value = CMU_MCU2CP_DATA1_DONE_SET;
    }
  }

  cmu->CP2MCU_IRQ_SET = value;
#else
  if (id == HAL_MCU2CP_ID_0) {
    if (type == HAL_MCU2CP_IRQ_SEND_IND) {
      value = CMU_MCU2CP_DATA_IND_SET;
    } else {
      value = CMU_CP2MCU_DATA_DONE_SET;
    }
  } else {
    if (type == HAL_MCU2CP_IRQ_SEND_IND) {
      value = CMU_MCU2CP_DATA1_IND_SET;
    } else {
      value = CMU_CP2MCU_DATA1_DONE_SET;
    }
  }

  cmu->MCU2CP_IRQ_SET = value;
#endif

  return 0;
}

static int MCU2CP_TEXT_LOC hal_mcu2cp_local_irq_clear(
    enum HAL_MCU2CP_ID_T id, enum HAL_MCU2CP_IRQ_TYPE_T type) {
  uint32_t value;

#ifdef CP_API
  if (id == HAL_MCU2CP_ID_0) {
    if (type == HAL_MCU2CP_IRQ_SEND_IND) {
      value = CMU_MCU2CP_DATA_IND_CLR;
    } else {
      value = CMU_CP2MCU_DATA_DONE_CLR;
    }
  } else {
    if (type == HAL_MCU2CP_IRQ_SEND_IND) {
      value = CMU_MCU2CP_DATA1_IND_CLR;
    } else {
      value = CMU_CP2MCU_DATA1_DONE_CLR;
    }
  }

  cmu->MCU2CP_IRQ_CLR = value;
#else
  if (id == HAL_MCU2CP_ID_0) {
    if (type == HAL_MCU2CP_IRQ_SEND_IND) {
      value = CMU_CP2MCU_DATA_IND_CLR;
    } else {
      value = CMU_MCU2CP_DATA_DONE_CLR;
    }
  } else {
    if (type == HAL_MCU2CP_IRQ_SEND_IND) {
      value = CMU_CP2MCU_DATA1_IND_CLR;
    } else {
      value = CMU_MCU2CP_DATA1_DONE_CLR;
    }
  }

  cmu->CP2MCU_IRQ_CLR = value;
#endif

  return 0;
}

static int MCU2CP_TEXT_LOC hal_mcu2cp_local_irq_set(
    enum HAL_MCU2CP_ID_T id, enum HAL_MCU2CP_IRQ_TYPE_T type) {
  uint32_t value;

#ifdef CP_API
  if (id == HAL_MCU2CP_ID_0) {
    if (type == HAL_MCU2CP_IRQ_SEND_IND) {
      value = CMU_MCU2CP_DATA_IND_SET;
    } else {
      value = CMU_CP2MCU_DATA_DONE_SET;
    }
  } else {
    if (type == HAL_MCU2CP_IRQ_SEND_IND) {
      value = CMU_MCU2CP_DATA1_IND_SET;
    } else {
      value = CMU_CP2MCU_DATA1_DONE_SET;
    }
  }

  cmu->MCU2CP_IRQ_SET = value;
#else
  if (id == HAL_MCU2CP_ID_0) {
    if (type == HAL_MCU2CP_IRQ_SEND_IND) {
      value = CMU_CP2MCU_DATA_IND_SET;
    } else {
      value = CMU_MCU2CP_DATA_DONE_SET;
    }
  } else {
    if (type == HAL_MCU2CP_IRQ_SEND_IND) {
      value = CMU_CP2MCU_DATA1_IND_SET;
    } else {
      value = CMU_MCU2CP_DATA1_DONE_SET;
    }
  }

  cmu->CP2MCU_IRQ_SET = value;
#endif

  return 0;
}

static void MCU2CP_TEXT_LOC hal_mcu2cp_rx_irq(void) {
  int id;
  const struct HAL_MCU2CP_MSG_T *msg_ptr;
  enum HAL_MCU2CP_MSG_TYPE_T type;
  unsigned int processed;

  for (id = HAL_MCU2CP_ID_0; id < HAL_MCU2CP_ID_QTY; id++) {
    if (NVIC_GetActive(rx_irq_id[id])) {
      hal_mcu2cp_local_irq_clear(id, HAL_MCU2CP_IRQ_SEND_IND);

      if (recv_pending_head[id].data) {
        // Previous unprocessed message
        msg_ptr = &recv_pending_head[id];
      } else {
        // New message
        msg_ptr = recv_msg_list_p[id];
      }
      while (msg_ptr) {
        type = msg_ptr->type;
        if (type >= HAL_MCU2CP_MSG_TYPE_QTY) {
          // Error
          ASSERT(false, "MCU2CP-RX: Invalid msg type: %d", type);
          break;
        }
        if (rx_irq_handler[id][type]) {
          processed = rx_irq_handler[id][type](msg_ptr->data, msg_ptr->len);
          // Check if flow control needed
          if (processed < msg_ptr->len) {
            recv_pending_head[id].next = msg_ptr->next;
            recv_pending_head[id].type = msg_ptr->type;
            recv_pending_head[id].len = msg_ptr->len - processed;
            recv_pending_head[id].data = msg_ptr->data + processed;
            break;
          }
        } else {
          // Error
          ASSERT(false, "MCU2CP-RX: Handler missing");
          break;
        }
        msg_ptr = msg_ptr->next;
      }

      if (msg_ptr == NULL) {
        if (!need_flow_ctrl[id]) {
          hal_mcu2cp_peer_irq_set(id, HAL_MCU2CP_IRQ_RECV_DONE);
        }
        recv_pending_head[id].data = NULL;
      }
    }
  }
}

static void MCU2CP_TEXT_LOC hal_mcu2cp_tx_irq(void) {
  int id;
  struct HAL_MCU2CP_MSG_T *msg_ptr;
  enum HAL_MCU2CP_MSG_TYPE_T type;

  for (id = HAL_MCU2CP_ID_0; id < HAL_MCU2CP_ID_QTY; id++) {
    if (NVIC_GetActive(tx_irq_id[id])) {
      hal_mcu2cp_local_irq_clear(id, HAL_MCU2CP_IRQ_RECV_DONE);

      msg_ptr = send_msg_list_p[id];
      while (msg_ptr) {
        type = msg_ptr->type;
        if (type >= HAL_MCU2CP_MSG_TYPE_QTY) {
          // Error
          ASSERT(false, "MCU2CP-TX: Invalid msg type: %d", type);
          break;
        }
        if (tx_irq_handler[id][type]) {
          tx_irq_handler[id][type](msg_ptr->data, msg_ptr->len);
        };
        CONTAINER_OF(msg_ptr, struct HAL_MCU2CP_SEND_RECORD_T, msg)->in_use =
            false;
        msg_ptr = msg_ptr->next;
      }

      if (send_pending_list_p[id]) {
        send_msg_list_p[id] = send_pending_list_p[id];
        send_pending_list_p[id] = NULL;
        hal_mcu2cp_peer_irq_set(id, HAL_MCU2CP_IRQ_SEND_IND);
      } else {
        send_msg_list_p[id] = NULL;
        // Allow sleep
        hal_mcu2cp_busy(id, false);
      }
    }
  }
}

const struct HAL_MCU2CP_MSG_T **hal_mcu2cp_get_send_msg_list_mcu(void);
const struct HAL_MCU2CP_MSG_T **hal_mcu2cp_get_send_msg_list_cp(void);

#ifdef CP_API
// This is initialization code and should NOT be in CP text location
const struct HAL_MCU2CP_MSG_T **hal_mcu2cp_get_send_msg_list_cp(void)
#else
const struct HAL_MCU2CP_MSG_T **hal_mcu2cp_get_send_msg_list_mcu(void)
#endif
{
  return (const struct HAL_MCU2CP_MSG_T **)&send_msg_list_p[0];
}

int MCU2CP_API(hal_mcu2cp_open)(enum HAL_MCU2CP_ID_T id,
                                enum HAL_MCU2CP_MSG_TYPE_T type,
                                HAL_MCU2CP_RX_IRQ_HANDLER rxhandler,
                                HAL_MCU2CP_TX_IRQ_HANDLER txhandler,
                                bool rx_flowctrl) {
  int i;

  if (id >= HAL_MCU2CP_ID_QTY) {
    return 1;
  }
  if (type >= HAL_MCU2CP_MSG_TYPE_QTY) {
    return 2;
  }

  if (chan_opened[id] == 0) {
    hal_mcu2cp_local_irq_clear(id, HAL_MCU2CP_IRQ_SEND_IND);
    hal_mcu2cp_local_irq_clear(id, HAL_MCU2CP_IRQ_RECV_DONE);
#ifdef CP_API
    recv_msg_list_p = hal_mcu2cp_get_send_msg_list_mcu();
#else
    recv_msg_list_p = hal_mcu2cp_get_send_msg_list_cp();
#endif

    NVIC_SetVector(rx_irq_id[id], (uint32_t)hal_mcu2cp_rx_irq);
    NVIC_SetPriority(rx_irq_id[id], IRQ_PRIORITY_NORMAL);

    NVIC_SetVector(tx_irq_id[id], (uint32_t)hal_mcu2cp_tx_irq);
    NVIC_SetPriority(tx_irq_id[id], IRQ_PRIORITY_NORMAL);

    // Stop IRQs by default
    NVIC_DisableIRQ(rx_irq_id[id]);
    NVIC_DisableIRQ(tx_irq_id[id]);

    send_msg_list_p[id] = NULL;
    send_pending_list_p[id] = NULL;
    recv_pending_head[id].data = NULL;
    for (i = 0; i < MAX_SEND_RECORD_COUNT; i++) {
      send_msgs[id][i].in_use = false;
    }
    need_flow_ctrl[id] = rx_flowctrl;
  } else {
    ASSERT(need_flow_ctrl[id] == rx_flowctrl,
           "MCU2CP-OPEN: rx_flowctrl=%d (should be %d)", rx_flowctrl,
           need_flow_ctrl[id]);
    return 3;
  }
  chan_opened[id] |= (1 << type);

  rx_irq_handler[id][type] = rxhandler;
  tx_irq_handler[id][type] = txhandler;

  return 0;
}

int MCU2CP_API(hal_mcu2cp_close)(enum HAL_MCU2CP_ID_T id,
                                 enum HAL_MCU2CP_MSG_TYPE_T type) {
  if (id >= HAL_MCU2CP_ID_QTY) {
    return 1;
  }

  chan_opened[id] &= ~(1 << type);
  rx_irq_handler[id][type] = NULL;
  tx_irq_handler[id][type] = NULL;

  if (chan_opened[id] == 0) {
    // Stop IRQs by default
    NVIC_DisableIRQ(rx_irq_id[id]);
    NVIC_DisableIRQ(tx_irq_id[id]);

    send_msg_list_p[id] = NULL;
    send_pending_list_p[id] = NULL;
    recv_pending_head[id].data = NULL;
    need_flow_ctrl[id] = false;
  }

  return 0;
}

int MCU2CP_API(hal_mcu2cp_start_recv)(enum HAL_MCU2CP_ID_T id) {
  if (id >= HAL_MCU2CP_ID_QTY) {
    return 1;
  }

  NVIC_EnableIRQ(rx_irq_id[id]);
  // Check if there is any previous unprocessed message
  if (recv_pending_head[id].data) {
    hal_mcu2cp_local_irq_set(id, HAL_MCU2CP_IRQ_SEND_IND);
  }
  return 0;
}

int MCU2CP_API(hal_mcu2cp_stop_recv)(enum HAL_MCU2CP_ID_T id) {
  if (id >= HAL_MCU2CP_ID_QTY) {
    return 1;
  }

  NVIC_DisableIRQ(rx_irq_id[id]);
  return 0;
}

int MCU2CP_API(hal_mcu2cp_send)(enum HAL_MCU2CP_ID_T id,
                                enum HAL_MCU2CP_MSG_TYPE_T type,
                                const unsigned char *data, unsigned int len) {
  uint32_t lock;
  int ret;
  struct HAL_MCU2CP_SEND_RECORD_T *record;
  struct HAL_MCU2CP_MSG_T *next;
  int i;

  if (id >= HAL_MCU2CP_ID_QTY) {
    return 1;
  }
  if (type >= HAL_MCU2CP_MSG_TYPE_QTY) {
    return 2;
  }
  if ((chan_opened[id] & (1 << type)) == 0) {
    return 3;
  }

  NVIC_EnableIRQ(tx_irq_id[id]);

  ret = -1;
  record = &send_msgs[id][0];

  lock = int_lock();
  for (i = 0; i < MAX_SEND_RECORD_COUNT; i++) {
    if (record->in_use) {
      record++;
      continue;
    }
    record->in_use = true;
    record->msg.next = NULL;
    record->msg.type = type;
    record->msg.len = len;
    record->msg.data = data;
    if (send_msg_list_p[id] == NULL) {
      send_msg_list_p[id] = &record->msg;
      hal_mcu2cp_peer_irq_set(id, HAL_MCU2CP_IRQ_SEND_IND);
    } else if (send_pending_list_p[id] == NULL) {
      send_pending_list_p[id] = &record->msg;
    } else {
      next = send_pending_list_p[id];
      while (next->next) {
        next = next->next;
      }
      next->next = &record->msg;
    }
    ret = 0;
    // Prohibit sleep here
    hal_mcu2cp_busy(id, true);
    break;
  }
  int_unlock(lock);

  return ret;
}

void MCU2CP_API(hal_mcu2cp_rx_done)(enum HAL_MCU2CP_ID_T id) {
  hal_mcu2cp_peer_irq_set(id, HAL_MCU2CP_IRQ_RECV_DONE);
}

int MCU2CP_API(hal_mcu2cp_opened)(enum HAL_MCU2CP_ID_T id) {
  return !!chan_opened[id];
}

int MCU2CP_API(hal_mcu2cp_local_irq_pending)(enum HAL_MCU2CP_ID_T id) {
  uint32_t value;

#ifdef CP_API
  if (id == HAL_MCU2CP_ID_0) {
    value = CMU_MCU2CP_DATA_IND_SET | CMU_CP2MCU_DATA_DONE_SET;
  } else {
    value = CMU_MCU2CP_DATA1_IND_SET | CMU_CP2MCU_DATA1_DONE_SET;
  }

  return !!(cmu->MCU2CP_IRQ_SET & value);
#else
  if (id == HAL_MCU2CP_ID_0) {
    value = CMU_CP2MCU_DATA_IND_SET | CMU_MCU2CP_DATA_DONE_SET;
  } else {
    value = CMU_CP2MCU_DATA1_IND_SET | CMU_MCU2CP_DATA1_DONE_SET;
  }

  return !!(cmu->CP2MCU_IRQ_SET & value);
#endif
}

#endif

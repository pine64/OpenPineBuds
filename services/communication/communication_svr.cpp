#include "communication_svr.h"
#include "app_ai_if_config.h"
#include "cmsis_nvic.h"
#include "cmsis_os.h"
#include "communication_sysapi.h"
#include "crc32.h"
#include "hal_gpio.h"
#include "hal_iomux.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "hal_uart.h"
#include "plat_types.h"
#include "stdbool.h"
#include "stdint.h"
#include "stdio.h"
#include "string.h"

//======================================================================================================

enum COMMUNICATION_MSG {
  COMMUNICATION_MSG_TX_REQ = 0,
  COMMUNICATION_MSG_TX_DONE = 1,
  COMMUNICATION_MSG_RX_REQ = 2,
  COMMUNICATION_MSG_RX_DONE = 3,
  COMMUNICATION_MSG_INIT = 4,
  COMMUNICATION_MSG_REINIT = 5,
  COMMUNICATION_MSG_RESET = 6,
  COMMUNICATION_MSG_BREAK = 7,
};

const static uint8_t communication_process_log[8][26] = {
    "COMMUNICATION_MSG_TX_REQ", "COMMUNICATION_MSG_TX_DONE",
    "COMMUNICATION_MSG_RX_REQ", "COMMUNICATION_MSG_RX_DONE",
    "COMMUNICATION_MSG_INIT",   "COMMUNICATION_MSG_REINIT",
    "COMMUNICATION_MSG_RESET",  "COMMUNICATION_MSG_BREAK",
};

enum COMMUNICATION_MODE {
  COMMUNICATION_MODE_NULL,
  COMMUNICATION_MODE_TX,
  COMMUNICATION_MODE_RX,
  COMMUNICATION_MODE_ENABLE_IRQ,
  COMMUNICATION_MODE_DISABLE_IRQ,
};

#define COMMAND_BLOCK_MAX (5)
#define COMMAND_LEN_MAX (128)
#define COMMAND_TRANSMITTED_SIGNAL (1 << 0)

typedef struct {
  uint8_t cmd_buf[COMMAND_LEN_MAX];
  uint8_t cmd_len;
} COMMAND_BLOCK;
osPoolDef(command_mempool, COMMAND_BLOCK_MAX, COMMAND_BLOCK);
osPoolId command_mempool = NULL;

#define COMMUNICATION_MAILBOX_MAX (10)
typedef struct {
  uint32_t src_thread;
  uint32_t system_time;
  uint32_t message;
  uint32_t parms1;
  uint32_t parms2;
} COMMUNICATION_MAIL;

osMailQDef(communication_mailbox, COMMUNICATION_MAILBOX_MAX,
           COMMUNICATION_MAIL);
static osMailQId communication_mailbox = NULL;
static uint8_t communication_mailbox_cnt = 0;

static osThreadId communication_tid = NULL;
static void communication_thread(void const *argument);
osThreadDef(communication_thread, osPriorityHigh, 1, 2048,
            "communication_server");

static bool uart_opened = false;
static int uart_error_detected = 0;

#if defined(CHIP_BEST2300P) || defined(CHIP_BEST2300A)
static const enum HAL_UART_ID_T comm_uart = HAL_UART_ID_2;
#else
static const enum HAL_UART_ID_T comm_uart = HAL_UART_ID_1;
#endif

static const struct HAL_UART_CFG_T uart_cfg = {
    HAL_UART_PARITY_NONE,
    HAL_UART_STOP_BITS_1,
    HAL_UART_DATA_BITS_8,
    HAL_UART_FLOW_CONTROL_NONE,
    HAL_UART_FIFO_LEVEL_1_2,
    HAL_UART_FIFO_LEVEL_1_2,
#if defined(CHIP_BEST2300P) || defined(CHIP_BEST2300A)
    1152000,
#else
    921600,
#endif
    true,
    true,
    false,
};

static bool uart_rx_dma_is_running = false;

static COMMAND_BLOCK *rx_command_block_p = NULL;

static uint32_t uart_rx_idle_counter = 0;
static void uart_rx_idle_handler(void const *param);
osTimerDef(uart_rx_idle_timer, uart_rx_idle_handler);
static osTimerId uart_rx_idle_timer_id = NULL;

static communication_receive_func_typedef communication_receive_cb = NULL;

const static struct HAL_IOMUX_PIN_FUNCTION_MAP cfg_led2 = {
    HAL_IOMUX_PIN_LED2,
    HAL_IOMUX_FUNC_GPIO,
    HAL_IOMUX_PIN_VOLTAGE_VBAT,
    HAL_IOMUX_PIN_PULLUP_ENABLE,
};

inline int communication_mailbox_put(COMMUNICATION_MAIL *msg_src);
static void communication_process(COMMUNICATION_MAIL *mail_p);
int communication_io_mode_switch(enum COMMUNICATION_MODE mode);

static void uart_rx_dma_stop(void) {
  union HAL_UART_IRQ_T mask;

  uint32_t lock = int_lock();
  //    TRACE(1,"uart_rx_dma_stop:%d", uart_rx_dma_is_running);
  if (uart_rx_dma_is_running) {
    mask.reg = 0;
    hal_uart_irq_set_mask(comm_uart, mask);
    hal_uart_stop_dma_recv(comm_uart);
    uart_rx_dma_is_running = false;
  }

  int_unlock(lock);
}

static void uart_rx_dma_start(void) {
  union HAL_UART_IRQ_T mask;

  uint32_t lock = int_lock();

  //    TRACE(1,"uart_rx_dma_start:%d", uart_rx_dma_is_running);

  hal_uart_flush(comm_uart, 0);
  // hal_uart_dma_recv(comm_uart, rx_command_block_p->cmd_buf, COMMAND_LEN_MAX,
  // NULL, NULL);
  mask.reg = 0;
  mask.RT = 1;
  // hal_uart_irq_set_mask(comm_uart, mask);
  hal_uart_dma_recv_mask(comm_uart, rx_command_block_p->cmd_buf,
                         COMMAND_LEN_MAX, NULL, NULL, &mask);

  uart_rx_dma_is_running = true;

  int_unlock(lock);
}

static void uart_break_handler(void) {
  union HAL_UART_IRQ_T mask;
  COMMUNICATION_MAIL msg;
  memset(&msg, 0, sizeof(msg));

  TRACE(0, "UART-BREAK");
  mask.reg = 0;
  hal_uart_irq_set_mask(comm_uart, mask);
  hal_uart_flush(comm_uart, 0);
  uart_error_detected = 1;
  msg.message = COMMUNICATION_MSG_BREAK;
  communication_mailbox_put(&msg);
}

static void uart_rx_dma_handler(uint32_t xfer_size, int dma_error,
                                union HAL_UART_IRQ_T status) {
  COMMUNICATION_MAIL msg;

  // TRACE(8,"UART-RX size:%d dma_error=%d, status=0x%08x rt:%d fe:%d pe:%d
  // be:%d oe:%d", xfer_size, dma_error, status, status.RT, status.FE,
  // status.PE, status.BE, status.OE);

  if (status.BE) {
    uart_break_handler();
    return;
  }

  memset(&msg, 0, sizeof(COMMUNICATION_MAIL));
  msg.message = COMMUNICATION_MSG_RX_DONE;

  if (dma_error || status.FE || status.PE || status.BE || status.OE) {
    uart_error_detected = 1;
    uart_rx_dma_stop();
    msg.parms1 = false;
    msg.message = COMMUNICATION_MSG_RESET;
    communication_mailbox_put(&msg);
  } else {
    if (xfer_size) {
      msg.parms1 = true;
      msg.parms2 = xfer_size;
      communication_mailbox_put(&msg);
    } else {
      uart_rx_dma_stop();
      msg.message = COMMUNICATION_MSG_RESET;
      communication_mailbox_put(&msg);
    }
  }
}

static void uart_tx_dma_handler(uint32_t xfer_size, int dma_error) {
  COMMUNICATION_MAIL msg;

  memset(&msg, 0, sizeof(COMMUNICATION_MAIL));

  //    TRACE(2,"UART-TX size:%d dma_error=%d", xfer_size, dma_error);

  osSignalSet(communication_tid, COMMAND_TRANSMITTED_SIGNAL);

  msg.message = COMMUNICATION_MSG_TX_DONE;

  communication_mailbox_put(&msg);
}

static void uart_init(void) {
  struct HAL_UART_CFG_T comm_uart_cfg;

  if (!uart_opened) {
    memcpy(&comm_uart_cfg, &uart_cfg, sizeof(comm_uart_cfg));
    hal_uart_open(comm_uart, &comm_uart_cfg);
    hal_uart_irq_set_dma_handler(comm_uart, uart_rx_dma_handler,
                                 uart_tx_dma_handler);
    uart_opened = true;
  }

  hal_uart_flush(comm_uart, 0);
}

static void uart_deinit(void) {
  if (uart_opened) {
    hal_uart_close(comm_uart);
    uart_opened = false;
  }
}

static void uart_rx_idle_timer_start(void) {
  uart_rx_idle_counter = 0;
  TRACE(1, "[%s] enter...", __func__);

  if (uart_rx_idle_timer_id != NULL) {
    osTimerStop(uart_rx_idle_timer_id);
    osTimerStart(uart_rx_idle_timer_id, 100);
  }
}

#if defined(CHIP_BEST1400) || defined(CHIP_BEST1402)
static void uart_rx_edge_detect_handler(enum HAL_GPIO_PIN_T pin) {
  COMMUNICATION_MAIL msg = {0};

  // disable led2 pin external interrupt mode...
  communication_io_mode_switch(COMMUNICATION_MODE_DISABLE_IRQ);

  // post uart rx request...
  msg.message = COMMUNICATION_MSG_RX_REQ;
  communication_mailbox_put(&msg);
}
#endif

static void uart_rx_idle_handler(void const *param) {
  const uint8_t rx_idle_indicate[2] = {0x10, 0x24};

  if (uart_rx_idle_counter++ >= 150) // 150 * 100 = 15s
  {
    TRACE(1, "[%s] enter...", __func__);

    uart_rx_idle_counter = 0;

    // stop virtual timer...
    osTimerStop(uart_rx_idle_timer_id);

    // stop uart rx dma...
    uart_rx_dma_stop();

    // notify charge box uart rx dma has closed...
    communication_io_mode_switch(COMMUNICATION_MODE_TX);
    hal_uart_dma_send(comm_uart, rx_idle_indicate, 2, NULL, NULL);
    osDelay(10);

    // config led2 pin as external interrupt mode...
    communication_io_mode_switch(COMMUNICATION_MODE_ENABLE_IRQ);
  }
}

int communication_io_mode_switch(enum COMMUNICATION_MODE mode) {
  // best1400 and best1402 platform
  switch (mode) {
  case COMMUNICATION_MODE_TX: {
#if defined(CHIP_BEST1400) || defined(CHIP_BEST1402)
    hal_iomux_single_wire_uart1_tx();
#endif
  } break;
  case COMMUNICATION_MODE_RX: {
#if defined(CHIP_BEST1400) || defined(CHIP_BEST1402)
    hal_iomux_single_wire_uart1_rx();
#endif
  } break;
  case COMMUNICATION_MODE_ENABLE_IRQ: {
#if defined(CHIP_BEST1400) || defined(CHIP_BEST1402)
    hal_iomux_single_wire_uart1_enable_irq(uart_rx_edge_detect_handler);
#endif
  } break;
  case COMMUNICATION_MODE_DISABLE_IRQ: {
#if defined(CHIP_BEST1400) || defined(CHIP_BEST1402)
    hal_iomux_single_wire_uart1_disable_irq();
#endif
  } break;
  default:
    break;
  }
  return 0;
}

int communication_io_mode_init(void) {
  communication_io_mode_switch(COMMUNICATION_MODE_RX);

  return 0;
}

inline int communication_mailbox_put(COMMUNICATION_MAIL *msg_src) {
  osStatus status;

  COMMUNICATION_MAIL *mail_p = NULL;

  if (msg_src == NULL) {
    return -1;
  }

  mail_p = (COMMUNICATION_MAIL *)osMailAlloc(communication_mailbox, 0);
  if (!mail_p) {
    osEvent evt;
    TRACE(0, "communication_mailbox");
    for (uint8_t i = 0; i < COMMUNICATION_MAILBOX_MAX; i++) {
      evt = osMailGet(communication_mailbox, 0);
      if (evt.status == osEventMail) {
        TRACE(4, "msg cnt:%d msg:%d parms:%08x/%08x", i,
              ((COMMUNICATION_MAIL *)(evt.value.p))->message,
              ((COMMUNICATION_MAIL *)(evt.value.p))->parms1,
              ((COMMUNICATION_MAIL *)(evt.value.p))->parms2);
      } else {
        break;
      }
    }
    ASSERT(mail_p, "communication_mailbox error");
  }
  mail_p->src_thread = (uint32_t)osThreadGetId();
  mail_p->system_time = hal_sys_timer_get();
  mail_p->message = msg_src->message;
  mail_p->parms1 = msg_src->parms1;
  mail_p->parms2 = msg_src->parms2;
  status = osMailPut(communication_mailbox, mail_p);
  if (osOK == status) {
    communication_mailbox_cnt++;
  }

  return (int)status;
}

inline int communication_mailbox_free(COMMUNICATION_MAIL *mail_p) {
  osStatus status;

  status = osMailFree(communication_mailbox, mail_p);
  if (osOK == status) {
    communication_mailbox_cnt--;
  }
  return (int)status;
}

inline int communication_mailbox_get(COMMUNICATION_MAIL **mail_p) {
  osEvent evt;
  evt = osMailGet(communication_mailbox, osWaitForever);
  if (evt.status == osEventMail) {
    *mail_p = (COMMUNICATION_MAIL *)evt.value.p;
    return 0;
  }
  return -1;
}

static void communication_thread(void const *argument) {
  while (1) {
    COMMUNICATION_MAIL *mail_p = NULL;
    if (!communication_mailbox_get(&mail_p)) {
      communication_process(mail_p);
      communication_mailbox_free(mail_p);
    }
  }
}

int communication_command_block_alloc(COMMAND_BLOCK **cmd_blk) {
  *cmd_blk = (COMMAND_BLOCK *)osPoolCAlloc(command_mempool);
  ASSERT(*cmd_blk, "%s error", __func__);
  return 0;
}

void communication_command_block_free(COMMAND_BLOCK *cmd_blk) {
  osPoolFree(command_mempool, cmd_blk);
}

void communication_send_command(COMMAND_BLOCK *cmd_blk) {
  COMMUNICATION_MAIL mail;
  memset(&mail, 0, sizeof(mail));
  ASSERT(cmd_blk->cmd_len <= COMMAND_LEN_MAX, "%s len error", __func__);
  mail.message = COMMUNICATION_MSG_TX_REQ;
  mail.parms2 = (uint32_t)cmd_blk;
  communication_mailbox_put(&mail);
}

static void communication_process(COMMUNICATION_MAIL *mail_p) {
  osEvent evt;
  COMMAND_BLOCK *command_block_p;
  COMMUNICATION_MAIL msg;
  uint32_t lock;

  TRACE(2, "[%s]: %s", __func__, communication_process_log[mail_p->message]);

  memset(&msg, 0, sizeof(COMMUNICATION_MAIL));
  switch (mail_p->message) {
  case COMMUNICATION_MSG_TX_REQ:
    command_block_p = (COMMAND_BLOCK *)mail_p->parms2;
    TRACE(1, "UART TX:%d", command_block_p->cmd_len);
    DUMP8("%02x ", command_block_p->cmd_buf, command_block_p->cmd_len);
    uart_rx_dma_stop();
    communication_io_mode_switch(COMMUNICATION_MODE_TX);
    osSignalClear(communication_tid, COMMAND_TRANSMITTED_SIGNAL);
    hal_uart_dma_send(comm_uart, command_block_p->cmd_buf,
                      command_block_p->cmd_len, NULL, NULL);
    evt = osSignalWait(COMMAND_TRANSMITTED_SIGNAL, 1000);
    if (evt.status == osEventTimeout) {
      ASSERT(0, "%s osEventTimeout", __func__);
    }
    communication_command_block_free(command_block_p);
    while (!hal_uart_get_flag(comm_uart).TXFE ||
           hal_uart_get_flag(comm_uart).BUSY) {
      osThreadYield();
    }
    communication_io_mode_switch(COMMUNICATION_MODE_RX);
    if (!uart_error_detected) {
      uart_rx_dma_start();
      uart_rx_idle_timer_start();
    }
    break;
  case COMMUNICATION_MSG_TX_DONE:
    break;
  case COMMUNICATION_MSG_RX_REQ:
    communication_io_mode_init();
    uart_init();
    uart_rx_dma_start();
    uart_rx_idle_timer_start();
    break;
  case COMMUNICATION_MSG_RX_DONE:
    TRACE(2, "UART RX status:%d len:%d", mail_p->parms1, mail_p->parms2);
    DUMP8("%02x ", rx_command_block_p->cmd_buf, mail_p->parms2);
    TRACE(1, "%s", rx_command_block_p->cmd_buf);

    if (communication_receive_cb != NULL)
      (*communication_receive_cb)(rx_command_block_p->cmd_buf, mail_p->parms2);

    memset(rx_command_block_p->cmd_buf, 0, mail_p->parms2);

    if (!uart_error_detected) {
      uart_rx_dma_start();
      uart_rx_idle_timer_start();
    }
    break;
  case COMMUNICATION_MSG_REINIT:
    uart_deinit();
  case COMMUNICATION_MSG_INIT:
    uart_error_detected = 0;
    communication_io_mode_init();
    uart_init();
    uart_rx_dma_start();
    uart_rx_idle_timer_id =
        osTimerCreate(osTimer(uart_rx_idle_timer), osTimerPeriodic, NULL);
    uart_rx_idle_timer_start();
    break;
  case COMMUNICATION_MSG_RESET:
  case COMMUNICATION_MSG_BREAK:
    lock = int_lock();
    uart_rx_dma_stop();
    communication_io_mode_switch(COMMUNICATION_MODE_RX);
    hal_uart_flush(comm_uart, 0);
    uart_rx_dma_start();
    uart_error_detected = 0;
    int_unlock(lock);
    uart_rx_idle_timer_start();
    break;

  default:
    break;
  }
}

void communication_init(void) {
  COMMUNICATION_MAIL msg;
  COMMAND_BLOCK *cmd_blk;

  memset(&msg, 0, sizeof(COMMUNICATION_MAIL));
  TRACE(1, "%s", __func__);

  if (command_mempool == NULL) {
    command_mempool = osPoolCreate(osPool(command_mempool));
  }

  if (communication_mailbox == NULL) {
    communication_mailbox = osMailCreate(osMailQ(communication_mailbox), NULL);
  }

  if (communication_tid == NULL) {
    communication_tid = osThreadCreate(osThread(communication_thread), NULL);
  }

  if (rx_command_block_p == NULL) {
    communication_command_block_alloc(&rx_command_block_p);
    memset(rx_command_block_p->cmd_buf, 0, COMMAND_LEN_MAX);
    rx_command_block_p->cmd_len = 0;
  }

  msg.message = COMMUNICATION_MSG_INIT;
  communication_mailbox_put(&msg);

  communication_command_block_alloc(&cmd_blk);
  cmd_blk->cmd_buf[0] = 0xff;
  cmd_blk->cmd_len = 1;
  communication_send_command(cmd_blk);
}

int communication_receive_register_callback(
    communication_receive_func_typedef p) {
  if (p == NULL)
    return -1;

  communication_receive_cb = p;

  TRACE(1, "[%s] register receive callback success\n", __func__);

  return 0;
}

int communication_send_buf(uint8_t *buf, uint8_t len) {
  COMMAND_BLOCK *cmd_blk;

  communication_command_block_alloc(&cmd_blk);
  memcpy(cmd_blk->cmd_buf, buf, len);
  cmd_blk->cmd_len = len;
  communication_send_command(cmd_blk);
  return 0;
}

#ifdef KNOWLES_UART_DATA
#include "knowles_uart.h"

void uart_audio_init() {
  TRACE(1, "%s run!!!", __func__);
  init_transport();
}

void uart_audio_deinit() {
  deinit_transport();
  reopen_uart();
}

#endif

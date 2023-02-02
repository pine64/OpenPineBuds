#include "hal_trace.h"
#include "heap_api.h"

extern uint8_t __StackLimit[];
extern uint8_t __HeapLimit[];

uint8_t *syspool_addr = NULL;
uint32_t syspool_size = 0;

static uint32_t syspoll_used = 0;

static void syspool_init_addr(void) {
  syspool_addr = __HeapLimit;
  syspool_size = syspool_original_size();
}

uint32_t syspool_original_size(void) {
  return __StackLimit - __HeapLimit - 512;
}

void syspool_init() {
  syspool_init_addr();
  syspoll_used = 0;
  memset(syspool_addr, 0, syspool_size);
  TRACE(2, "syspool_init: %p,0x%x", syspool_addr, syspool_size);
}

void syspool_init_specific_size(uint32_t size) {
  syspool_init_addr();
  syspoll_used = 0;
  TRACE(2, "syspool_init_specific_size: %d/%d", size, syspool_size);
  if (size < syspool_size) {
    syspool_size = size;
  }
  memset(syspool_addr, 0, syspool_size);
  TRACE(2, "syspool_init_specific_size: %p,0x%x", syspool_addr, size);
}

uint8_t *syspool_start_addr(void) { return (uint8_t *)(&__HeapLimit); }

uint32_t syspool_total_size(void) { return syspool_size; }

int syspool_free_size() { return syspool_size - syspoll_used; }

int syspool_get_buff(uint8_t **buff, uint32_t size) {
  uint32_t buff_size_free;

  buff_size_free = syspool_free_size();

  if (size % 4) {
    size = size + (4 - size % 4);
  }
  TRACE(3, "[%s] size = %d , free size = %d", __func__, size, buff_size_free);
  ASSERT(size <= buff_size_free,
         "System pool in shortage! To allocate size %d but free size %d.", size,
         buff_size_free);
  *buff = syspool_addr + syspoll_used;
  syspoll_used += size;
  return buff_size_free;
}

int syspool_get_available(uint8_t **buff) {
  uint32_t buff_size_free;
  buff_size_free = syspool_free_size();

  TRACE(2, "[%s] free size = %d", __func__, buff_size_free);
  if (buff_size_free < 8)
    return -1;
  if (buff != NULL) {
    *buff = syspool_addr + syspoll_used;
    syspoll_used += buff_size_free;
  }
  return buff_size_free;
}

#if defined(A2DP_LDAC_ON)
int syspool_force_used_size(uint32_t size) { return syspoll_used = size; }
#endif

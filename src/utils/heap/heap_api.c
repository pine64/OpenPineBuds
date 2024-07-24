#include "heap_api.h"
#include "hal_trace.h"
#include "multi_heap.h"
#include "string.h"

// #define HEAP_API_DEBUG

#if 0
extern uint32_t __HeapBase;
extern uint32_t  __HeapLimit;
static heap_handle_t global_heap = NULL;

void *malloc(size_t size)
{
    if (global_heap == NULL)
        global_heap = heap_register((void *)&__HeapBase,(&__HeapLimit - &__HeapBase));
    return heap_malloc(global_heap,size);
}

void free(void *ptr)
{
    heap_free(global_heap,ptr);
}

void *calloc(size_t nmemb, size_t size)
{
    void *ptr = malloc(nmemb*size);
    if (ptr != NULL)
        memset(ptr,0,nmemb*size);
    return ptr;
}
void *realloc(void *ptr, size_t size)
{
    return heap_realloc(global_heap,ptr,size);
}
#endif

// TODO: multi heap and block
#define MED_HEAP_BLOCK_MAX_NUM (3)
static int g_block_index = 0;
static int g_switch_cp = 0;
// NOTE: Can use g_med_heap_begin_addr and g_med_heap_size to optimize free
// speed static void *g_med_heap_begin_addr[MED_HEAP_BLOCK_MAX_NUM]; static
// size_t g_med_heap_size[MED_HEAP_BLOCK_MAX_NUM];

static heap_handle_t g_med_heap[MED_HEAP_BLOCK_MAX_NUM];
static heap_handle_t g_cp_heap;

void heap_memory_info(heap_handle_t heap, size_t *total, size_t *used,
                      size_t *max_used) {
  multi_heap_info_t info;
  heap_get_info(heap, &info);

  if (total != NULL)
    *total = info.total_bytes;

  if (used != NULL)
    *used = info.total_allocated_bytes;

  if (max_used != NULL)
    *max_used = info.total_bytes - info.minimum_free_bytes;
}

static int med_help_get_index(void *ptr) {
  int index = 0;
  int diff_addr = 0;
  multi_heap_info_t info;

  for (index = 0; index < g_block_index; index++) {
    heap_get_info(g_med_heap[index], &info);
    diff_addr = (char *)ptr - (char *)g_med_heap[index];

#ifdef HEAP_API_DEBUG
    TRACE(3, "[%s] index = %d, diff_addr = %d", __func__, index, diff_addr);
#endif

    if ((diff_addr > 0) && (diff_addr < info.total_bytes)) {
      break;
    }
  }

  ASSERT(index < g_block_index, "[%s] Can not find ptr = %p", __func__, ptr);

  return index;
}

static void med_heap_reset(void) {
  g_block_index = 0;
  g_switch_cp = 0;

  for (int i = 0; i < MED_HEAP_BLOCK_MAX_NUM; i++) {
    // g_med_heap_begin_addr[i] = NULL;
    // g_med_heap_size[i] = NULL;
    g_med_heap[i] = NULL;
  }
  g_cp_heap = NULL;
}

void med_heap_set_cp(int switch_cp) {
  TRACE(2, "[%s] switch_cp: %d", __func__, switch_cp);
  g_switch_cp = switch_cp;
}

void med_heap_add_block(void *begin_addr, size_t size) {
  TRACE(4, "[%s] g_block_index = %d, begin_addr = %p, size = %d", __func__,
        g_block_index, begin_addr, size);
  ASSERT(g_block_index < MED_HEAP_BLOCK_MAX_NUM,
         "[%s] g_block_index(%d) >= MED_HEAP_BLOCK_MAX_NUM", __func__,
         g_block_index);

  memset(begin_addr, 0, size);
  if (g_switch_cp) {
    g_cp_heap = heap_register(begin_addr, size);
  } else {
    g_med_heap[g_block_index] = heap_register(begin_addr, size);
    // g_med_heap_begin_addr[g_block_index] = begin_addr;
    // g_med_heap_size[g_block_index] = size;

    g_block_index++;
  }
}

void med_heap_init(void *begin_addr, size_t size) {
  med_heap_reset();
  med_heap_add_block(begin_addr, size);
}

void *med_malloc(size_t size) {
  int index = 0;
  void *ptr = NULL;

  if (size == 0)
    return NULL;

  if (g_switch_cp) {
    ptr = heap_malloc(g_cp_heap, size);
    goto exit;
  }

  for (index = 0; index < g_block_index; index++) {
    if (multi_heap_free_size(g_med_heap[index]) >= size) {
      break;
    }
  }

#ifdef HEAP_API_DEBUG
  TRACE(3, "[%s] index = %d, size = %d", __func__, index, size);
#endif

  ASSERT(index < g_block_index,
         "[%s] index = %d, g_block_index = %d. Can not malloc any RAM",
         __func__, index, g_block_index);

  ptr = heap_malloc(g_med_heap[index], size);

exit:
  ASSERT(ptr != NULL, "[%s]: no memory, needed size %d", __FUNCTION__, size);

  return ptr;
}

void med_free(void *p) {
  if (p) {
    if (g_switch_cp) {
      heap_free(g_cp_heap, p);
    } else {
      int index = med_help_get_index(p);
      heap_free(g_med_heap[index], p);
    }

    p = NULL;
  }
}

void *med_calloc(size_t nmemb, size_t size) {
  if (size == 0)
    return NULL;

  void *ptr = med_malloc(nmemb * size);

  if (ptr) {
    memset(ptr, 0, nmemb * size);
  }

  return ptr;
}

void *med_realloc(void *ptr, size_t size) {
  // TODO: Do not support multi blocks
  // TODO: Do not support cp
  void *newptr = heap_realloc(g_med_heap[0], ptr, size);

  ASSERT(newptr != NULL, "[%s]: no memory, needed size %d", __FUNCTION__, size);

  return newptr;
}

void med_memory_info(size_t *total, size_t *used, size_t *max_used) {
  size_t _total = 0;
  size_t _used = 0;
  size_t _max_used = 0;

  for (int i = 0; i < g_block_index; i++) {
    heap_memory_info(g_med_heap[i], &_total, &_used, &_max_used);
    *total += _total;
    *used += _used;
    *max_used += _max_used;

#ifdef HEAP_API_DEBUG
    TRACE(4, "[%s] %d: g_med_heap = %p, size = %d", __func__, i, g_med_heap[i],
          _total);
#endif
  }
}

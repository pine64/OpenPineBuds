#ifndef AUDIO_MEMORY_H
#define AUDIO_MEMORY_H

#include "string.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef VQE_SIMULATE

#include "heap_api.h"

extern heap_handle_t g_audio_heap;

inline void audio_heap_init(void *begin_addr, size_t size)
{
    memset(begin_addr,0,size);
    g_audio_heap = heap_register(begin_addr,size);
}

inline void *audio_malloc(size_t size)
{
    return heap_malloc(g_audio_heap,size);
}

inline void audio_free(void *p)
{
    heap_free(g_audio_heap,p);
}

inline void *audio_calloc(size_t nmemb, size_t size)
{
    void *ptr = audio_malloc(nmemb*size);
    if (ptr != NULL)
        memset(ptr,0,nmemb*size);
    return ptr;
}

inline void *audio_realloc(void *ptr, size_t size)
{
    return heap_realloc(g_audio_heap,ptr,size);
}

inline void audio_memory_info(size_t *total,
                    size_t *used,
                    size_t *max_used)
{
    heap_memory_info(g_audio_heap,total,used,max_used);
}
#else
#include <stddef.h>
#include <stdlib.h>

void audio_heap_init(void *begin_addr, size_t size);
void *audio_malloc(size_t size);
void audio_free(void *p);
void *audio_calloc(size_t nmemb, size_t size);
void *audio_realloc(void *ptr, size_t size);
void audio_memory_info(size_t *total,
                    size_t *used,
                    size_t *max_used);
#endif

#ifdef __cplusplus
}
#endif

#endif
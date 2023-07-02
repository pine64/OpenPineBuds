#ifndef __INTEGER_RESAMPLING_H__
#define __INTEGER_RESAMPLING_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum  {
    INTEGER_RESAMPLING_ITEM_48K_TO_16K = 0,

    INTEGER_RESAMPLING_ITEM_NUM
} INTEGER_RESAMPLING_ITEM_T;

struct IntegerResamplingState_;

typedef struct IntegerResamplingState_ IntegerResamplingState;

// Creat a instance from integer_resampling module/class
IntegerResamplingState *integer_resampling_create(int32_t frame_size, int32_t channel_num, INTEGER_RESAMPLING_ITEM_T item, void *ext_alloc(int));

// Destory a integer_resampling instance
int32_t integer_resampling_destroy(IntegerResamplingState *st);

// Get/set some value or enable/disable some function
int32_t integer_resampling_ctl(IntegerResamplingState *st, int32_t ctl, void *ptr);

// Process speech stream
int32_t integer_resampling_process(IntegerResamplingState *st, int16_t *pcm_in, int32_t pcm_len, int16_t *pcm_out);

// Debug integer_resampling instance
int32_t integer_resampling_dump(IntegerResamplingState *st);

#ifdef __cplusplus
}
#endif

#endif

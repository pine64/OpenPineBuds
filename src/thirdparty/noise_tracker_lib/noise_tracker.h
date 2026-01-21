#ifndef NOISE_TRACKER_H
#define NOISE_TRACKER_H

#include <stdint.h>

typedef void (*NoiseTrackerCallback)(float);

#ifdef __cplusplus
extern "C" {
#endif

void noise_tracker_init(NoiseTrackerCallback cb, int ch_num, int threshold);

void noise_tracker_process(int16_t *buf, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif
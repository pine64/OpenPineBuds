#include "noise_tracker_callback.h"
#include "hal_trace.h"

/**
 * @brief Trigger word callback from kws lib
 *
 * @param word      Detected word
 * @param score     Score of word
 *
 */
void nt_demo_words_cb(float power)
{
    TRACE(1,"active power %d", (int)(power));
}
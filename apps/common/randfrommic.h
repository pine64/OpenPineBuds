#ifdef __RAND_FROM_MIC__
#ifndef __RANDFROMMIC_H__
#define __RANDFROMMIC_H__
#ifdef __cplusplus
extern "C" {
#endif

#define RANDOM_CAPTURE_BUFFER_SIZE 512

// random module state machine
typedef enum {
    RAND_STATUS_CLOSE = 0x00,       // initial state
    RAND_STATUS_OPEN = 0x01,        // indicate MIC has been started by random module
    RAND_STATUS_MIC_STARTED = 0x02, // indicate MIC has been started
    RAND_STATUS_MIC_OPENED = 0x03,  // indicate MIC has been opened but not start
    RAND_STATUS_NUM = 0x0F
}RAND_STATUS_E;

typedef struct{
    uint8_t skipRound; // used to indicate the number of rounds should be skipped to avoid all zero value
    RAND_STATUS_E status;
}__attribute__ ((__packed__))RAND_NUMBER_T;

void initSeed(void);
void random_status_sync(void);
void random_data_process(uint8_t *buf, uint32_t len,enum AUD_BITS_T bits, enum AUD_CHANNEL_NUM_T ch_num);
void randInit(void);

#ifdef __cplusplus
}
#endif
#endif
#endif

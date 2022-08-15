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
#ifdef CHIP_HAS_SDMMC

#include "errno.h"
#include "stdarg.h"
#include "stdio.h"
#include "string.h"
#include "plat_addr_map.h"
#include "reg_sdmmcip.h"
#include "hal_sdmmc.h"
#include "cmsis_nvic.h"
#include "hal_uart.h"
#include "hal_trace.h"
#include "hal_dma.h"
#include "hal_timer.h"
#include "hal_cmu.h"

#ifndef ETIMEDOUT
#define ETIMEDOUT   110 /* Connection timed out */
#endif

#define HAL_SDMMC_USE_DMA 1
//#define __BUS_WIDTH_SUPPORT_4BIT__ 1

#define HAL_SDMMC_TRACE(...)
//#define HAL_SDMMC_TRACE   TRACE
#define HAL_SDMMC_ASSERT(...)
//#define HAL_SDMMC_ASSERT  ASSERT

#define _SDMMC_CLOCK 52000000

#define _SDMMC_DMA_MINALIGN 32

#define _sdmmc_be32_to_cpu(x) \
    ((uint32_t)( \
            (((uint32_t)(x) & (uint32_t)0x000000ffUL) << 24) | \
            (((uint32_t)(x) & (uint32_t)0x0000ff00UL) <<  8) | \
            (((uint32_t)(x) & (uint32_t)0x00ff0000UL) >>  8) | \
            (((uint32_t)(x) & (uint32_t)0xff000000UL) >> 24) ))

#define _SDMMC_DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))
#define _SDMMC_ROUND(a,b)      (((a) + (b) - 1) & ~((b) - 1))

#define _SDMMC_PAD_COUNT(s, pad) (((s) - 1) / (pad) + 1)
#define _SDMMC_PAD_SIZE(s, pad) (_SDMMC_PAD_COUNT(s, pad) * pad)
#define _SDMMC_ALLOC_ALIGN_BUFFER_PAD(type, name, size, align, pad)        \
        char __##name[_SDMMC_ROUND(_SDMMC_PAD_SIZE((size) * sizeof(type), pad), align) + (align - 1)]; \
        type *name = (type *) ALIGN((uint32_t)__##name, align)

#define _SDMMC_ALLOC_ALIGN_BUFFER(type, name, size, align)     \
    _SDMMC_ALLOC_ALIGN_BUFFER_PAD(type, name, size, align, 1)
#define _SDMMC_ALLOC_CACHE_ALIGN_BUFFER_PAD(type, name, size, pad)     \
    _SDMMC_ALLOC_ALIGN_BUFFER_PAD(type, name, size, _SDMMC_DMA_MINALIGN, pad)
#define _SDMMC_ALLOC_CACHE_ALIGN_BUFFER(type, name, size) \
    _SDMMC_ALLOC_ALIGN_BUFFER(type, name, size, _SDMMC_DMA_MINALIGN)

typedef struct block_dev_desc {
    int     if_type;    /* type of the interface */
    int     dev;        /* device number */
    unsigned char   part_type;  /* partition type */
    unsigned char   target;     /* target SCSI ID */
    unsigned char   lun;        /* target LUN */
    unsigned char   type;       /* device type */
    unsigned char   removable;  /* removable device */
#ifdef CONFIG_LBA48
    unsigned char   lba48;      /* device can use 48bit addr (ATA/ATAPI v7) */
#endif
    uint32_t    lba;        /* number of blocks */
    unsigned long   blksz;      /* block size */
    int     log2blksz;  /* for convenience: log2(blksz) */
    char        vendor [40+1];  /* IDE model, SCSI Vendor */
    char        product[20+1];  /* IDE Serial no, SCSI product */
    char        revision[8+1];  /* firmware revision */
    void        *priv;      /* driver private struct pointer */
} block_dev_desc_t;

/* SD/MMC version bits; 8 flags, 8 major, 8 minor, 8 change */
#define SD_VERSION_SD   (1U << 31)
#define MMC_VERSION_MMC (1U << 30)

#define MAKE_SDMMC_VERSION(a, b, c) \
    ((((uint32_t)(a)) << 16) | ((uint32_t)(b) << 8) | (uint32_t)(c))
#define MAKE_SD_VERSION(a, b, c)    \
    (SD_VERSION_SD | MAKE_SDMMC_VERSION(a, b, c))
#define MAKE_MMC_VERSION(a, b, c)   \
    (MMC_VERSION_MMC | MAKE_SDMMC_VERSION(a, b, c))

#define EXTRACT_SDMMC_MAJOR_VERSION(x)  \
    (((uint32_t)(x) >> 16) & 0xff)
#define EXTRACT_SDMMC_MINOR_VERSION(x)  \
    (((uint32_t)(x) >> 8) & 0xff)
#define EXTRACT_SDMMC_CHANGE_VERSION(x) \
    ((uint32_t)(x) & 0xff)

#define SD_VERSION_3        MAKE_SD_VERSION(3, 0, 0)
#define SD_VERSION_2        MAKE_SD_VERSION(2, 0, 0)
#define SD_VERSION_1_0      MAKE_SD_VERSION(1, 0, 0)
#define SD_VERSION_1_10     MAKE_SD_VERSION(1, 10, 0)

#define MMC_VERSION_UNKNOWN MAKE_MMC_VERSION(0, 0, 0)
#define MMC_VERSION_1_2     MAKE_MMC_VERSION(1, 2, 0)
#define MMC_VERSION_1_4     MAKE_MMC_VERSION(1, 4, 0)
#define MMC_VERSION_2_2     MAKE_MMC_VERSION(2, 2, 0)
#define MMC_VERSION_3       MAKE_MMC_VERSION(3, 0, 0)
#define MMC_VERSION_4       MAKE_MMC_VERSION(4, 0, 0)
#define MMC_VERSION_4_1     MAKE_MMC_VERSION(4, 1, 0)
#define MMC_VERSION_4_2     MAKE_MMC_VERSION(4, 2, 0)
#define MMC_VERSION_4_3     MAKE_MMC_VERSION(4, 3, 0)
#define MMC_VERSION_4_41    MAKE_MMC_VERSION(4, 4, 1)
#define MMC_VERSION_4_5     MAKE_MMC_VERSION(4, 5, 0)
#define MMC_VERSION_5_0     MAKE_MMC_VERSION(5, 0, 0)

#define MMC_MODE_HS     (1 << 0)
#define MMC_MODE_HS_52MHz   (1 << 1)
#define MMC_MODE_4BIT       (1 << 2)
#define MMC_MODE_8BIT       (1 << 3)
#define MMC_MODE_SPI        (1 << 4)
#define MMC_MODE_DDR_52MHz  (1 << 5)

#define SD_DATA_4BIT    0x00040000

#define IS_SD(x)    ((x)->version & SD_VERSION_SD)
#define IS_MMC(x)   ((x)->version & MMC_VERSION_MMC)

#define MMC_DATA_READ       1
#define MMC_DATA_WRITE      2

#define NO_CARD_ERR     -16 /* No SD/MMC card inserted */
#define UNUSABLE_ERR        -17 /* Unusable Card */
#define COMM_ERR        -18 /* Communications Error */
#define TIMEOUT         -19
#define SWITCH_ERR      -20 /* Card reports failure to switch mode */

#define MMC_CMD_GO_IDLE_STATE       0
#define MMC_CMD_SEND_OP_COND        1
#define MMC_CMD_ALL_SEND_CID        2
#define MMC_CMD_SET_RELATIVE_ADDR   3
#define MMC_CMD_SET_DSR         4
#define MMC_CMD_SWITCH          6
#define MMC_CMD_SELECT_CARD     7
#define MMC_CMD_SEND_EXT_CSD        8
#define MMC_CMD_SEND_CSD        9
#define MMC_CMD_SEND_CID        10
#define MMC_CMD_STOP_TRANSMISSION   12
#define MMC_CMD_SEND_STATUS     13
#define MMC_CMD_SET_BLOCKLEN        16
#define MMC_CMD_READ_SINGLE_BLOCK   17
#define MMC_CMD_READ_MULTIPLE_BLOCK 18
#define MMC_CMD_SET_BLOCK_COUNT         23
#define MMC_CMD_WRITE_SINGLE_BLOCK  24
#define MMC_CMD_WRITE_MULTIPLE_BLOCK    25
#define MMC_CMD_ERASE_GROUP_START   35
#define MMC_CMD_ERASE_GROUP_END     36
#define MMC_CMD_ERASE           38
#define MMC_CMD_APP_CMD         55
#define MMC_CMD_SPI_READ_OCR        58
#define MMC_CMD_SPI_CRC_ON_OFF      59
#define MMC_CMD_RES_MAN         62

#define MMC_CMD62_ARG1          0xefac62ec
#define MMC_CMD62_ARG2          0xcbaea7


#define SD_CMD_SEND_RELATIVE_ADDR   3
#define SD_CMD_SWITCH_FUNC      6
#define SD_CMD_SEND_IF_COND     8
#define SD_CMD_SWITCH_UHS18V        11

#define SD_CMD_APP_SET_BUS_WIDTH    6
#define SD_CMD_ERASE_WR_BLK_START   32
#define SD_CMD_ERASE_WR_BLK_END     33
#define SD_CMD_APP_SEND_OP_COND     41
#define SD_CMD_APP_SEND_SCR     51

/* SCR definitions in different words */
#define SD_HIGHSPEED_BUSY   0x00020000
#define SD_HIGHSPEED_SUPPORTED  0x00020000

#define OCR_BUSY        0x80000000
#define OCR_HCS         0x40000000
#define OCR_VOLTAGE_MASK    0x007FFF80
#define OCR_ACCESS_MODE     0x60000000

#define SECURE_ERASE        0x80000000

#define MMC_STATUS_MASK     (~0x0206BF7F)
#define MMC_STATUS_SWITCH_ERROR (1 << 7)
#define MMC_STATUS_RDY_FOR_DATA (1 << 8)
#define MMC_STATUS_CURR_STATE   (0xf << 9)
#define MMC_STATUS_ERROR    (1 << 19)

#define MMC_STATE_PRG       (7 << 9)

#define MMC_VDD_165_195     0x00000080  /* VDD voltage 1.65 - 1.95 */
#define MMC_VDD_20_21       0x00000100  /* VDD voltage 2.0 ~ 2.1 */
#define MMC_VDD_21_22       0x00000200  /* VDD voltage 2.1 ~ 2.2 */
#define MMC_VDD_22_23       0x00000400  /* VDD voltage 2.2 ~ 2.3 */
#define MMC_VDD_23_24       0x00000800  /* VDD voltage 2.3 ~ 2.4 */
#define MMC_VDD_24_25       0x00001000  /* VDD voltage 2.4 ~ 2.5 */
#define MMC_VDD_25_26       0x00002000  /* VDD voltage 2.5 ~ 2.6 */
#define MMC_VDD_26_27       0x00004000  /* VDD voltage 2.6 ~ 2.7 */
#define MMC_VDD_27_28       0x00008000  /* VDD voltage 2.7 ~ 2.8 */
#define MMC_VDD_28_29       0x00010000  /* VDD voltage 2.8 ~ 2.9 */
#define MMC_VDD_29_30       0x00020000  /* VDD voltage 2.9 ~ 3.0 */
#define MMC_VDD_30_31       0x00040000  /* VDD voltage 3.0 ~ 3.1 */
#define MMC_VDD_31_32       0x00080000  /* VDD voltage 3.1 ~ 3.2 */
#define MMC_VDD_32_33       0x00100000  /* VDD voltage 3.2 ~ 3.3 */
#define MMC_VDD_33_34       0x00200000  /* VDD voltage 3.3 ~ 3.4 */
#define MMC_VDD_34_35       0x00400000  /* VDD voltage 3.4 ~ 3.5 */
#define MMC_VDD_35_36       0x00800000  /* VDD voltage 3.5 ~ 3.6 */

#define MMC_SWITCH_MODE_CMD_SET     0x00 /* Change the command set */
#define MMC_SWITCH_MODE_SET_BITS    0x01 /* Set bits in EXT_CSD byte
                        addressed by index which are
                        1 in value field */
#define MMC_SWITCH_MODE_CLEAR_BITS  0x02 /* Clear bits in EXT_CSD byte
                        addressed by index, which are
                        1 in value field */
#define MMC_SWITCH_MODE_WRITE_BYTE  0x03 /* Set target byte to value */

#define SD_SWITCH_CHECK     0
#define SD_SWITCH_SWITCH    1

/*
 * EXT_CSD fields
 */
#define EXT_CSD_ENH_START_ADDR      136 /* R/W */
#define EXT_CSD_ENH_SIZE_MULT       140 /* R/W */
#define EXT_CSD_GP_SIZE_MULT        143 /* R/W */
#define EXT_CSD_PARTITION_SETTING   155 /* R/W */
#define EXT_CSD_PARTITIONS_ATTRIBUTE    156 /* R/W */
#define EXT_CSD_MAX_ENH_SIZE_MULT   157 /* R */
#define EXT_CSD_PARTITIONING_SUPPORT    160 /* RO */
#define EXT_CSD_RST_N_FUNCTION      162 /* R/W */
#define EXT_CSD_WR_REL_PARAM        166 /* R */
#define EXT_CSD_WR_REL_SET      167 /* R/W */
#define EXT_CSD_RPMB_MULT       168 /* RO */
#define EXT_CSD_ERASE_GROUP_DEF     175 /* R/W */
#define EXT_CSD_BOOT_BUS_WIDTH      177
#define EXT_CSD_PART_CONF       179 /* R/W */
#define EXT_CSD_BUS_WIDTH       183 /* R/W */
#define EXT_CSD_HS_TIMING       185 /* R/W */
#define EXT_CSD_REV         192 /* RO */
#define EXT_CSD_CARD_TYPE       196 /* RO */
#define EXT_CSD_SEC_CNT         212 /* RO, 4 bytes */
#define EXT_CSD_HC_WP_GRP_SIZE      221 /* RO */
#define EXT_CSD_HC_ERASE_GRP_SIZE   224 /* RO */
#define EXT_CSD_BOOT_MULT       226 /* RO */

/*
 * EXT_CSD field definitions
 */

#define EXT_CSD_CMD_SET_NORMAL      (1 << 0)
#define EXT_CSD_CMD_SET_SECURE      (1 << 1)
#define EXT_CSD_CMD_SET_CPSECURE    (1 << 2)

#define EXT_CSD_CARD_TYPE_26    (1 << 0)    /* Card can run at 26MHz */
#define EXT_CSD_CARD_TYPE_52    (1 << 1)    /* Card can run at 52MHz */
#define EXT_CSD_CARD_TYPE_DDR_1_8V  (1 << 2)
#define EXT_CSD_CARD_TYPE_DDR_1_2V  (1 << 3)
#define EXT_CSD_CARD_TYPE_DDR_52    (EXT_CSD_CARD_TYPE_DDR_1_8V \
                    | EXT_CSD_CARD_TYPE_DDR_1_2V)

#define EXT_CSD_BUS_WIDTH_1 0   /* Card is in 1 bit mode */
#define EXT_CSD_BUS_WIDTH_4 1   /* Card is in 4 bit mode */
#define EXT_CSD_BUS_WIDTH_8 2   /* Card is in 8 bit mode */
#define EXT_CSD_DDR_BUS_WIDTH_4 5   /* Card is in 4 bit DDR mode */
#define EXT_CSD_DDR_BUS_WIDTH_8 6   /* Card is in 8 bit DDR mode */

#define EXT_CSD_BOOT_ACK_ENABLE         (1 << 6)
#define EXT_CSD_BOOT_PARTITION_ENABLE       (1 << 3)
#define EXT_CSD_PARTITION_ACCESS_ENABLE     (1 << 0)
#define EXT_CSD_PARTITION_ACCESS_DISABLE    (0 << 0)

#define EXT_CSD_BOOT_ACK(x)     (x << 6)
#define EXT_CSD_BOOT_PART_NUM(x)    (x << 3)
#define EXT_CSD_PARTITION_ACCESS(x) (x << 0)

#define EXT_CSD_BOOT_BUS_WIDTH_MODE(x)  (x << 3)
#define EXT_CSD_BOOT_BUS_WIDTH_RESET(x) (x << 2)
#define EXT_CSD_BOOT_BUS_WIDTH_WIDTH(x) (x)

#define EXT_CSD_PARTITION_SETTING_COMPLETED (1 << 0)

#define EXT_CSD_ENH_USR     (1 << 0)    /* user data area is enhanced */
#define EXT_CSD_ENH_GP(x)   (1 << ((x)+1))  /* GP part (x+1) is enhanced */

#define EXT_CSD_HS_CTRL_REL (1 << 0)    /* host controlled WR_REL_SET */

#define EXT_CSD_WR_DATA_REL_USR     (1 << 0)    /* user data area WR_REL */
#define EXT_CSD_WR_DATA_REL_GP(x)   (1 << ((x)+1))  /* GP part (x+1) WR_REL */

#define R1_ILLEGAL_COMMAND      (1 << 22)
#define R1_APP_CMD          (1 << 5)

#define MMC_RSP_PRESENT (1 << 0)
#define MMC_RSP_136 (1 << 1)        /* 136 bit response */
#define MMC_RSP_CRC (1 << 2)        /* expect valid crc */
#define MMC_RSP_BUSY    (1 << 3)        /* card may send busy */
#define MMC_RSP_OPCODE  (1 << 4)        /* response contains opcode */

#define MMC_RSP_NONE    (0)
#define MMC_RSP_R1  (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R1b (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE| \
            MMC_RSP_BUSY)
#define MMC_RSP_R2  (MMC_RSP_PRESENT|MMC_RSP_136|MMC_RSP_CRC)
#define MMC_RSP_R3  (MMC_RSP_PRESENT)
#define MMC_RSP_R4  (MMC_RSP_PRESENT)
#define MMC_RSP_R5  (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R6  (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R7  (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)

#define MMCPART_NOAVAILABLE (0xff)
#define PART_ACCESS_MASK    (0x7)
#define PART_SUPPORT        (0x1)
#define ENHNCD_SUPPORT      (0x2)
#define PART_ENH_ATTRIB     (0x1f)

/* Maximum block size for MMC */
#define MMC_MAX_BLOCK_LEN   512

/* The number of MMC physical partitions.  These consist of:
 * boot partitions (2), general purpose partitions (4) in MMC v4.4.
 */
#define MMC_NUM_BOOT_PARTITION  2
#define MMC_PART_RPMB           3       /* RPMB partition number */

struct mmc_cid {
    unsigned long psn;
    unsigned short oid;
    uint8_t mid;
    uint8_t prv;
    uint8_t mdt;
    char pnm[7];
};

struct mmc_cmd {
    u16 cmdidx;
    uint32_t resp_type;
    uint32_t cmdarg;
    uint32_t response[4];
};

struct mmc_data {
    union {
        char *dest;
        const char *src; /* src buffers don't get written to */
    };
    uint32_t flags;
    uint32_t blocks;
    uint32_t blocksize;
};

/* forward decl. */
struct mmc;

struct mmc_ops {
    int (*send_cmd)(struct mmc *mmc,
            struct mmc_cmd *cmd, struct mmc_data *data);
    void (*set_ios)(struct mmc *mmc);
    int (*init)(struct mmc *mmc);
    int (*getcd)(struct mmc *mmc);
    int (*getwp)(struct mmc *mmc);
};

struct mmc_config {
    const struct mmc_ops *ops;
    uint32_t host_caps;
    uint32_t voltages;
    uint32_t f_min;
    uint32_t f_max;
    uint32_t b_max;
    uint8_t part_type;
};

/* TODO struct mmc should be in mmc_private but it's hard to fix right now */
struct mmc {
    const struct mmc_config *cfg;   /* provided configuration */
    uint32_t version;
    void *priv;
    uint32_t has_init;
    int high_capacity;
    uint32_t bus_width;
    uint32_t clock;
    uint32_t card_caps;
    uint32_t ocr;
    uint32_t dsr;
    uint32_t dsr_imp;
    uint32_t scr[2];
    uint32_t csd[4];
    uint32_t cid[4];
    u16 rca;
    u8 part_support;
    u8 part_attr;
    u8 wr_rel_set;
    char part_config;
    char part_num;
    uint32_t tran_speed;
    uint32_t read_bl_len;
    uint32_t write_bl_len;
    uint32_t erase_grp_size;    /* in 512-byte sectors */
    uint32_t hc_wp_grp_size;    /* in 512-byte sectors */
    uint64_t capacity;
    uint64_t capacity_user;
    uint64_t capacity_boot;
    uint64_t capacity_rpmb;
    uint64_t capacity_gp[4];
    uint64_t enh_user_start;
    uint64_t enh_user_size;
    block_dev_desc_t block_dev;
    char op_cond_pending;   /* 1 if we are waiting on an op_cond command */
    char init_in_progress;  /* 1 if we have done mmc_start_init() */
    char preinit;       /* start init as early as possible */
    int ddr_mode;
};

struct mmc_hwpart_conf {
    struct {
        uint32_t enh_start; /* in 512-byte sectors */
        uint32_t enh_size;  /* in 512-byte sectors, if 0 no enh area */
        unsigned wr_rel_change : 1;
        unsigned wr_rel_set : 1;
    } user;
    struct {
        uint32_t size;  /* in 512-byte sectors */
        unsigned enhanced : 1;
        unsigned wr_rel_change : 1;
        unsigned wr_rel_set : 1;
    } gp_part[4];
};

enum mmc_hwpart_conf_mode {
    MMC_HWPART_CONF_CHECK,
    MMC_HWPART_CONF_SET,
    MMC_HWPART_CONF_COMPLETE,
};

struct sdmmcip_host {
    void *ioaddr;
    unsigned int quirks;
    unsigned int caps;
    unsigned int version;
    unsigned int clock;
    unsigned int bus_hz;
    unsigned int div;
    int dev_index;
    int dev_id;
    int buswidth;
    uint32_t fifoth_val;
	bool detect_enb;
    struct mmc *mmc;
    void *priv;

    void (*clksel)(struct sdmmcip_host *host);
    void (*board_init)(struct sdmmcip_host *host);
    unsigned int (*get_mmc_clk)(struct sdmmcip_host *host);

    struct mmc_config cfg;

#ifdef HAL_SDMMC_USE_DMA
    /* TODO : os and non-os condition */
    uint8_t dma_ch;
    volatile uint32_t sdmmc_dma_lock;
    HAL_DMA_IRQ_HANDLER_T tx_dma_handler;
    HAL_DMA_IRQ_HANDLER_T rx_dma_handler;
#endif
};

struct sdmmcip_idmac {
    uint32_t flags;
    uint32_t cnt;
    uint32_t addr;
    uint32_t next_addr;
};

static void hal_sdmmc_delay(uint32_t ms);

static HAL_SDMMC_DELAY_FUNC sdmmc_delay = NULL;

static inline void sdmmcip_writel(struct sdmmcip_host *host, int reg, uint32_t val)
{
    *((volatile uint32_t *)(host->ioaddr + reg)) = val;
}

static inline uint32_t sdmmcip_readl(struct sdmmcip_host *host, int reg)
{
    return *((volatile uint32_t *)(host->ioaddr + reg));
}

#ifdef CONFIG_MMC_SPI
#define sdmmc_host_is_spi(mmc)    ((mmc)->cfg->host_caps & MMC_MODE_SPI)
#else
#define sdmmc_host_is_spi(mmc)    0
#endif

#ifndef CONFIG_SYS_MMC_MAX_BLK_COUNT
#define CONFIG_SYS_MMC_MAX_BLK_COUNT 65535
#endif

static void (*sdmmc_detected_callback)(uint8_t) = NULL;
static uint32_t sdmmc_ip_base[HAL_SDMMC_ID_NUM] = {SDMMC_BASE, };
static struct sdmmcip_host sdmmc_host[HAL_SDMMC_ID_NUM];
static struct mmc sdmmc_devices[HAL_SDMMC_ID_NUM];
//static const char * const invalid_id = "Invalid SDMMC ID: %d\n";

#ifdef HAL_SDMMC_USE_DMA
static void sdmmcip0_ext_dma_tx_handler(uint8_t chan, uint32_t remain_tsize, uint32_t error, struct HAL_DMA_DESC_T *lli);
static void sdmmcip0_ext_dma_rx_handler(uint8_t chan, uint32_t remain_tsize, uint32_t error, struct HAL_DMA_DESC_T *lli);
static HAL_DMA_IRQ_HANDLER_T sdmmcip_ext_dma_irq_handlers[HAL_SDMMC_ID_NUM*2] = {
    sdmmcip0_ext_dma_tx_handler, sdmmcip0_ext_dma_rx_handler,
};
#endif

uint32_t _sdmmc_div64_32(uint64_t *n, uint32_t base)
{
    uint64_t rem = *n;
    uint64_t b = base;
    uint64_t res, d = 1;
    uint32_t high = rem >> 32;

    /* Reduce the thing a bit first */
    res = 0;
    if (high >= base) {
        high /= base;
        res = (uint64_t) high << 32;
        rem -= (uint64_t) (high*base) << 32;
    }

    while ((int64_t)b > 0 && b < rem) {
        b = b+b;
        d = d+d;
    }

    do {
        if (rem >= b) {
            rem -= b;
            res += d;
        }
        b >>= 1;
        d >>= 1;
    } while (d);

    *n = res;
    return rem;
}


/* The unnecessary pointer compare is there
 *  * to check for type safety (n must be 64bit)
 *   */
# define _sdmmc_do_div(n,base) ({              \
            uint32_t __base = (base);           \
            uint32_t __rem;                 \
            (void)(((typeof((n)) *)0) == ((uint64_t *)0));  \
            if (((n) >> 32) == 0) {         \
                __rem = (uint32_t)(n) % __base;     \
                (n) = (uint32_t)(n) / __base;       \
            } else                      \
                __rem = _sdmmc_div64_32(&(n), __base);   \
            __rem;                      \
         })

/* Wrapper for _sdmmc_do_div(). Doesn't modify dividend and returns
 *  * the result, not reminder.
 *   */
static inline uint64_t _sdmmc_lldiv(uint64_t dividend, uint32_t divisor)
{
    uint64_t __res = dividend;
    _sdmmc_do_div(__res, divisor);
    return(__res);
}


static void mmc_udelay(int cnt)
{
    volatile uint32_t i = 0, c = 0;
    for(i = 0; i < cnt; ++i) {
        c++;
        __asm("nop");
    }
}


static int sdmmcip_wait_reset(struct sdmmcip_host *host, uint32_t value)
{
    uint32_t ctrl;
    unsigned long timeout = 1000;

    sdmmcip_writel(host, SDMMCIP_REG_CTRL, value);

    while (timeout--) {
        ctrl = sdmmcip_readl(host, SDMMCIP_REG_CTRL);
        if (!(ctrl & SDMMCIP_REG_RESET_ALL))
            return 1;
    }
    return 0;
}

#ifdef HAL_SDMMC_USE_DMA
static void sdmmcip0_ext_dma_tx_handler(uint8_t chan, uint32_t remain_tsize, uint32_t error, struct HAL_DMA_DESC_T *lli)
{
    uint32_t ip_raw_int_status = 0;
    struct sdmmcip_host *host = &sdmmc_host[HAL_SDMMC_ID_0];
    HAL_SDMMC_TRACE(2,"%s:%d\n", __func__, __LINE__);
    ip_raw_int_status = sdmmcip_readl(host, SDMMCIP_REG_RINTSTS);
    HAL_SDMMC_TRACE(3,"%s:%d, tx ip_raw_int_status 0x%x\n", __func__, __LINE__, ip_raw_int_status);
    if (ip_raw_int_status & (SDMMCIP_REG_DATA_ERR|SDMMCIP_REG_DATA_TOUT)) {
        HAL_SDMMC_TRACE(3,"%s:%d, sdmmcip0 tx dma error 0x%x\n", __func__, __LINE__, ip_raw_int_status);
    }

    /* TODO : os and non-os condition */
    host->sdmmc_dma_lock = 0;
}

static void sdmmcip0_ext_dma_rx_handler(uint8_t chan, uint32_t remain_tsize, uint32_t error, struct HAL_DMA_DESC_T *lli)
{
    uint32_t ip_raw_int_status = 0;
    struct sdmmcip_host *host = &sdmmc_host[HAL_SDMMC_ID_0];
    HAL_SDMMC_TRACE(2,"%s:%d\n", __func__, __LINE__);
    ip_raw_int_status = sdmmcip_readl(host, SDMMCIP_REG_RINTSTS);
    HAL_SDMMC_TRACE(3,"%s:%d, ip_raw_int_status 0x%x\n", __func__, __LINE__, ip_raw_int_status);
    if (ip_raw_int_status & (SDMMCIP_REG_DATA_ERR|SDMMCIP_REG_DATA_TOUT)) {
        HAL_SDMMC_TRACE(3,"%s:%d, sdmmcip0 rx dma error 0x%x\n", __func__, __LINE__, ip_raw_int_status);
    }

    /* TODO : os and non-os condition */
    host->sdmmc_dma_lock = 0;
}
#endif

static void sdmmcip_prepare_data(struct sdmmcip_host *host,
                   struct mmc_data *data,
                   struct sdmmcip_idmac *cur_idmac,
                   void *bounce_buffer)
{
#ifdef HAL_SDMMC_USE_DMA
    uint32_t ctrl = 0;
    struct HAL_DMA_CH_CFG_T dma_cfg;
#endif
    sdmmcip_writel(host, SDMMCIP_REG_BLKSIZ, data->blocksize);
    sdmmcip_writel(host, SDMMCIP_REG_BYTCNT, data->blocksize * data->blocks);


    /* use dma */
#ifdef HAL_SDMMC_USE_DMA

    HAL_SDMMC_TRACE(0,"sdmmc use dma\n");
    /* enable sdmmcip e-dma */
    ctrl = sdmmcip_readl(host, SDMMCIP_REG_CTRL);
    ctrl |= (SDMMCIP_REG_DMA_EN);
    sdmmcip_writel(host, SDMMCIP_REG_CTRL, ctrl);

    host->sdmmc_dma_lock = 1;

    memset(&dma_cfg, 0, sizeof(dma_cfg));
    if (data->flags & MMC_DATA_READ) {
        HAL_SDMMC_TRACE(0,"sdmmc use dma read\n");
        dma_cfg.dst = (uint32_t)data->dest;
        //HAL_SDMMC_TRACE(2,"sddma :len %d, buf 0x%x\n", data->blocksize*data->blocks, dma_cfg.dst);
        if ((dma_cfg.dst % 4) != 0) {
            dma_cfg.dst_width = HAL_DMA_WIDTH_BYTE;
        }
        else {
            dma_cfg.dst_width = HAL_DMA_WIDTH_WORD;
        }
        dma_cfg.dst_bsize = HAL_DMA_BSIZE_1;
        dma_cfg.dst_periph = 0;  // useless
        //dma_cfg.dst_width = HAL_DMA_WIDTH_WORD;
        dma_cfg.handler = host->rx_dma_handler;
        dma_cfg.src_bsize = HAL_DMA_BSIZE_1;
        dma_cfg.src_periph = HAL_GPDMA_SDMMC;
        dma_cfg.src_tsize = data->blocks*data->blocksize/4;
        HAL_SDMMC_TRACE(1,"sdmmc use dma tsize %d\n", dma_cfg.src_tsize);
        dma_cfg.src_width = HAL_DMA_WIDTH_WORD;
        dma_cfg.try_burst = 1;
        dma_cfg.type = HAL_DMA_FLOW_P2M_DMA;
        dma_cfg.src = (uint32_t)0; // useless
        dma_cfg.ch = hal_gpdma_get_chan(dma_cfg.src_periph, HAL_DMA_HIGH_PRIO);
        HAL_SDMMC_TRACE(1,"sdmmc use dma get ch %d\n", dma_cfg.ch);
    }
    else {
        HAL_SDMMC_TRACE(0,"sdmmc use dma write\n");
        dma_cfg.dst = 0; // useless
        dma_cfg.dst_bsize = HAL_DMA_BSIZE_1;
        dma_cfg.dst_periph = HAL_GPDMA_SDMMC;
        dma_cfg.dst_width = HAL_DMA_WIDTH_WORD;
        dma_cfg.handler = host->tx_dma_handler;
        dma_cfg.src_bsize = HAL_DMA_BSIZE_1;
        dma_cfg.src_periph = 0; // useless
        //dma_cfg.src_tsize = data->blocks*data->blocksize/4;
        HAL_SDMMC_TRACE(1,"sdmmc use dma tsize %d\n", dma_cfg.src_tsize);
        //dma_cfg.src_width = HAL_DMA_WIDTH_WORD;
        dma_cfg.try_burst = 1;
        dma_cfg.type = HAL_DMA_FLOW_M2P_DMA;
        dma_cfg.src = (uint32_t)data->src;
        //uart_printf("sddma :len %d, buf 0x%x\n", data->blocksize*data->blocks, dma_cfg.src);
        if ((dma_cfg.src % 4) != 0) {
            dma_cfg.src_width = HAL_DMA_WIDTH_BYTE;
            dma_cfg.src_tsize = data->blocks*data->blocksize;
        }
        else {
            dma_cfg.src_width = HAL_DMA_WIDTH_WORD;
            dma_cfg.src_tsize = data->blocks*data->blocksize/4;
        }
        dma_cfg.ch = hal_gpdma_get_chan(dma_cfg.dst_periph, HAL_DMA_HIGH_PRIO);
        HAL_SDMMC_TRACE(1,"sdmmc use dma get ch %d\n", dma_cfg.ch);

        //uart_printf("%s:%d src 0x%x\n", __func__, __LINE__, dma_cfg.src);
    }

    host->dma_ch = dma_cfg.ch;

    hal_gpdma_start(&dma_cfg);
#endif
}

static int sdmmcip_set_transfer_mode(struct sdmmcip_host *host,
        struct mmc_data *data)
{
    unsigned long mode;

    mode = SDMMCIP_REG_CMD_DATA_EXP;
    if (data->flags & MMC_DATA_WRITE)
        mode |= SDMMCIP_REG_CMD_RW;

    return mode;
}

static int sdmmcip_send_cmd(struct mmc *mmc, struct mmc_cmd *cmd,
        struct mmc_data *data)
{
    int ret = 0;
    int flags = 0, i;
    uint32_t retry = 1000000;
    uint32_t mask, ctrl;
    int busy_tmo = MS_TO_TICKS(1000), busy_t = 0;
#ifndef HAL_SDMMC_USE_DMA
    uint32_t status = 0, fifo_data = 0;
#endif
    struct sdmmcip_host *host = mmc->priv;

    busy_t = hal_sys_timer_get();

    while ((sdmmcip_readl(host, SDMMCIP_REG_STATUS) & SDMMCIP_REG_BUSY) && hal_sys_timer_get()<(busy_t+busy_tmo)) {
        HAL_SDMMC_TRACE(0,"[sdmmc]busy\n");
    }

    sdmmcip_writel(host, SDMMCIP_REG_RINTSTS, SDMMCIP_REG_INTMSK_CD);

    if (data) {
        sdmmcip_prepare_data(host, data, 0, 0);
    }

    sdmmcip_writel(host, SDMMCIP_REG_CMDARG, cmd->cmdarg);

    if (data)
        flags = sdmmcip_set_transfer_mode(host, data);

    if ((cmd->resp_type & MMC_RSP_136) && (cmd->resp_type & MMC_RSP_BUSY))
        return -1;

    if (cmd->cmdidx == MMC_CMD_STOP_TRANSMISSION)
        flags |= SDMMCIP_REG_CMD_ABORT_STOP;
    else
        flags |= SDMMCIP_REG_CMD_PRV_DAT_WAIT;

    if (cmd->resp_type & MMC_RSP_PRESENT) {
        flags |= SDMMCIP_REG_CMD_RESP_EXP;
        if (cmd->resp_type & MMC_RSP_136)
            flags |= SDMMCIP_REG_CMD_RESP_LENGTH;
    }

    if (cmd->resp_type & MMC_RSP_CRC)
        flags |= SDMMCIP_REG_CMD_CHECK_CRC;

    flags |= (cmd->cmdidx | SDMMCIP_REG_CMD_START | SDMMCIP_REG_CMD_USE_HOLD_REG);

    sdmmcip_writel(host, SDMMCIP_REG_CMD, flags);

    for (i = 0; i < retry; i++) {
        mask = sdmmcip_readl(host, SDMMCIP_REG_RINTSTS);
        if (mask & SDMMCIP_REG_INTMSK_CDONE) {
            if (!data)
                sdmmcip_writel(host, SDMMCIP_REG_RINTSTS, SDMMCIP_REG_INTMSK_CDONE);
            break;
        }
    }

    if (i == retry) {
        HAL_SDMMC_TRACE(1,"%s: Timeout.\n", __func__);
        return TIMEOUT;
    }

    if (mask & SDMMCIP_REG_INTMSK_RTO) {
        /*
         * Timeout here is not necessarily fatal. (e)MMC cards
         * will splat here when they receive CMD55 as they do
         * not support this command and that is exactly the way
         * to tell them apart from SD cards. Thus, this output
         * below shall be TRACE(). eMMC cards also do not favor
         * CMD8, please keep that in mind.
         */
        HAL_SDMMC_TRACE(1,"%s: Response Timeout.\n", __func__);
        return TIMEOUT;
    } else if (mask & SDMMCIP_REG_INTMSK_RE) {
        HAL_SDMMC_TRACE(1,"%s: Response Error.\n", __func__);
        return -1;
    }


    if (cmd->resp_type & MMC_RSP_PRESENT) {
        if (cmd->resp_type & MMC_RSP_136) {
            cmd->response[0] = sdmmcip_readl(host, SDMMCIP_REG_RESP3);
            cmd->response[1] = sdmmcip_readl(host, SDMMCIP_REG_RESP2);
            cmd->response[2] = sdmmcip_readl(host, SDMMCIP_REG_RESP1);
            cmd->response[3] = sdmmcip_readl(host, SDMMCIP_REG_RESP0);
        } else {
            cmd->response[0] = sdmmcip_readl(host, SDMMCIP_REG_RESP0);
        }
    }

#ifndef HAL_SDMMC_USE_DMA
    if (data) {
        i = 0;
        while(1) {
            mask = sdmmcip_readl(host, SDMMCIP_REG_RINTSTS);
            if (mask & (SDMMCIP_REG_DATA_ERR | SDMMCIP_REG_DATA_TOUT)) {
                HAL_SDMMC_TRACE(1,"%s: READ DATA ERROR!\n", __func__);
                ret = -1;
                goto out;
            }
            status = sdmmcip_readl(host, SDMMCIP_REG_STATUS);
            if (data->flags == MMC_DATA_READ) {
                if (status & SDMMCIP_REG_FIFO_COUNT_MASK) {
                    fifo_data = sdmmcip_readl(host, SDMMCIP_REG_FIFO_OFFSET);
                    //HAL_SDMMC_TRACE(3,"%s: count %d, read -> 0x%x\n", __func__, i, fifo_data);
                    /* FIXME: now we just deal with 32bit width fifo one time */
                    if (i < data->blocks*data->blocksize) {
                        memcpy(data->dest + i, &fifo_data, sizeof(fifo_data));
                        i += sizeof(fifo_data);
                    }
                    else {
                        HAL_SDMMC_TRACE(1,"%s: fifo data too much\n", __func__);
                        ret = -1;
                        goto out;
                    }
                }
                /* nothing to read from fifo and DTO is set */
                else if(mask & SDMMCIP_REG_INTMSK_DTO) {
                    if(i != data->blocks*data->blocksize) {
                        HAL_SDMMC_TRACE(3,"%s: need to read %d, actually read %d\n", __func__, data->blocks*data->blocksize, i);
                    }
                    ret = 0;
                    goto out;
                }
            }
            else {
                /* nothing to write to fifo and DTO is set */
                if(mask & SDMMCIP_REG_INTMSK_DTO) {
                    /* check if number is right */
                    if(i != data->blocks*data->blocksize) {
                        HAL_SDMMC_TRACE(3,"%s: need to write %d, actually written %d\n", __func__, data->blocks*data->blocksize, i);
                    }
                    ret = 0;
                    goto out;
                }
                else if (!(status & SDMMCIP_REG_FIFO_COUNT_MASK)) {
                    /* FIXME: now we just deal with 32bit width fifo one time */
                    if(i < data->blocks*data->blocksize) {
                        memcpy(&fifo_data, data->src + i, sizeof(fifo_data));
                        //HAL_SDMMC_TRACE(4,"%s: fifo %d, count %d, write -> 0x%x\n", __func__, ((status & SDMMCIP_REG_FIFO_COUNT_MASK)>>SDMMCIP_REG_FIFO_COUNT_SHIFT), i, fifo_data);
                        i += sizeof(fifo_data);
                        sdmmcip_writel(host, SDMMCIP_REG_FIFO_OFFSET, fifo_data);
                    }
                    else {
                        HAL_SDMMC_TRACE(1,"%s: no data to write to fifo, do nothing\n", __func__);
                    }
                }
            }
        }
    }
#endif

#ifdef HAL_SDMMC_USE_DMA
    while (host->sdmmc_dma_lock);
    ctrl = sdmmcip_readl(host, SDMMCIP_REG_CTRL);
    ctrl |= SDMMCIP_REG_CTRL_DMA_RESET;
    sdmmcip_wait_reset(host, ctrl);
	if (data) {
    	hal_gpdma_free_chan(host->dma_ch);
#if 0
        if (data->flags & MMC_DATA_READ) {
            for (int ccc = 0; ccc < 400; ++ccc) {
                HAL_SDMMC_TRACE(2,"%d-0x%x ", ccc, data->dest[ccc]);
            }
            HAL_SDMMC_TRACE(0,"\n");
        }
#endif
    }
#endif

#ifndef HAL_SDMMC_USE_DMA
out:
#endif

    mask = sdmmcip_readl(host, SDMMCIP_REG_RINTSTS);
    sdmmcip_writel(host, SDMMCIP_REG_RINTSTS, mask&(~SDMMCIP_REG_INTMSK_CD));

    ctrl = sdmmcip_readl(host, SDMMCIP_REG_CTRL);
    ctrl &= ~(SDMMCIP_REG_DMA_EN);
    sdmmcip_writel(host, SDMMCIP_REG_CTRL, ctrl);

    mmc_udelay(100);

    return ret;
}

static int sdmmcip_setup_bus(struct sdmmcip_host *host, uint32_t freq)
{
    uint32_t div, status;
    int timeout = 10000;
    unsigned long sclk;

    if ((freq == host->clock) || (freq == 0))
        return 0;
    /*
     * If host->get_mmc_clk isn't defined,
     * then assume that host->bus_hz is source clock value.
     * host->bus_hz should be set by user.
     */
    if (host->get_mmc_clk)
        sclk = host->get_mmc_clk(host);
    else if (host->bus_hz)
        sclk = host->bus_hz;
    else {
        HAL_SDMMC_TRACE(1,"%s: Didn't get source clock value.\n", __func__);
        return -EINVAL;
    }

    if (sclk == freq)
        div = 0;    /* bypass mode */
    else
        div = _SDMMC_DIV_ROUND_UP(sclk, 2 * freq);

    HAL_SDMMC_TRACE(4,"%s : freq %d, sclk %ld, div %d\n", __func__, freq, sclk, div);

    sdmmcip_writel(host, SDMMCIP_REG_CLKENA, 0);
    //sdmmcip_writel(host, SDMMCIP_REG_CLKSRC, 0);
    //sdmmcip_writel(host, SDMMCIP_REG_CLKSRC, 0x03000000);
    //sdmmcip_writel(host, SDMMCIP_REG_CLKSRC, 0x11000000);

    sdmmcip_writel(host, SDMMCIP_REG_CLKDIV, div);
    sdmmcip_writel(host, SDMMCIP_REG_CMD, SDMMCIP_REG_CMD_PRV_DAT_WAIT |
            SDMMCIP_REG_CMD_UPD_CLK | SDMMCIP_REG_CMD_START);

    do {
        status = sdmmcip_readl(host, SDMMCIP_REG_CMD);
        if (timeout-- < 0) {
            HAL_SDMMC_TRACE(1,"%s: Timeout!\n", __func__);
            return -ETIMEDOUT;
        }
    } while (status & SDMMCIP_REG_CMD_START);

    sdmmcip_writel(host, SDMMCIP_REG_CLKENA, SDMMCIP_REG_CLKEN_ENABLE |
            SDMMCIP_REG_CLKEN_LOW_PWR);
    //sdmmcip_writel(host, SDMMCIP_REG_CLKENA, SDMMCIP_REG_CLKEN_ENABLE);

    sdmmcip_writel(host, SDMMCIP_REG_CMD, SDMMCIP_REG_CMD_PRV_DAT_WAIT |
            SDMMCIP_REG_CMD_UPD_CLK | SDMMCIP_REG_CMD_START);

    timeout = 10000;
    do {
        status = sdmmcip_readl(host, SDMMCIP_REG_CMD);
        if (timeout-- < 0) {
            HAL_SDMMC_TRACE(1,"%s: Timeout!\n", __func__);
            return -ETIMEDOUT;
        }
    } while (status & SDMMCIP_REG_CMD_START);

    host->clock = freq;

    return 0;
}

static void sdmmcip_set_ios(struct mmc *mmc)
{
    struct sdmmcip_host *host = (struct sdmmcip_host *)mmc->priv;
    uint32_t ctype, regs;

    HAL_SDMMC_TRACE(2,"Buswidth = %d, clock: %d\n", mmc->bus_width, mmc->clock);

    sdmmcip_setup_bus(host, mmc->clock);

    switch (mmc->bus_width) {
        case 8:
            ctype = SDMMCIP_REG_CTYPE_8BIT;
            break;
        case 4:
            ctype = SDMMCIP_REG_CTYPE_4BIT;
            break;
        default:
            ctype = SDMMCIP_REG_CTYPE_1BIT;
            break;
    }

    sdmmcip_writel(host, SDMMCIP_REG_CTYPE, ctype);

    regs = sdmmcip_readl(host, SDMMCIP_REG_UHS_REG);
    if (mmc->ddr_mode)
        regs |= SDMMCIP_REG_DDR_MODE;
    else
        regs &= ~SDMMCIP_REG_DDR_MODE;

    sdmmcip_writel(host, SDMMCIP_REG_UHS_REG, regs);

    if (host->clksel)
        host->clksel(host);
}

static int sdmmcip_init(struct mmc *mmc)
{
    struct sdmmcip_host *host = mmc->priv;

    if (host->board_init)
        host->board_init(host);

    HAL_SDMMC_TRACE(2,"host->ioaddr %x, SDMMCIP_REG_PWREN %x\n", host->ioaddr, SDMMCIP_REG_PWREN);
    sdmmcip_writel(host, SDMMCIP_REG_PWREN, 1);

    if (!sdmmcip_wait_reset(host, SDMMCIP_REG_RESET_ALL)) {
        HAL_SDMMC_TRACE(2,"%s[%d] Fail-reset!!\n", __func__, __LINE__);
        return -1;
    }

    /* Enumerate at 400KHz */
    sdmmcip_setup_bus(host, mmc->cfg->f_min);

	if (host->detect_enb){
		sdmmcip_writel(host, SDMMCIP_REG_RINTSTS, SDMMCIP_REG_INTMSK_ALL&(~SDMMCIP_REG_INTMSK_CD));
		sdmmcip_writel(host, SDMMCIP_REG_INTMASK, SDMMCIP_REG_INTMSK_CD);
		sdmmcip_writel(host, SDMMCIP_REG_CTRL, SDMMCIP_REG_INT_EN);
	}else{
		sdmmcip_writel(host, SDMMCIP_REG_RINTSTS, SDMMCIP_REG_INTMSK_ALL);
    	sdmmcip_writel(host, SDMMCIP_REG_INTMASK, ~SDMMCIP_REG_INTMSK_ALL);
	}

    sdmmcip_writel(host, SDMMCIP_REG_TMOUT, 0xFFFFFFFF);

    sdmmcip_writel(host, SDMMCIP_REG_IDINTEN, 0);
    sdmmcip_writel(host, SDMMCIP_REG_BMOD, 1);

    sdmmcip_writel(host, SDMMCIP_REG_FIFOTH, host->fifoth_val);

    sdmmcip_writel(host, SDMMCIP_REG_CLKENA, 0);
    //sdmmcip_writel(host, SDMMCIP_REG_CLKSRC, 0);
    //sdmmcip_writel(host, SDMMCIP_REG_CLKSRC, 0x03000000);
    //sdmmcip_writel(host, SDMMCIP_REG_CLKSRC, 0x11000000);

    sdmmcip_writel(host, SDMMCIP_REG_RESET_CARD, 0);
    mmc_udelay(1000);
    sdmmcip_writel(host, SDMMCIP_REG_RESET_CARD, 1);
    mmc_udelay(1000);
    mmc_udelay(1000);
    sdmmcip_writel(host, SDMMCIP_REG_RESET_CARD, 0);
    //sdmmcip_writel(host, SDMMCIP_REG_RESET_CARD, 1);

    return 0;
}

static const struct mmc_ops sdmmcip_ops = {
    .send_cmd   = sdmmcip_send_cmd,
    .set_ios    = sdmmcip_set_ios,
    .init       = sdmmcip_init,
};

int board_mmc_getcd(struct mmc *mmc)
{
    return -1;
}

int mmc_send_cmd(struct mmc *mmc, struct mmc_cmd *cmd, struct mmc_data *data)
{
    int ret;

    int i;
    uint8_t *ptr;

    HAL_SDMMC_TRACE(1,"CMD_SEND:%d\n", cmd->cmdidx);
    HAL_SDMMC_TRACE(1,"\t\tARG\t\t\t 0x%08X\n", cmd->cmdarg);
    ret = mmc->cfg->ops->send_cmd(mmc, cmd, data);
    HAL_SDMMC_TRACE(0,"send cmd end...");
    switch (cmd->resp_type) {
        case MMC_RSP_NONE:
            HAL_SDMMC_TRACE(0,"\t\tMMC_RSP_NONE\n");
            break;
        case MMC_RSP_R1:
            HAL_SDMMC_TRACE(1,"\t\tMMC_RSP_R1,5,6,7 \t 0x%08X \n",
                    cmd->response[0]);
            break;
        case MMC_RSP_R1b:
            HAL_SDMMC_TRACE(1,"\t\tMMC_RSP_R1b\t\t 0x%08X \n",
                    cmd->response[0]);
            break;
        case MMC_RSP_R2:
            HAL_SDMMC_TRACE(1,"\t\tMMC_RSP_R2\t\t 0x%08X \n",
                    cmd->response[0]);
            HAL_SDMMC_TRACE(1,"\t\t          \t\t 0x%08X \n",
                    cmd->response[1]);
            HAL_SDMMC_TRACE(1,"\t\t          \t\t 0x%08X \n",
                    cmd->response[2]);
            HAL_SDMMC_TRACE(1,"\t\t          \t\t 0x%08X \n",
                    cmd->response[3]);
            HAL_SDMMC_TRACE(0,"\n");
            HAL_SDMMC_TRACE(0,"\t\t\t\t\tDUMPING DATA\n");
            for (i = 0; i < 4; i++) {
                int j;
                HAL_SDMMC_TRACE(1,"\t\t\t\t\t%03d - ", i*4);
                ptr = (uint8_t *)&cmd->response[i];
                ptr += 3;
                for (j = 0; j < 4; j++)
                    HAL_SDMMC_TRACE(1,"%02X ", *ptr--);
                HAL_SDMMC_TRACE(0,"\n");
            }
            break;
        case MMC_RSP_R3:
            HAL_SDMMC_TRACE(1,"\t\tMMC_RSP_R3,4\t\t 0x%08X \n",
                    cmd->response[0]);
            break;
        default:
            HAL_SDMMC_TRACE(0,"\t\tERROR MMC rsp not supported\n");
            break;
    }
    return ret;
}

int mmc_send_status(struct mmc *mmc, int timeout)
{
    struct mmc_cmd cmd;
    int err, retries = 5;
    int POSSIBLY_UNUSED status;

    cmd.cmdidx = MMC_CMD_SEND_STATUS;
    cmd.resp_type = MMC_RSP_R1;
    if (!sdmmc_host_is_spi(mmc))
        cmd.cmdarg = mmc->rca << 16;

    while (1) {
        err = mmc_send_cmd(mmc, &cmd, NULL);
        if (!err) {
            if ((cmd.response[0] & MMC_STATUS_RDY_FOR_DATA) &&
                    (cmd.response[0] & MMC_STATUS_CURR_STATE) !=
                    MMC_STATE_PRG)
                break;
            else if (cmd.response[0] & MMC_STATUS_MASK) {
                return COMM_ERR;
            }
        } else if (--retries < 0)
            return err;

        if (timeout-- <= 0)
            break;

        mmc_udelay(1000);
    }

    status = (cmd.response[0] & MMC_STATUS_CURR_STATE) >> 9;
    HAL_SDMMC_TRACE(1,"CURR STATE:%d\n", status);
    if (timeout <= 0) {
        return TIMEOUT;
    }
    if (cmd.response[0] & MMC_STATUS_SWITCH_ERROR)
        return SWITCH_ERR;

    return 0;
}

int mmc_set_blocklen(struct mmc *mmc, int len)
{
    struct mmc_cmd cmd;

    if (mmc->ddr_mode)
        return 0;

    cmd.cmdidx = MMC_CMD_SET_BLOCKLEN;
    cmd.resp_type = MMC_RSP_R1;
    cmd.cmdarg = len;

    return mmc_send_cmd(mmc, &cmd, NULL);
}

struct mmc *find_mmc_device(int dev_num)
{
    return &sdmmc_devices[dev_num];
}

static int mmc_read_blocks(struct mmc *mmc, void *dst, uint32_t start,
        uint32_t blkcnt)
{
    struct mmc_cmd cmd;
    struct mmc_data data;

    if (blkcnt > 1)
        cmd.cmdidx = MMC_CMD_READ_MULTIPLE_BLOCK;
    else
        cmd.cmdidx = MMC_CMD_READ_SINGLE_BLOCK;

    if (mmc->high_capacity)
        cmd.cmdarg = start;
    else
        cmd.cmdarg = start * mmc->read_bl_len;

    cmd.resp_type = MMC_RSP_R1;

    data.dest = dst;
    data.blocks = blkcnt;
    data.blocksize = mmc->read_bl_len;
    data.flags = MMC_DATA_READ;

    if (mmc_send_cmd(mmc, &cmd, &data))
        return 0;

    if (blkcnt > 1) {
        cmd.cmdidx = MMC_CMD_STOP_TRANSMISSION;
        cmd.cmdarg = 0;
        cmd.resp_type = MMC_RSP_R1b;
        if (mmc_send_cmd(mmc, &cmd, NULL)) {
            return 0;
        }
    }

    return blkcnt;
}

static unsigned long mmc_bread(int dev_num, uint32_t start, uint32_t blkcnt, void *dst)
{
    uint32_t cur, blocks_todo = blkcnt;

    if (blkcnt == 0)
        return 0;

    struct mmc *mmc = find_mmc_device(dev_num);
    if (!mmc)
        return 0;

    if ((start + blkcnt) > mmc->block_dev.lba) {
        return 0;
    }

    if (mmc_set_blocklen(mmc, mmc->read_bl_len))
        return 0;

    do {
        cur = (blocks_todo > mmc->cfg->b_max) ?
            mmc->cfg->b_max : blocks_todo;
        if(mmc_read_blocks(mmc, dst, start, cur) != cur)
            return 0;
        blocks_todo -= cur;
        start += cur;
        dst += cur * mmc->read_bl_len;
    } while (blocks_todo > 0);

    return blkcnt;
}

static unsigned long mmc_erase_t(struct mmc *mmc, unsigned long start, uint32_t blkcnt)
{
    struct mmc_cmd cmd;
    unsigned long end;
    int err, start_cmd, end_cmd;

    if (mmc->high_capacity) {
        end = start + blkcnt - 1;
    } else {
        end = (start + blkcnt - 1) * mmc->write_bl_len;
        start *= mmc->write_bl_len;
    }

    if (IS_SD(mmc)) {
        start_cmd = SD_CMD_ERASE_WR_BLK_START;
        end_cmd = SD_CMD_ERASE_WR_BLK_END;
    } else {
        start_cmd = MMC_CMD_ERASE_GROUP_START;
        end_cmd = MMC_CMD_ERASE_GROUP_END;
    }

    cmd.cmdidx = start_cmd;
    cmd.cmdarg = start;
    cmd.resp_type = MMC_RSP_R1;

    err = mmc_send_cmd(mmc, &cmd, NULL);
    if (err)
        goto err_out;

    cmd.cmdidx = end_cmd;
    cmd.cmdarg = end;

    err = mmc_send_cmd(mmc, &cmd, NULL);
    if (err)
        goto err_out;

    cmd.cmdidx = MMC_CMD_ERASE;
    cmd.cmdarg = SECURE_ERASE;
    cmd.resp_type = MMC_RSP_R1b;

    err = mmc_send_cmd(mmc, &cmd, NULL);
    if (err)
        goto err_out;

    return 0;

err_out:
    //puts("mmc erase failed\n");
    return err;
}

unsigned long mmc_berase(int dev_num, uint32_t start, uint32_t blkcnt)
{
    int err = 0;
    struct mmc *mmc = find_mmc_device(dev_num);
    uint32_t blk = 0, blk_r = 0;
    int timeout = 1000;

    if (!mmc)
        return -1;

    if ((start % mmc->erase_grp_size) || (blkcnt % mmc->erase_grp_size))
        //printf("\n\nCaution! Your devices Erase group is 0x%x\n"
        //       "The erase range would be change to "
        //       "0x" LBAF "~0x" LBAF "\n\n",
        //       mmc->erase_grp_size, start & ~(mmc->erase_grp_size - 1),
        //       ((start + blkcnt + mmc->erase_grp_size)
        //      & ~(mmc->erase_grp_size - 1)) - 1);

        while (blk < blkcnt) {
            blk_r = ((blkcnt - blk) > mmc->erase_grp_size) ?
                mmc->erase_grp_size : (blkcnt - blk);
            err = mmc_erase_t(mmc, start + blk, blk_r);
            if (err)
                break;

            blk += blk_r;

            /* Waiting for the ready status */
            if (mmc_send_status(mmc, timeout))
                return 0;
        }

    return blk;
}

static unsigned long mmc_write_blocks(struct mmc *mmc, uint32_t start,
        uint32_t blkcnt, const void *src)
{
    struct mmc_cmd cmd;
    struct mmc_data data;
    int timeout = 1000;

    if ((start + blkcnt) > mmc->block_dev.lba) {
        return 0;
    }

    if (blkcnt == 0)
        return 0;
    else if (blkcnt == 1)
        cmd.cmdidx = MMC_CMD_WRITE_SINGLE_BLOCK;
    else
        cmd.cmdidx = MMC_CMD_WRITE_MULTIPLE_BLOCK;

    if (mmc->high_capacity)
        cmd.cmdarg = start;
    else
        cmd.cmdarg = start * mmc->write_bl_len;

    cmd.resp_type = MMC_RSP_R1;

    data.src = src;
    data.blocks = blkcnt;
    data.blocksize = mmc->write_bl_len;
    data.flags = MMC_DATA_WRITE;

    if (mmc_send_cmd(mmc, &cmd, &data)) {
        HAL_SDMMC_TRACE(0,"mmc write failed\n");
        return 0;
    }

    /* SPI multiblock writes terminate using a special
     * token, not a STOP_TRANSMISSION request.
     */
    if (!sdmmc_host_is_spi(mmc) && blkcnt > 1) {
        cmd.cmdidx = MMC_CMD_STOP_TRANSMISSION;
        cmd.cmdarg = 0;
        cmd.resp_type = MMC_RSP_R1b;
        if (mmc_send_cmd(mmc, &cmd, NULL)) {
            HAL_SDMMC_TRACE(0,"mmc fail to send stop cmd\n");
            return 0;
        }
    }

    /* Waiting for the ready status */
    if (mmc_send_status(mmc, timeout))
        return 0;

    return blkcnt;
}

unsigned long mmc_bwrite(int dev_num, uint32_t start, uint32_t blkcnt, const void *src)
{
    uint32_t cur, blocks_todo = blkcnt;

    struct mmc *mmc = find_mmc_device(dev_num);
    if (!mmc)
        return 0;

    if (mmc_set_blocklen(mmc, mmc->write_bl_len))
        return 0;

    do {
        cur = (blocks_todo > mmc->cfg->b_max) ?
            mmc->cfg->b_max : blocks_todo;
        if (mmc_write_blocks(mmc, start, cur, src) != cur)
            return 0;
        blocks_todo -= cur;
        start += cur;
        src += cur * mmc->write_bl_len;
    } while (blocks_todo > 0);

    return blkcnt;
}

static int mmc_go_idle(struct mmc *mmc)
{
    struct mmc_cmd cmd;
    int err;

    mmc_udelay(1000);

    cmd.cmdidx = MMC_CMD_GO_IDLE_STATE;
    cmd.cmdarg = 0;
    cmd.resp_type = MMC_RSP_NONE;

    err = mmc_send_cmd(mmc, &cmd, NULL);

    if (err)
        return err;

    mmc_udelay(2000);

    return 0;
}

static int sd_send_op_cond(struct mmc *mmc)
{
    int timeout = 1000;
    int err;
    struct mmc_cmd cmd;

    while (1) {
        cmd.cmdidx = MMC_CMD_APP_CMD;
        cmd.resp_type = MMC_RSP_R1;
        cmd.cmdarg = 0;

        err = mmc_send_cmd(mmc, &cmd, NULL);

        if (err)
            return err;

        cmd.cmdidx = SD_CMD_APP_SEND_OP_COND;
        cmd.resp_type = MMC_RSP_R3;

        /*
         * Most cards do not answer if some reserved bits
         * in the ocr are set. However, Some controller
         * can set bit 7 (reserved for low voltages), but
         * how to manage low voltages SD card is not yet
         * specified.
         */
        cmd.cmdarg = sdmmc_host_is_spi(mmc) ? 0 :
            (mmc->cfg->voltages & 0xff8000);

        if (mmc->version == SD_VERSION_2)
            cmd.cmdarg |= OCR_HCS;

        err = mmc_send_cmd(mmc, &cmd, NULL);

        if (err)
            return err;

        if (cmd.response[0] & OCR_BUSY)
            break;

        if (timeout-- <= 0)
            return UNUSABLE_ERR;

        mmc_udelay(1000);
    }

    if (mmc->version != SD_VERSION_2)
        mmc->version = SD_VERSION_1_0;

    if (sdmmc_host_is_spi(mmc)) { /* read OCR for spi */
        cmd.cmdidx = MMC_CMD_SPI_READ_OCR;
        cmd.resp_type = MMC_RSP_R3;
        cmd.cmdarg = 0;

        err = mmc_send_cmd(mmc, &cmd, NULL);

        if (err)
            return err;
    }

    mmc->ocr = cmd.response[0];

    mmc->high_capacity = ((mmc->ocr & OCR_HCS) == OCR_HCS);
    mmc->rca = 0;

    return 0;
}

static int mmc_send_op_cond_iter(struct mmc *mmc, int use_arg)
{
    struct mmc_cmd cmd;
    int err;

    cmd.cmdidx = MMC_CMD_SEND_OP_COND;
    cmd.resp_type = MMC_RSP_R3;
    cmd.cmdarg = 0;
    if (use_arg && !sdmmc_host_is_spi(mmc))
        cmd.cmdarg = OCR_HCS |
            (mmc->cfg->voltages &
             (mmc->ocr & OCR_VOLTAGE_MASK)) |
            (mmc->ocr & OCR_ACCESS_MODE);

    err = mmc_send_cmd(mmc, &cmd, NULL);
    if (err)
        return err;
    mmc->ocr = cmd.response[0];
    return 0;
}

static int mmc_send_op_cond(struct mmc *mmc)
{
    int err, i;

    /* Some cards seem to need this */
    // mmc_go_idle(mmc);

    /* Asking to the card its capabilities */
    for (i = 0; i < 2; i++) {
        err = mmc_send_op_cond_iter(mmc, i != 0);
        if (err)
            return err;

        /* exit if not busy (flag seems to be inverted) */
        if (mmc->ocr & OCR_BUSY)
            break;
    }
    mmc->op_cond_pending = 1;
    return 0;
}

static int mmc_complete_op_cond(struct mmc *mmc)
{
    struct mmc_cmd cmd;
    int err;

    mmc->op_cond_pending = 0;
    if (!(mmc->ocr & OCR_BUSY)) {
        while (1) {
            err = mmc_send_op_cond_iter(mmc, 1);
            if (err)
                return err;
            if (mmc->ocr & OCR_BUSY)
                break;
            mmc_udelay(100);
        }
    }

    if (sdmmc_host_is_spi(mmc)) { /* read OCR for spi */
        cmd.cmdidx = MMC_CMD_SPI_READ_OCR;
        cmd.resp_type = MMC_RSP_R3;
        cmd.cmdarg = 0;

        err = mmc_send_cmd(mmc, &cmd, NULL);

        if (err)
            return err;

        mmc->ocr = cmd.response[0];
    }

    mmc->version = MMC_VERSION_UNKNOWN;

    mmc->high_capacity = ((mmc->ocr & OCR_HCS) == OCR_HCS);
    mmc->rca = 1;

    return 0;
}


static int mmc_send_ext_csd(struct mmc *mmc, uint8_t *ext_csd)
{
    struct mmc_cmd cmd;
    struct mmc_data data;
    int err;

    /* Get the Card Status Register */
    cmd.cmdidx = MMC_CMD_SEND_EXT_CSD;
    cmd.resp_type = MMC_RSP_R1;
    cmd.cmdarg = 0;

    data.dest = (char *)ext_csd;
    data.blocks = 1;
    data.blocksize = MMC_MAX_BLOCK_LEN;
    data.flags = MMC_DATA_READ;

    HAL_SDMMC_TRACE(1,"[%s] start...", __func__);
    err = mmc_send_cmd(mmc, &cmd, &data);
    HAL_SDMMC_TRACE(2,"[%s] err = %d", __func__, err);

    return err;
}


static int mmc_switch(struct mmc *mmc, uint8_t set, uint8_t index, uint8_t value)
{
    struct mmc_cmd cmd;
    int timeout = 1000;
    int ret;

    cmd.cmdidx = MMC_CMD_SWITCH;
    cmd.resp_type = MMC_RSP_R1b;
    cmd.cmdarg = (MMC_SWITCH_MODE_WRITE_BYTE << 24) |
        (index << 16) |
        (value << 8);

    ret = mmc_send_cmd(mmc, &cmd, NULL);

    /* Waiting for the ready status */
    if (!ret)
        ret = mmc_send_status(mmc, timeout);

    return ret;

}

static int mmc_change_freq(struct mmc *mmc)
{
    _SDMMC_ALLOC_CACHE_ALIGN_BUFFER(uint8_t, ext_csd, MMC_MAX_BLOCK_LEN);
    char cardtype;
    int err;

    mmc->card_caps = 0;

    if (sdmmc_host_is_spi(mmc))
        return 0;

    /* Only version 4 supports high-speed */
    if (mmc->version < MMC_VERSION_4)
        return 0;

    mmc->card_caps |= MMC_MODE_4BIT | MMC_MODE_8BIT;

    err = mmc_send_ext_csd(mmc, ext_csd);

    if (err)
        return err;

    cardtype = ext_csd[EXT_CSD_CARD_TYPE] & 0xf;

    err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_HS_TIMING, 1);

    if (err)
        return err == SWITCH_ERR ? 0 : err;

    /* Now check to see that it worked */
    err = mmc_send_ext_csd(mmc, ext_csd);

    if (err)
        return err;

    /* No high-speed support */
    if (!ext_csd[EXT_CSD_HS_TIMING])
        return 0;

    /* High Speed is set, there are two types: 52MHz and 26MHz */
    if (cardtype & EXT_CSD_CARD_TYPE_52) {
        if (cardtype & EXT_CSD_CARD_TYPE_DDR_1_8V)
            mmc->card_caps |= MMC_MODE_DDR_52MHz;
        mmc->card_caps |= MMC_MODE_HS_52MHz | MMC_MODE_HS;
    } else {
        mmc->card_caps |= MMC_MODE_HS;
    }

    return 0;
}

static int mmc_set_capacity(struct mmc *mmc, int part_num)
{
    switch (part_num) {
        case 0:
            mmc->capacity = mmc->capacity_user;
            break;
        case 1:
        case 2:
            mmc->capacity = mmc->capacity_boot;
            break;
        case 3:
            mmc->capacity = mmc->capacity_rpmb;
            break;
        case 4:
        case 5:
        case 6:
        case 7:
            mmc->capacity = mmc->capacity_gp[part_num - 4];
            break;
        default:
            return -1;
    }

    mmc->block_dev.lba = _sdmmc_lldiv(mmc->capacity, mmc->read_bl_len);

    return 0;
}

int mmc_getcd(struct mmc *mmc)
{
    int cd;

    cd = board_mmc_getcd(mmc);

    if (cd < 0) {
        if (mmc->cfg->ops->getcd)
            cd = mmc->cfg->ops->getcd(mmc);
        else
            cd = 1;
    }

    return cd;
}

static int sd_switch(struct mmc *mmc, int mode, int group, uint8_t value, uint8_t *resp)
{
    struct mmc_cmd cmd;
    struct mmc_data data;

    /* Switch the frequency */
    cmd.cmdidx = SD_CMD_SWITCH_FUNC;
    cmd.resp_type = MMC_RSP_R1;
    cmd.cmdarg = (mode << 31) | 0xffffff;
    cmd.cmdarg &= ~(0xf << (group * 4));
    cmd.cmdarg |= value << (group * 4);

    data.dest = (char *)resp;
    data.blocksize = 64;
    data.blocks = 1;
    data.flags = MMC_DATA_READ;

    hal_sdmmc_delay(1);

    return mmc_send_cmd(mmc, &cmd, &data);
}


static int sd_change_freq(struct mmc *mmc)
{
    int err;
    struct mmc_cmd cmd;
    _SDMMC_ALLOC_CACHE_ALIGN_BUFFER(uint32_t, scr, 2);
    _SDMMC_ALLOC_CACHE_ALIGN_BUFFER(uint32_t, switch_status, 16);
    struct mmc_data data;
    int timeout;

    mmc->card_caps = 0;

    if (sdmmc_host_is_spi(mmc))
        return 0;

    /* Read the SCR to find out if this card supports higher speeds */
    cmd.cmdidx = MMC_CMD_APP_CMD;
    cmd.resp_type = MMC_RSP_R1;
    cmd.cmdarg = mmc->rca << 16;

    err = mmc_send_cmd(mmc, &cmd, NULL);

    if (err)
        return err;

    cmd.cmdidx = SD_CMD_APP_SEND_SCR;
    cmd.resp_type = MMC_RSP_R1;
    cmd.cmdarg = 0;

    timeout = 3;

retry_scr:
    data.dest = (char *)scr;
    data.blocksize = 8;
    data.blocks = 1;
    data.flags = MMC_DATA_READ;

    err = mmc_send_cmd(mmc, &cmd, &data);

    if (err) {
        if (timeout--)
            goto retry_scr;

        return err;
    }

    mmc->scr[0] = _sdmmc_be32_to_cpu(scr[0]);
    mmc->scr[1] = _sdmmc_be32_to_cpu(scr[1]);

    switch ((mmc->scr[0] >> 24) & 0xf) {
        case 0:
            mmc->version = SD_VERSION_1_0;
            break;
        case 1:
            mmc->version = SD_VERSION_1_10;
            break;
        case 2:
            mmc->version = SD_VERSION_2;
            if ((mmc->scr[0] >> 15) & 0x1)
                mmc->version = SD_VERSION_3;
            break;
        default:
            mmc->version = SD_VERSION_1_0;
            break;
    }

    if (mmc->scr[0] & SD_DATA_4BIT)
        mmc->card_caps |= MMC_MODE_4BIT;

    /* Version 1.0 doesn't support switching */
    if (mmc->version == SD_VERSION_1_0)
        return 0;

    timeout = 4;
    while (timeout--) {
        err = sd_switch(mmc, SD_SWITCH_CHECK, 0, 1,
                (uint8_t *)switch_status);

        if (err)
            return err;

        /* The high-speed function is busy.  Try again */
        if (!(_sdmmc_be32_to_cpu(switch_status[7]) & SD_HIGHSPEED_BUSY))
            break;
    }

    /* If high-speed isn't supported, we return */
    if (!(_sdmmc_be32_to_cpu(switch_status[3]) & SD_HIGHSPEED_SUPPORTED))
        return 0;

    /*
     * If the host doesn't support SD_HIGHSPEED, do not switch card to
     * HIGHSPEED mode even if the card support SD_HIGHSPPED.
     * This can avoid furthur problem when the card runs in different
     * mode between the host.
     */
    if (!((mmc->cfg->host_caps & MMC_MODE_HS_52MHz) &&
                (mmc->cfg->host_caps & MMC_MODE_HS)))
        return 0;

    err = sd_switch(mmc, SD_SWITCH_SWITCH, 0, 1, (uint8_t *)switch_status);

    if (err)
        return err;

    if ((_sdmmc_be32_to_cpu(switch_status[4]) & 0x0f000000) == 0x01000000)
        mmc->card_caps |= MMC_MODE_HS;

    return 0;
}

/* frequency bases */
/* divided by 10 to be nice to platforms without floating point */
static const int fbase[] = {
    10000,
    100000,
    1000000,
    10000000,
};

/* Multiplier values for TRAN_SPEED.  Multiplied by 10 to be nice
 * to platforms without floating point.
 */
static const int multipliers[] = {
    0,  /* reserved */
    10,
    12,
    13,
    15,
    20,
    25,
    30,
    35,
    40,
    45,
    50,
    55,
    60,
    70,
    80,
};

static void mmc_set_ios(struct mmc *mmc)
{
    if (mmc->cfg->ops->set_ios)
        mmc->cfg->ops->set_ios(mmc);
}

void mmc_set_clock(struct mmc *mmc, uint32_t clock)
{
    if (clock > mmc->cfg->f_max)
        clock = mmc->cfg->f_max;

    if (clock < mmc->cfg->f_min)
        clock = mmc->cfg->f_min;

    mmc->clock = clock;

    mmc_set_ios(mmc);
}

static void mmc_set_bus_width(struct mmc *mmc, uint32_t width)
{
    mmc->bus_width = width;

    mmc_set_ios(mmc);
}

static int mmc_startup(struct mmc *mmc)
{
    int err, i;
    uint32_t mult, freq;
    uint32_t cmult, csize, capacity;
    struct mmc_cmd cmd;
    _SDMMC_ALLOC_CACHE_ALIGN_BUFFER(uint8_t, ext_csd, MMC_MAX_BLOCK_LEN);
    _SDMMC_ALLOC_CACHE_ALIGN_BUFFER(uint8_t, test_csd, MMC_MAX_BLOCK_LEN);
    int timeout = 1000;
    bool has_parts = false;
    bool part_completed;

#ifdef CONFIG_MMC_SPI_CRC_ON
    if (sdmmc_host_is_spi(mmc)) { /* enable CRC check for spi */
        cmd.cmdidx = MMC_CMD_SPI_CRC_ON_OFF;
        cmd.resp_type = MMC_RSP_R1;
        cmd.cmdarg = 1;
        err = mmc_send_cmd(mmc, &cmd, NULL);

        if (err)
            return err;
    }
#endif

    /* Put the Card in Identify Mode */
    cmd.cmdidx = sdmmc_host_is_spi(mmc) ? MMC_CMD_SEND_CID :
        MMC_CMD_ALL_SEND_CID; /* cmd not supported in spi */
    cmd.resp_type = MMC_RSP_R2;
    cmd.cmdarg = 0;

    err = mmc_send_cmd(mmc, &cmd, NULL);

    if (err)
        return err;

    memcpy(mmc->cid, cmd.response, 16);

    /*
     * For MMC cards, set the Relative Address.
     * For SD cards, get the Relatvie Address.
     * This also puts the cards into Standby State
     */
    if (!sdmmc_host_is_spi(mmc)) { /* cmd not supported in spi */
        cmd.cmdidx = SD_CMD_SEND_RELATIVE_ADDR;
        cmd.cmdarg = mmc->rca << 16;
        cmd.resp_type = MMC_RSP_R6;

        err = mmc_send_cmd(mmc, &cmd, NULL);

        if (err)
            return err;

        if (IS_SD(mmc))
            mmc->rca = (cmd.response[0] >> 16) & 0xffff;
    }

    /* Get the Card-Specific Data */
    cmd.cmdidx = MMC_CMD_SEND_CSD;
    cmd.resp_type = MMC_RSP_R2;
    cmd.cmdarg = mmc->rca << 16;

    err = mmc_send_cmd(mmc, &cmd, NULL);

    /* Waiting for the ready status */
    mmc_send_status(mmc, timeout);

    if (err)
        return err;

    mmc->csd[0] = cmd.response[0];
    mmc->csd[1] = cmd.response[1];
    mmc->csd[2] = cmd.response[2];
    mmc->csd[3] = cmd.response[3];

    if (mmc->version == MMC_VERSION_UNKNOWN) {
        int version = (cmd.response[0] >> 26) & 0xf;

        switch (version) {
            case 0:
                mmc->version = MMC_VERSION_1_2;
                break;
            case 1:
                mmc->version = MMC_VERSION_1_4;
                break;
            case 2:
                mmc->version = MMC_VERSION_2_2;
                break;
            case 3:
                mmc->version = MMC_VERSION_3;
                break;
            case 4:
                mmc->version = MMC_VERSION_4;
                break;
            default:
                mmc->version = MMC_VERSION_1_2;
                break;
        }
    }

    /* divide frequency by 10, since the mults are 10x bigger */
    freq = fbase[(cmd.response[0] & 0x7)];
    mult = multipliers[((cmd.response[0] >> 3) & 0xf)];

    mmc->tran_speed = freq * mult;

    mmc->dsr_imp = ((cmd.response[1] >> 12) & 0x1);
    mmc->read_bl_len = 1 << ((cmd.response[1] >> 16) & 0xf);

    if (IS_SD(mmc))
        mmc->write_bl_len = mmc->read_bl_len;
    else
        mmc->write_bl_len = 1 << ((cmd.response[3] >> 22) & 0xf);

    if (mmc->high_capacity) {
        csize = (mmc->csd[1] & 0x3f) << 16
            | (mmc->csd[2] & 0xffff0000) >> 16;
        cmult = 8;
    } else {
        csize = (mmc->csd[1] & 0x3ff) << 2
            | (mmc->csd[2] & 0xc0000000) >> 30;
        cmult = (mmc->csd[2] & 0x00038000) >> 15;
    }

    mmc->capacity_user = (csize + 1) << (cmult + 2);
    mmc->capacity_user *= mmc->read_bl_len;
    mmc->capacity_boot = 0;
    mmc->capacity_rpmb = 0;
    for (i = 0; i < 4; i++)
        mmc->capacity_gp[i] = 0;

    if (mmc->read_bl_len > MMC_MAX_BLOCK_LEN)
        mmc->read_bl_len = MMC_MAX_BLOCK_LEN;

    if (mmc->write_bl_len > MMC_MAX_BLOCK_LEN)
        mmc->write_bl_len = MMC_MAX_BLOCK_LEN;

    if ((mmc->dsr_imp) && (0xffffffff != mmc->dsr)) {
        cmd.cmdidx = MMC_CMD_SET_DSR;
        cmd.cmdarg = (mmc->dsr & 0xffff) << 16;
        cmd.resp_type = MMC_RSP_NONE;
        if (mmc_send_cmd(mmc, &cmd, NULL)) {
            HAL_SDMMC_TRACE(0,"MMC: SET_DSR failed\n");
        }
    }

    /* Select the card, and put it into Transfer Mode */
    if (!sdmmc_host_is_spi(mmc)) { /* cmd not supported in spi */
        cmd.cmdidx = MMC_CMD_SELECT_CARD;
        cmd.resp_type = MMC_RSP_R1;
        cmd.cmdarg = mmc->rca << 16;
        err = mmc_send_cmd(mmc, &cmd, NULL);

        if (err)
            return err;
    }

    /*
     * For SD, its erase group is always one sector
     */
    mmc->erase_grp_size = 1;
    mmc->part_config = MMCPART_NOAVAILABLE;
    if (!IS_SD(mmc) && (mmc->version >= MMC_VERSION_4)) {
        /* check  ext_csd version and capacity */
        err = mmc_send_ext_csd(mmc, ext_csd);
        if (err)
            return err;
        if (ext_csd[EXT_CSD_REV] >= 2) {
            /*
             * According to the JEDEC Standard, the value of
             * ext_csd's capacity is valid if the value is more
             * than 2GB
             */
            capacity = ext_csd[EXT_CSD_SEC_CNT] << 0
                | ext_csd[EXT_CSD_SEC_CNT + 1] << 8
                | ext_csd[EXT_CSD_SEC_CNT + 2] << 16
                | ext_csd[EXT_CSD_SEC_CNT + 3] << 24;
            capacity *= MMC_MAX_BLOCK_LEN;
            if ((capacity >> 20) > 2 * 1024)
                mmc->capacity_user = capacity;
        }

        switch (ext_csd[EXT_CSD_REV]) {
            case 1:
                mmc->version = MMC_VERSION_4_1;
                break;
            case 2:
                mmc->version = MMC_VERSION_4_2;
                break;
            case 3:
                mmc->version = MMC_VERSION_4_3;
                break;
            case 5:
                mmc->version = MMC_VERSION_4_41;
                break;
            case 6:
                mmc->version = MMC_VERSION_4_5;
                break;
            case 7:
                mmc->version = MMC_VERSION_5_0;
                break;
        }

        /* The partition data may be non-zero but it is only
         * effective if PARTITION_SETTING_COMPLETED is set in
         * EXT_CSD, so ignore any data if this bit is not set,
         * except for enabling the high-capacity group size
         * definition (see below). */
        part_completed = !!(ext_csd[EXT_CSD_PARTITION_SETTING] &
                EXT_CSD_PARTITION_SETTING_COMPLETED);

        /* store the partition info of emmc */
        mmc->part_support = ext_csd[EXT_CSD_PARTITIONING_SUPPORT];
        if ((ext_csd[EXT_CSD_PARTITIONING_SUPPORT] & PART_SUPPORT) ||
                ext_csd[EXT_CSD_BOOT_MULT])
            mmc->part_config = ext_csd[EXT_CSD_PART_CONF];
        if (part_completed &&
                (ext_csd[EXT_CSD_PARTITIONING_SUPPORT] & ENHNCD_SUPPORT))
            mmc->part_attr = ext_csd[EXT_CSD_PARTITIONS_ATTRIBUTE];

        mmc->capacity_boot = ext_csd[EXT_CSD_BOOT_MULT] << 17;

        mmc->capacity_rpmb = ext_csd[EXT_CSD_RPMB_MULT] << 17;

        for (i = 0; i < 4; i++) {
            int idx = EXT_CSD_GP_SIZE_MULT + i * 3;
            uint32_t mult = (ext_csd[idx + 2] << 16) +
                (ext_csd[idx + 1] << 8) + ext_csd[idx];
            if (mult)
                has_parts = true;
            if (!part_completed)
                continue;
            mmc->capacity_gp[i] = mult;
            mmc->capacity_gp[i] *=
                ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE];
            mmc->capacity_gp[i] *= ext_csd[EXT_CSD_HC_WP_GRP_SIZE];
            mmc->capacity_gp[i] <<= 19;
        }

        if (part_completed) {
            mmc->enh_user_size =
                (ext_csd[EXT_CSD_ENH_SIZE_MULT+2] << 16) +
                (ext_csd[EXT_CSD_ENH_SIZE_MULT+1] << 8) +
                ext_csd[EXT_CSD_ENH_SIZE_MULT];
            mmc->enh_user_size *= ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE];
            mmc->enh_user_size *= ext_csd[EXT_CSD_HC_WP_GRP_SIZE];
            mmc->enh_user_size <<= 19;
            mmc->enh_user_start =
                (ext_csd[EXT_CSD_ENH_START_ADDR+3] << 24) +
                (ext_csd[EXT_CSD_ENH_START_ADDR+2] << 16) +
                (ext_csd[EXT_CSD_ENH_START_ADDR+1] << 8) +
                ext_csd[EXT_CSD_ENH_START_ADDR];
            if (mmc->high_capacity)
                mmc->enh_user_start <<= 9;
        }

        /*
         * Host needs to enable ERASE_GRP_DEF bit if device is
         * partitioned. This bit will be lost every time after a reset
         * or power off. This will affect erase size.
         */
        if (part_completed)
            has_parts = true;
        if ((ext_csd[EXT_CSD_PARTITIONING_SUPPORT] & PART_SUPPORT) &&
                (ext_csd[EXT_CSD_PARTITIONS_ATTRIBUTE] & PART_ENH_ATTRIB))
            has_parts = true;
        if (has_parts) {
            err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL,
                    EXT_CSD_ERASE_GROUP_DEF, 1);

            if (err)
                return err;
            else
                ext_csd[EXT_CSD_ERASE_GROUP_DEF] = 1;
        }

        if (ext_csd[EXT_CSD_ERASE_GROUP_DEF] & 0x01) {
            /* Read out group size from ext_csd */
            mmc->erase_grp_size =
                ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE] * 1024;
            /*
             * if high capacity and partition setting completed
             * SEC_COUNT is valid even if it is smaller than 2 GiB
             * JEDEC Standard JESD84-B45, 6.2.4
             */
            if (mmc->high_capacity && part_completed) {
                capacity = (ext_csd[EXT_CSD_SEC_CNT]) |
                    (ext_csd[EXT_CSD_SEC_CNT + 1] << 8) |
                    (ext_csd[EXT_CSD_SEC_CNT + 2] << 16) |
                    (ext_csd[EXT_CSD_SEC_CNT + 3] << 24);
                capacity *= MMC_MAX_BLOCK_LEN;
                mmc->capacity_user = capacity;
            }
        } else {
            /* Calculate the group size from the csd value. */
            int erase_gsz, erase_gmul;
            erase_gsz = (mmc->csd[2] & 0x00007c00) >> 10;
            erase_gmul = (mmc->csd[2] & 0x000003e0) >> 5;
            mmc->erase_grp_size = (erase_gsz + 1)
                * (erase_gmul + 1);
        }

        mmc->hc_wp_grp_size = 1024
            * ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE]
            * ext_csd[EXT_CSD_HC_WP_GRP_SIZE];

        mmc->wr_rel_set = ext_csd[EXT_CSD_WR_REL_SET];
    }

    err = mmc_set_capacity(mmc, mmc->part_num);
    if (err)
        return err;

    if (IS_SD(mmc))
        err = sd_change_freq(mmc);
    else
        err = mmc_change_freq(mmc);

    if (err)
        return err;

    /* Restrict card's capabilities by what the host can do */
    mmc->card_caps &= mmc->cfg->host_caps;

    if (IS_SD(mmc)) {
#ifdef __BUS_WIDTH_SUPPORT_4BIT__
        if (mmc->card_caps & MMC_MODE_4BIT) {
            cmd.cmdidx = MMC_CMD_APP_CMD;
            cmd.resp_type = MMC_RSP_R1;
            cmd.cmdarg = mmc->rca << 16;

            err = mmc_send_cmd(mmc, &cmd, NULL);
            if (err)
                return err;

            cmd.cmdidx = SD_CMD_APP_SET_BUS_WIDTH;
            cmd.resp_type = MMC_RSP_R1;
            cmd.cmdarg = 2;
            err = mmc_send_cmd(mmc, &cmd, NULL);
            if (err)
                return err;

            mmc_set_bus_width(mmc, 4);
        }
#endif
        if (mmc->card_caps & MMC_MODE_HS)
            mmc->tran_speed = 50000000;
        else
            mmc->tran_speed = 25000000;
    } else if (mmc->version >= MMC_VERSION_4) {
        /* Only version 4 of MMC supports wider bus widths */
        int idx;

        /* An array of possible bus widths in order of preference */
        static unsigned ext_csd_bits[] = {
            EXT_CSD_DDR_BUS_WIDTH_8,
            EXT_CSD_DDR_BUS_WIDTH_4,
            EXT_CSD_BUS_WIDTH_8,
            EXT_CSD_BUS_WIDTH_4,
            EXT_CSD_BUS_WIDTH_1,
        };

        /* An array to map CSD bus widths to host cap bits */
        static unsigned ext_to_hostcaps[] = {
            [EXT_CSD_DDR_BUS_WIDTH_4] =
                MMC_MODE_DDR_52MHz | MMC_MODE_4BIT,
            [EXT_CSD_DDR_BUS_WIDTH_8] =
                MMC_MODE_DDR_52MHz | MMC_MODE_8BIT,
            [EXT_CSD_BUS_WIDTH_4] = MMC_MODE_4BIT,
            [EXT_CSD_BUS_WIDTH_8] = MMC_MODE_8BIT,
        };

        /* An array to map chosen bus width to an integer */
        static unsigned widths[] = {
            8, 4, 8, 4, 1,
        };

        for (idx=0; idx < ARRAY_SIZE(ext_csd_bits); idx++) {
            unsigned int extw = ext_csd_bits[idx];
            unsigned int caps = ext_to_hostcaps[extw];

            /*
             * If the bus width is still not changed,
             * don't try to set the default again.
             * Otherwise, recover from switch attempts
             * by switching to 1-bit bus width.
             */
            if (extw == EXT_CSD_BUS_WIDTH_1 &&
                    mmc->bus_width == 1) {
                err = 0;
                break;
            }

            /*
             * Check to make sure the card and controller support
             * these capabilities
             */
            if ((mmc->card_caps & caps) != caps)
                continue;

            err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL,
                    EXT_CSD_BUS_WIDTH, extw);

            if (err)
                continue;

            mmc->ddr_mode = (caps & MMC_MODE_DDR_52MHz) ? 1 : 0;
            mmc_set_bus_width(mmc, widths[idx]);

            err = mmc_send_ext_csd(mmc, test_csd);

            if (err)
                continue;

            /* Only compare read only fields */
            if (ext_csd[EXT_CSD_PARTITIONING_SUPPORT]
                    == test_csd[EXT_CSD_PARTITIONING_SUPPORT] &&
                    ext_csd[EXT_CSD_HC_WP_GRP_SIZE]
                    == test_csd[EXT_CSD_HC_WP_GRP_SIZE] &&
                    ext_csd[EXT_CSD_REV]
                    == test_csd[EXT_CSD_REV] &&
                    ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE]
                    == test_csd[EXT_CSD_HC_ERASE_GRP_SIZE] &&
                    memcmp(&ext_csd[EXT_CSD_SEC_CNT],
                        &test_csd[EXT_CSD_SEC_CNT], 4) == 0)
                break;
            else
                err = SWITCH_ERR;
        }

        if (err)
            return err;

        if (mmc->card_caps & MMC_MODE_HS) {
            if (mmc->card_caps & MMC_MODE_HS_52MHz)
                mmc->tran_speed = 52000000;
            else
                mmc->tran_speed = 26000000;
        }
    }

    mmc_set_clock(mmc, mmc->tran_speed);

    /* Fix the block length for DDR mode */
    if (mmc->ddr_mode) {
        mmc->read_bl_len = MMC_MAX_BLOCK_LEN;
        mmc->write_bl_len = MMC_MAX_BLOCK_LEN;
    }

    /* fill in device description */
    mmc->block_dev.lun = 0;
    mmc->block_dev.type = 0;
    mmc->block_dev.blksz = mmc->read_bl_len;
    //mmc->block_dev.log2blksz = LOG2(mmc->block_dev.blksz);
    mmc->block_dev.lba = _sdmmc_lldiv(mmc->capacity, mmc->read_bl_len);
#if !defined(CONFIG_SPL_BUILD) || defined(CONFIG_SPL_LIBCOMMON_SUPPORT)
    sprintf(mmc->block_dev.vendor, "Man %06x Snr %04x%04x",
            (unsigned)(mmc->cid[0] >> 24), (unsigned)(mmc->cid[2] & 0xffff),
            (unsigned)((mmc->cid[3] >> 16) & 0xffff));
    sprintf(mmc->block_dev.product, "%c%c%c%c%c%c", (char)mmc->cid[0],
            (char)(mmc->cid[1] >> 24), (char)(mmc->cid[1] >> 16),
            (char)(mmc->cid[1] >> 8), (char)mmc->cid[1],
            (char)(mmc->cid[2] >> 24));
    sprintf(mmc->block_dev.revision, "%d.%d", (unsigned)((mmc->cid[2] >> 20) & 0xf),
            (unsigned)((mmc->cid[2] >> 16) & 0xf));
#else
    mmc->block_dev.vendor[0] = 0;
    mmc->block_dev.product[0] = 0;
    mmc->block_dev.revision[0] = 0;
#endif

    HAL_SDMMC_TRACE(1,"%s done\n", __func__);
    return 0;
}

static int mmc_send_if_cond(struct mmc *mmc)
{
    struct mmc_cmd cmd;
    int err;

    cmd.cmdidx = SD_CMD_SEND_IF_COND;
    /* We set the bit if the host supports voltages between 2.7 and 3.6 V */
    cmd.cmdarg = ((mmc->cfg->voltages & 0xff8000) != 0) << 8 | 0xaa;
    cmd.resp_type = MMC_RSP_R7;

    err = mmc_send_cmd(mmc, &cmd, NULL);

    if (err)
        return err;

    if ((cmd.response[0] & 0xff) != 0xaa)
        return UNUSABLE_ERR;
    else
        mmc->version = SD_VERSION_2;

    return 0;
}

struct mmc *mmc_create(int id, const struct mmc_config *cfg, void *priv)
{
    struct mmc *mmc;

    /* quick validation */
    if (cfg == NULL || cfg->ops == NULL || cfg->ops->send_cmd == NULL ||
            cfg->f_min == 0 || cfg->f_max == 0 || cfg->b_max == 0)
        return NULL;

    mmc = &sdmmc_devices[id];
    if (mmc == NULL)
        return NULL;

    mmc->cfg = cfg;
    mmc->priv = priv;

    /* the following chunk was mmc_register() */

    /* Setup dsr related values */
    mmc->dsr_imp = 0;
    mmc->dsr = 0xffffffff;
    /* Setup the universal parts of the block interface just once */
    mmc->block_dev.removable = 1;
    /* setup initial part type */
    mmc->block_dev.part_type = mmc->cfg->part_type;

    return mmc;
}

int mmc_start_init(struct mmc *mmc)
{
    int err;

    /* we pretend there's no card when init is NULL */
    if (mmc_getcd(mmc) == 0 || mmc->cfg->ops->init == NULL) {
        mmc->has_init = 0;
        HAL_SDMMC_TRACE(1,"%s : no card present\n", __func__);
        return NO_CARD_ERR;
    }

    if (mmc->has_init)
        return 0;

    /* made sure it's not NULL earlier */
    err = mmc->cfg->ops->init(mmc);
    HAL_SDMMC_TRACE(1,"%s : init done\n", __func__);

    if (err)
        return err;

    mmc->ddr_mode = 0;
    mmc_set_bus_width(mmc, 1);
    mmc_set_clock(mmc, 1);

    /* Reset the Card */
    err = mmc_go_idle(mmc);
    HAL_SDMMC_TRACE(1,"%s : idle done\n", __func__);

    if (err)
        return err;

    /* The internal partition reset to user partition(0) at every CMD0*/
    mmc->part_num = 0;

    /* Test for SD version 2 */
    err = mmc_send_if_cond(mmc);

    /* Now try to get the SD card's operating condition */
    err = sd_send_op_cond(mmc);

    /* If the command timed out, we check for an MMC card */
    if (err == TIMEOUT) {
        err = mmc_send_op_cond(mmc);

        if (err) {
#if !defined(CONFIG_SPL_BUILD) || defined(CONFIG_SPL_LIBCOMMON_SUPPORT)
            HAL_SDMMC_TRACE(1,"%s : Card did not respond to voltage select!\n", __func__);
#endif
            return UNUSABLE_ERR;
        }
    }

    if (!err)
        mmc->init_in_progress = 1;

    HAL_SDMMC_TRACE(1,"%s : done\n", __func__);
    return err;
}

static int mmc_complete_init(struct mmc *mmc)
{
    int err = 0;

    mmc->init_in_progress = 0;
    if (mmc->op_cond_pending)
        err = mmc_complete_op_cond(mmc);

    if (!err)
        err = mmc_startup(mmc);
    if (err)
        mmc->has_init = 0;
    else
        mmc->has_init = 1;
    return err;
}

void hal_sdmmc_irq_handler(void)
{
	uint32_t cdetect,mask;
	struct sdmmcip_host *host = &sdmmc_host[HAL_SDMMC_ID_0];
    mask = sdmmcip_readl(host, SDMMCIP_REG_RINTSTS);

	if (mask & SDMMCIP_REG_INTMSK_CD){
		sdmmcip_writel(host, SDMMCIP_REG_RINTSTS, SDMMCIP_REG_INTMSK_CD);
		cdetect = sdmmcip_readl(host, SDMMCIP_REG_CDETECT);
		HAL_SDMMC_TRACE(3,"%s cdetect:%d mask:%x\n", __func__, cdetect, mask);
		if (sdmmc_detected_callback)
			sdmmc_detected_callback(cdetect&0x01 ? 0 : 1);
	}
}

int hal_sdmmc_enable_detecter(enum HAL_SDMMC_ID_T id,void (* cb)(uint8_t))
{
	uint32_t ctrl;
    struct sdmmcip_host *host = NULL;
	HAL_SDMMC_TRACE(1,"%s\n", __func__);

	sdmmc_detected_callback = cb;

    host = &sdmmc_host[id];
    host->ioaddr = (void *)sdmmc_ip_base[id];

	if (!sdmmcip_wait_reset(host, SDMMCIP_REG_RESET_ALL)) {
		HAL_SDMMC_TRACE(2,"%s[%d] Fail-reset!!\n", __func__, __LINE__);
		return -1;
	}

	host->detect_enb = true;

	sdmmcip_writel(host, SDMMCIP_REG_RINTSTS, 0xFFFFFFFF);
	sdmmcip_writel(host, SDMMCIP_REG_INTMASK, SDMMCIP_REG_INTMSK_CD);

    ctrl = SDMMCIP_REG_INT_EN;
    sdmmcip_writel(host, SDMMCIP_REG_CTRL, ctrl);

	NVIC_SetVector(SDMMC_IRQn, (uint32_t)hal_sdmmc_irq_handler);
	NVIC_SetPriority(SDMMC_IRQn, IRQ_PRIORITY_NORMAL);
    NVIC_ClearPendingIRQ(SDMMC_IRQn);
	NVIC_EnableIRQ(SDMMC_IRQn);
	return 0;
}

void hal_sdmmc_disable_detecter(enum HAL_SDMMC_ID_T id)
{
	uint32_t ctrl = 0;
    struct sdmmcip_host *host = NULL;
    host = &sdmmc_host[id];

	NVIC_DisableIRQ(SDMMC_IRQn);
	NVIC_SetVector(SDMMC_IRQn, (uint32_t)NULL);

    sdmmcip_writel(host, SDMMCIP_REG_RINTSTS, 0xFFFFFFFF);
    sdmmcip_writel(host, SDMMCIP_REG_INTMASK, ~SDMMCIP_REG_INTMSK_ALL);

	ctrl = sdmmcip_readl(host, SDMMCIP_REG_CTRL);
	ctrl |= (SDMMCIP_REG_DMA_EN);
    sdmmcip_writel(host, SDMMCIP_REG_CTRL, ctrl);

	host->detect_enb = false;
	sdmmc_detected_callback = NULL;
}

int mmc_init(struct mmc *mmc)
{
    int err = 0;
    if (mmc->has_init)
        return 0;

    if (!mmc->init_in_progress)
        err = mmc_start_init(mmc);

    if (!err)
        err = mmc_complete_init(mmc);

    return err;
}

/* hal api */
static void hal_sdmmc_delay(uint32_t ms)
{
    if (sdmmc_delay) {
        sdmmc_delay(ms);
    } else {
        hal_sys_timer_delay(MS_TO_TICKS(ms));
    }
}

HAL_SDMMC_DELAY_FUNC hal_sdmmc_set_delay_func(HAL_SDMMC_DELAY_FUNC new_func)
{
    HAL_SDMMC_DELAY_FUNC old_func = sdmmc_delay;
    sdmmc_delay = new_func;
    return old_func;
}

int32_t hal_sdmmc_open(enum HAL_SDMMC_ID_T id)
{
    int32_t ret = 0;
    struct sdmmcip_host *host = NULL;
    HAL_SDMMC_ASSERT(id < HAL_SDMMC_ID_NUM, invalid_id, id);

#ifdef FPGA
    hal_cmu_sdmmc_set_freq(HAL_CMU_PERIPH_FREQ_26M);
#else
    hal_cmu_sdmmc_set_freq(HAL_CMU_PERIPH_FREQ_52M);
#endif

    hal_cmu_clock_enable(HAL_CMU_MOD_O_SDMMC);
    hal_cmu_clock_enable(HAL_CMU_MOD_H_SDMMC);
    hal_cmu_reset_clear(HAL_CMU_MOD_O_SDMMC);
    hal_cmu_reset_clear(HAL_CMU_MOD_H_SDMMC);

#ifdef FPGA
#ifdef CHIP_BEST1000
    /* iomux */
    *((volatile uint32_t *)0x4001f004) |= 0x1<<16;
    *((volatile uint32_t *)0x4001f004) |= 0x1<<17;
#endif
#endif

    host = &sdmmc_host[id];

    host->ioaddr = (void *)sdmmc_ip_base[id];
    //host->buswidth   = 4;
    host->buswidth   = 1;
    host->clksel     = 0;
    host->bus_hz     = _SDMMC_CLOCK;
    host->dev_index  = id;
    /* fifo threshold : MSIZE 0 - 1 transter, rx water mark 0 - greater than 0 word triiger dma-req,
     * tx water mark 1 - less than or equal 1 word triiger dma-req */
    host->fifoth_val = MSIZE(0) | RX_WMARK(0) | TX_WMARK(1);

    host->cfg.ops = &sdmmcip_ops;
    host->cfg.f_min = 400000;
    //host->cfg.f_min = 1000000;
    //host->cfg.f_min = 25000000;
    //host->cfg.f_min = 52000000;
    //host->cfg.f_max  = 1000000;
    host->cfg.f_max = 26000000;
    //host->cfg.f_max = 52000000;
    host->cfg.voltages = MMC_VDD_32_33 | MMC_VDD_33_34 | MMC_VDD_165_195;
    host->cfg.host_caps = 0;

#ifdef HAL_SDMMC_USE_DMA
    host->tx_dma_handler = sdmmcip_ext_dma_irq_handlers[id*2];
    host->rx_dma_handler = sdmmcip_ext_dma_irq_handlers[id*2+1];
#endif

    if (host->buswidth == 8) {
        host->cfg.host_caps |= MMC_MODE_8BIT;
        host->cfg.host_caps &= ~MMC_MODE_4BIT;
    } else if (host->buswidth == 4) {
        host->cfg.host_caps |= MMC_MODE_4BIT;
        host->cfg.host_caps &= ~MMC_MODE_8BIT;
    }
    else {
        //host->cfg.host_caps &= ~MMC_MODE_4BIT;
        host->cfg.host_caps |= MMC_MODE_4BIT;
        host->cfg.host_caps &= ~MMC_MODE_8BIT;
    }

    host->cfg.host_caps |= MMC_MODE_HS | MMC_MODE_HS_52MHz;

    host->cfg.b_max = CONFIG_SYS_MMC_MAX_BLK_COUNT;

    host->mmc = mmc_create(host->dev_index, &host->cfg, host);

    ret = mmc_init(host->mmc);

    hal_sdmmc_dump(HAL_SDMMC_ID_0);

    return ret;
}
uint32_t hal_sdmmc_read_blocks(enum HAL_SDMMC_ID_T id, uint32_t start_block, uint32_t block_count, uint8_t* dest)
{
    HAL_SDMMC_ASSERT(id < HAL_SDMMC_ID_NUM, invalid_id, id);

    return mmc_bread(id, start_block, block_count, dest);
}

uint32_t hal_sdmmc_write_blocks(enum HAL_SDMMC_ID_T id, uint32_t start_block, uint32_t block_count, uint8_t* src)
{
    HAL_SDMMC_ASSERT(id < HAL_SDMMC_ID_NUM, invalid_id, id);

    return mmc_bwrite(id, start_block, block_count, src);
}

void hal_sdmmc_close(enum HAL_SDMMC_ID_T id)
{
	struct sdmmcip_host *host = NULL;

	HAL_SDMMC_ASSERT(id < HAL_SDMMC_ID_NUM, invalid_id, id);

	HAL_SDMMC_TRACE(1,"%s\n", __func__);

	host = &sdmmc_host[id];
	while(host->mmc->init_in_progress)
		hal_sdmmc_delay(1);

	host->mmc->has_init = 0;

    hal_cmu_reset_set(HAL_CMU_MOD_H_SDMMC);
    hal_cmu_reset_set(HAL_CMU_MOD_O_SDMMC);
    hal_cmu_clock_disable(HAL_CMU_MOD_H_SDMMC);
    hal_cmu_clock_disable(HAL_CMU_MOD_O_SDMMC);
}

void hal_sdmmc_dump(enum HAL_SDMMC_ID_T id)
{
    struct mmc *mmc = NULL;
    struct sdmmcip_host *host = NULL;
    HAL_SDMMC_ASSERT(id < HAL_SDMMC_ID_NUM, invalid_id, id);

    host = &sdmmc_host[id];
    mmc = host->mmc;

    HAL_SDMMC_TRACE(0,"-----------hal_sdmmc_dump-------------\n");
    HAL_SDMMC_TRACE(1,"    [sdmmc id]                       : %d\n", id);
    HAL_SDMMC_TRACE(1,"    [io register address]            : 0x%x\n", sdmmc_ip_base[id]);
    HAL_SDMMC_TRACE(1,"    [ddr mode]                       : %d\n", mmc->ddr_mode);
    switch(mmc->version)
    {
        case SD_VERSION_1_0:  HAL_SDMMC_TRACE(0,"    [version]                        : SD_VERSION_1_0\n");  break;
        case SD_VERSION_1_10: HAL_SDMMC_TRACE(0,"    [version]                        : SD_VERSION_1_10\n"); break;
        case SD_VERSION_2:    HAL_SDMMC_TRACE(0,"    [version]                        : SD_VERSION_2\n");    break;
        case SD_VERSION_3:    HAL_SDMMC_TRACE(0,"    [version]                        : SD_VERSION_3\n");    break;
        case MMC_VERSION_5_0: HAL_SDMMC_TRACE(0,"    [version]                        : MMC_VERSION_5_0\n"); break;
        case MMC_VERSION_4_5: HAL_SDMMC_TRACE(0,"    [version]                        : MMC_VERSION_4_5\n"); break;
        case MMC_VERSION_4_41:HAL_SDMMC_TRACE(0,"    [version]                        : MMC_VERSION_4_41\n");break;
        case MMC_VERSION_4_3: HAL_SDMMC_TRACE(0,"    [version]                        : MMC_VERSION_4_3\n"); break;
        case MMC_VERSION_4_2: HAL_SDMMC_TRACE(0,"    [version]                        : MMC_VERSION_4_2\n"); break;
        case MMC_VERSION_4_1: HAL_SDMMC_TRACE(0,"    [version]                        : MMC_VERSION_4_1\n"); break;
        case MMC_VERSION_4:   HAL_SDMMC_TRACE(0,"    [version]                        : MMC_VERSION_4\n");   break;
        case MMC_VERSION_3:   HAL_SDMMC_TRACE(0,"    [version]                        : MMC_VERSION_3\n");   break;
        case MMC_VERSION_2_2: HAL_SDMMC_TRACE(0,"    [version]                        : MMC_VERSION_2_2\n");   break;
        case MMC_VERSION_1_4: HAL_SDMMC_TRACE(0,"    [version]                        : MMC_VERSION_1_4\n");   break;
        case MMC_VERSION_1_2: HAL_SDMMC_TRACE(0,"    [version]                        : MMC_VERSION_1_2\n");   break;
        default:              HAL_SDMMC_TRACE(0,"    [version]                        : unkown version\n");   break;
    }
    HAL_SDMMC_TRACE(1,"    [is SD card]                     : 0x%x\n", IS_SD(mmc));
    HAL_SDMMC_TRACE(1,"    [high_capacity]                  : 0x%x\n", mmc->high_capacity);
    HAL_SDMMC_TRACE(1,"    [bus_width]                      : 0x%x\n", mmc->bus_width);
    HAL_SDMMC_TRACE(1,"    [clock]                          : %d\n", mmc->clock);
    HAL_SDMMC_TRACE(1,"    [card_caps]                      : 0x%x\n", mmc->card_caps);
    HAL_SDMMC_TRACE(1,"    [ocr]                            : 0x%x\n", mmc->ocr);
    HAL_SDMMC_TRACE(1,"    [dsr]                            : 0x%x\n", mmc->dsr);
    HAL_SDMMC_TRACE(1,"    [capacity_user/1024]             : %d(K)\n", (uint32_t)(mmc->capacity_user/1024));
    HAL_SDMMC_TRACE(1,"    [capacity_user/1024/1024]        : %d(M)\n", (uint32_t)(mmc->capacity_user/1024/1024));
    HAL_SDMMC_TRACE(1,"    [capacity_user/1024/1024/1024]   : %d(G)\n", (uint32_t)(mmc->capacity_user/1024/1024/1024));
    HAL_SDMMC_TRACE(1,"    [read_bl_len]                    : %d\n", mmc->read_bl_len);
    HAL_SDMMC_TRACE(1,"    [write_bl_len]                   : %d\n", mmc->write_bl_len);
    HAL_SDMMC_TRACE(0,"--------------------------------------\n");
}
void hal_sdmmc_info(enum HAL_SDMMC_ID_T id, uint32_t *sector_count, uint32_t *sector_size)
{
    struct mmc *mmc = NULL;
    struct sdmmcip_host *host = NULL;
    HAL_SDMMC_ASSERT(id < HAL_SDMMC_ID_NUM, invalid_id, id);

    host = &sdmmc_host[id];
    mmc = host->mmc;

    if (sector_size) {
        *sector_size = mmc->read_bl_len;
        HAL_SDMMC_TRACE(1,"[sdmmc] sector_size %d\n", *sector_size);
    }
    if (sector_count) {
        if (mmc->read_bl_len != 0)
            *sector_count = mmc->capacity_user/mmc->read_bl_len;
        else
            *sector_count = 0;
        HAL_SDMMC_TRACE(1,"[sdmmc] sector_count %d\n", *sector_count);
    }
}

#endif // CHIP_HAS_SDMMC

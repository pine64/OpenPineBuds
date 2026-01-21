/***************************************************************************
*
*Copyright 2015-2019 BES.
*All rights reserved. All unpublished rights reserved.
*
*No part of this work may be used or reproduced in any form or by any
*means, or stored in a database or retrieval system, without prior written
*permission of BES.
*
*Use of this work is governed by a license granted by BES.
*This work contains confidential and proprietary information of
*BES. which is protected by copyright, trade secret,
*trademark and other intellectual property rights.
*
****************************************************************************/


#ifndef __NVRECORD_GSOUND_H__
#define __NVRECORD_GSOUND_H__

#ifdef __cplusplus
extern "C"{
#endif

/*****************************header include********************************/
#include "nvrecord_extension.h"

/******************************macro defination*****************************/
#define F_4K_ALIGN(val) (((val)+0xFFF)&~0xFFF)
#define IS_F_4K_ALIGNED(val) (val==(val&~0xFFF))

#define INVALID_MODEL_NUM 0xFF          //!< used to distinguish from default 0 after first time bootup
#define DEFAULT_MODEL_NUM 1             //!< number of default supported hotword model
#define DEFAULT_HOTWORD_MODEL_ID "200Q" //!< default model ID, provided by Google

/******************************type defination******************************/

/****************************function declearation**************************/

/**
 * @brief Init the gsound info pointer which is pointing to the gsound info
 * saved in nv_section
 * 
 */
void nv_record_gsound_rec_init(void);

/**
 * @brief Get the pointer of gsound info saved in nv_section
 * 
 * @param ptr           Pointer to get the result
 */
void nv_record_gsound_rec_get_ptr(void **ptr);

/**
 * @brief Update the enable state of gsound service in nv_section
 * 
 * @param enable        true for enable, false for disable
 */
void nv_record_gsound_rec_updata_enable_state(bool enable);


#ifdef GSOUND_HOTWORD_ENABLED
/*---------------------------------------------------------------------------
 *            nv_record_gsound_rec_is_model_insert_allowed
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    is input model is allowed to be a supported model
 *
 * Parameters:
 *    model : string of model ID
 *
 * Return:
 *    true : imput is allowed to be a supported model
 *    false : imput is not allowed to be a supported model
 */
bool nv_record_gsound_rec_is_model_insert_allowed(const char *model);

/*---------------------------------------------------------------------------
 *            nv_record_gsound_rec_add_new_model
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    add a new hotword model
 *
 * Parameters:
 *    info : should be a pointer of structure HOTWORD_MODEL_INFO_T
 *           NOTE: startAddr of HOTWORD_MODEL_INFO_T is invalid here
 *
 * Return:
 *    true : add successfully
 *    false : fail to add
 */
bool nv_record_gsound_rec_add_new_model(void *info);

/*---------------------------------------------------------------------------
 *            nv_record_gsound_rec_get_hotword_model_addr
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    get hotword model file address accroding to the model ID
 *
 * Parameters:
 *    model_id : model id
 *    addNew : the flag to mark if return an avaliable addr for new model
 *               when incoming model ID is not found in flash
 *    newModelLen: the length of new incoming model
 *                 NOTE: only useful when addNew is true
 *
 * Return:
 *    the address of model file respective to given model ID
 */
uint32_t nv_record_gsound_rec_get_hotword_model_addr(const char *model_id, bool addNew, int32_t newModelLen);

/**
 * @brief get supported hotword model ID
 * 
 * @param models_out : pointer of supported model id string
 * @param length_out : length of supported model id string
 */
int nv_record_gsound_rec_get_supported_model_id(char *models_out,
                                                 uint8_t *length_out);
#endif

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __NVRECORD_GSOUND_H__ */
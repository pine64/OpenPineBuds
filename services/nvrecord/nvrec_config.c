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
#define LOG_TAG "bt_osi_config"

#include <assert.h>
#include <ctype.h>
#include "stdio.h"
#include <stdlib.h>
#include <string.h>
#include "list_ext.h"
#include "nvrecord.h"
#include "nvrec_config.h"
#include "hal_trace.h"
static nvrec_section_t *nvrec_section_new(const char *name);
static void nvrec_section_free(void *ptr);

static nvrec_entry_t *nvrec_entry_new(const char *key,const char *value,const char *descrip);
static nvrec_entry_t *nvrec_entry_find(const nvrec_config_t *config, const char *section, const char *key);

void *cfg_malloc (unsigned int size)
{
    void *ptr = NULL;
    if(size == 0)
        return NULL;
    ptr = pool_malloc(size);
    if(ptr == NULL)
    {
        printf("cfg_malloc......ERROR\n");
    }
    return ptr;
}

void *cfg_zmalloc (unsigned int size)
{
    void *ptr = NULL;
    ptr = cfg_malloc(size);
    memset(ptr, 0, size);
    return ptr;
}

void cfg_free (void *p_mem)
{
    pool_free(p_mem);
}

nvrec_config_t *nvrec_config_new(const char *filename) {
    nvrec_config_t *config;
    //add your code at here
//    printf("nvrec_config_new-->cfg_zmalloc..........................\r\n");
    config = (nvrec_config_t *)cfg_zmalloc(sizeof(nvrec_config_t));
    if(config)
        config->sections = list_new_ext(nvrec_section_free);

    return config;
}

void nvrec_config_free(nvrec_config_t *config) {
  if (!config)
    return;

  list_free_ext(config->sections);
//  printf("nvrec_config_free-->cfg_free..........................\r\n");
  cfg_free(config);
}

bool nvrec_config_has_section(const nvrec_config_t *config, const char *section) {
  assert(config != NULL);
  assert(section != NULL);

  return (nvrec_section_find(config, section) != NULL);
}

bool nvrec_config_has_key(const nvrec_config_t *config, const char *section, const char *key) {
  assert(config != NULL);
  assert(section != NULL);
  assert(key != NULL);

  return (nvrec_entry_find(config, section, key) != NULL);
}

int nvrec_config_get_int(const nvrec_config_t *config, const char *section, const char *key, int def_value) {
  nvrec_entry_t *entry = NULL;
  char *endptr = NULL;
  int ret = 0;
  assert(config != NULL);
  assert(section != NULL);
  assert(key != NULL);

  entry = nvrec_entry_find(config, section, key);
  if (!entry)
    return def_value;

  ret = strtol(entry->value, &endptr, 0);
  return (*endptr == '\0') ? ret : def_value;
}

bool nvrec_config_get_bool(const nvrec_config_t *config, const char *section, const char *key, bool def_value) {
  nvrec_entry_t *entry = NULL;
  assert(config != NULL);
  assert(section != NULL);
  assert(key != NULL);

  entry = nvrec_entry_find(config, section, key);
  if (!entry)
    return def_value;

  if (!strcmp(entry->value, "true"))
    return true;
  if (!strcmp(entry->value, "false"))
    return false;

  return def_value;
}

const char *nvrec_config_get_string(const nvrec_config_t *config, const char *section, const char *key, const char *def_value) {
  nvrec_entry_t *entry = NULL;

  assert(config != NULL);
  assert(section != NULL);
  assert(key != NULL);



  entry = nvrec_entry_find(config, section, key);

  if (!entry)
    return def_value;

  return entry->value;
}

void nvrec_config_set_int(nvrec_config_t *config, const char *section, const char *key, int value) {

  char value_str[32] = { 0 };
  assert(config != NULL);
  assert(section != NULL);
  assert(key != NULL);

  sprintf(value_str, "%d", value);
  nvrec_config_set_string(config, section, key, value_str);
}

void nvrec_config_set_bool(nvrec_config_t *config, const char *section, const char *key, bool value) {
  assert(config != NULL);
  assert(section != NULL);
  assert(key != NULL);

  nvrec_config_set_string(config, section, key, value ? "true" : "false");
}

void *strdup_ext(const char *value)
{
    void *ptr = NULL;

    if(NULL != value)
    {
        size_t malloclen = (strlen(value)+1);
        ptr = zmalloc_ext(malloclen);
        memcpy(ptr,value,strlen(value));
    }
    return ptr;
}

static char *bdaddrdup(const char * string,unsigned int len)
{
    char *memory;

    if (!string)
        return(NULL);
    memory = cfg_zmalloc(len+1);
    if (NULL != memory)
    {
        memcpy(memory,string,len);
        return memory;
    }
    return(NULL);
}

void nvrec_config_set_string(nvrec_config_t *config, const char *section, const char *key, const char *value) {
  nvrec_entry_t *entry = NULL;
  list_node_t *node = NULL;
  nvrec_section_t *sec = nvrec_section_find(config, section);
  if (!sec) {
    sec = nvrec_section_new(section);
    list_append_ext(config->sections, sec);
  }
  if(0 == strcmp(section,section_name_ddbrec))
  {
      for (node = list_begin_ext(sec->entries); node != list_end_ext(sec->entries); node = list_next_ext(node))
      {
          entry = list_node_ext(node);
          if (0 == memcmp(entry->key, key, BTIF_BD_ADDR_SIZE))
          {
              cfg_free(entry->value);
              entry->value = (char *)strdup_ext(value);
              return;
          }
      }
  }
  else
  {
      for (node = list_begin_ext(sec->entries); node != list_end_ext(sec->entries); node = list_next_ext(node))
      {
        entry = list_node_ext(node);
        if (!strcmp(entry->key, key))
        {
            cfg_free(entry->value);
            entry->value = (char *)strdup_ext(value);
            return;
        }
    }
  }
  entry = nvrec_entry_new(key,value,section);
  list_append_ext(sec->entries, entry);
}

static nvrec_section_t *nvrec_section_new(const char *name) {
 //   printf("nvrec_section_new-->cfg_zmalloc..........................\r\n");
  nvrec_section_t *section = cfg_zmalloc(sizeof(nvrec_section_t));
  if (!section)
    return NULL;

  section->name = strdup_ext(name);
  section->entries = list_new_ext(nvrec_entry_free);
  return section;
}

static void nvrec_section_free(void *ptr) {
  nvrec_section_t *section = ptr;
  if (!ptr)
    return;
//    printf("nvrec_section_free-->cfg_free..........................\r\n");
  cfg_free(section->name);
  list_free_ext(section->entries);
  cfg_free(section);
}

nvrec_section_t *nvrec_section_find(const nvrec_config_t *config, const char *section) {
  nvrec_section_t *sec = NULL;
  list_node_t *node = NULL;
  for (node = list_begin_ext(config->sections); node != list_end_ext(config->sections); node = list_next_ext(node)) {
    sec = list_node_ext(node);
    if (!strcmp(sec->name, section))
      return sec;
  }

  return NULL;
}

static nvrec_entry_t *nvrec_entry_new(const char *key,const char *value,const char *descrip)
{
    nvrec_entry_t *entry = cfg_zmalloc(sizeof(nvrec_entry_t));
    if (!entry)
        return NULL;
    if(0 == strcmp(descrip,section_name_ddbrec))
        entry->key = bdaddrdup(key,BTIF_BD_ADDR_SIZE);
    else
        entry->key = strdup_ext(key);
    entry->value = strdup_ext(value);
    return entry;
}

void nvrec_entry_free(void *ptr) {
  nvrec_entry_t *entry = ptr;
  if (!ptr)
    return;
//  printf("entry_free-->cfg_free..........................\r\n");
  cfg_free(entry->key);
  cfg_free(entry->value);
  cfg_free(entry);
}

static nvrec_entry_t *nvrec_entry_find(const nvrec_config_t *config, const char *section, const char *key) {
  list_node_t *node = NULL;
  nvrec_section_t *sec = nvrec_section_find(config, section);
  if (!sec)
    return NULL;
  if(0 == strcmp(section,section_name_ddbrec))
  {
    for (node = list_begin_ext(sec->entries); node != list_end_ext(sec->entries); node = list_next_ext(node))
    {
      nvrec_entry_t *entry = list_node_ext(node);
      if (0 == memcmp((const void*)entry->key, (const void*)key, BTIF_BD_ADDR_SIZE))
        return entry;
    }
    return NULL;
  }
  else
  {
    for (node = list_begin_ext(sec->entries); node != list_end_ext(sec->entries); node = list_next_ext(node)) {
      nvrec_entry_t *entry = list_node_ext(node);
      if (!strcmp(entry->key, key))
        return entry;
    }

    return NULL;
  }
}
#if 0
int nevrec_entry_remove(const config_t *config, const char *section, const char *key) {
  list_node_t *node;
  section_t *sec = section_find(config, section);
  if (!sec)
    return NULL;
  for (node = list_begin_ext(sec->entries); node != list_end_ext(sec->entries); node = list_next_ext(node)) {
    entry_t *entry = list_node_ext(node);
    if (!strcmp(entry->key, key))
      list_remove_ext(sec->entries, entry);
      return 0;
  }
  return 0;
}
#endif

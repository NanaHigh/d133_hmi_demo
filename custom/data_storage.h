/*
 * Copyright (C) 2025 ArtInChip Technology Co., Ltd.
 *
 * Data Storage Module Header
 */

#ifndef __DATA_STORAGE_H__
#define __DATA_STORAGE_H__

#include <rtthread.h>

#define DATA_NAME "/sdcard/data.txt"
#define DATA_LEN 20

extern uint32_t my_data[DATA_LEN];

int write_to_sdcard(void);
int read_to_sdcard(void);

#endif /* __DATA_STORAGE_H__ */

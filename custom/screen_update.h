/*
 * Copyright (C) 2025 ArtInChip Technology Co., Ltd.
 *
 * Screen Update Module Header
 */

#ifndef __SCREEN_UPDATE_H__
#define __SCREEN_UPDATE_H__

#include <rtthread.h>

#define BUF_SIZE 256

extern float speed;
extern uint8_t buf[BUF_SIZE];

void update_screen(void);

#endif /* __SCREEN_UPDATE_H__ */

/*
 * Copyright (C) 2025 ArtInChip Technology Co., Ltd.
 *
 * Serial Communication Module Header
 */

#ifndef __SERIAL_COMM_H__
#define __SERIAL_COMM_H__

#include <rtthread.h>
#include <stdbool.h>

#define UART_NAME "uart0"
#define BUF_SIZE 256

typedef struct {
    uint32_t value;
} uart_msg_t;

extern bool data_valid;

int uart_init(void);
void parse_data(void);
void uart_msg_poll(void);

#endif /* __SERIAL_COMM_H__ */

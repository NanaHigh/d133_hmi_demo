/*
 * Copyright (C) 2025 ArtInChip Technology Co., Ltd.
 *
 * Serial Communication Module Implementation
 */

#include "serial_comm.h"
#include "data_storage.h"
#include "screen_update.h"
#include "custom.h"
#include <rtthread.h>
#include <rtdevice.h>

#define UART_NAME "uart0"
#define BUF_SIZE 256

bool data_valid = false;

static rt_device_t serial = RT_NULL;
static struct rt_semaphore rx_sem;
static uint8_t rx_buf[BUF_SIZE]= {0};
static rt_size_t buf_index = 0;
static struct rt_messagequeue uart_msg_queue;
static uint8_t uart_msg_pool[sizeof(uart_msg_t) * 4];

extern ui_manager_t ui_manager;

static rt_err_t uart_rx_ind(rt_device_t dev, rt_size_t size) {
    rt_sem_release(&rx_sem);
    return RT_EOK;
}

void parse_data() {
    if ((rx_buf[0] == 0xAA)&&(rx_buf[1] == 0xBB)) {
        uint32_t data = rx_buf[2] | (rx_buf[3] << 8);
        uart_msg_t msg;
        msg.value = data > 10000 ? 10000 : data;
        rt_mq_send(&uart_msg_queue, &msg, sizeof(msg));
        buf_index = 0;
        write_to_sdcard();
    }
}

void uart_msg_poll(void)
{
    uart_msg_t msg;
    if (lv_scr_act() != ui_manager.screen.obj) {
        while (rt_mq_recv(&uart_msg_queue, &msg, sizeof(msg), 0) == RT_EOK) {
        }
        return;
    }
    while (rt_mq_recv(&uart_msg_queue, &msg, sizeof(msg), 0) == RT_EOK) {
        my_data[0] = msg.value;
        lv_spinbox_set_value(ui_manager.screen.spinbox_2, my_data[0]);
        update_screen();
    }
}

static void serial_thread_entry(void *parameter) {
    char ch;

    while (1) {
        rt_sem_take(&rx_sem, RT_WAITING_FOREVER);
        while (rt_device_read(serial, 0, &ch, 1) == 1) {
            if (buf_index >= BUF_SIZE - 1) {
                rt_kprintf("[WARN] Buffer overflow! Resetting buffer.\n");
                buf_index = 0;
                memset(rx_buf, 0, sizeof(rx_buf));
            }
            rx_buf[buf_index++] = ch;
        }
        parse_data();
        memset(rx_buf, 0, sizeof(rx_buf));
    }
}

int uart_init(void) {
    serial = rt_device_find(UART_NAME);
    if (!serial) {
        rt_kprintf("%s device not found!\n", UART_NAME);
        return -RT_ERROR;
    }

    rt_sem_init(&rx_sem, "rx_sem", 0, RT_IPC_FLAG_FIFO);
    rt_mq_init(&uart_msg_queue, "uart_mq", uart_msg_pool, sizeof(uart_msg_t), sizeof(uart_msg_pool), RT_IPC_FLAG_FIFO);

    rt_device_open(serial, RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_DMA_TX | RT_DEVICE_FLAG_STREAM);
    
    rt_device_set_rx_indicate(serial, uart_rx_ind);
    
    rt_thread_t thread = rt_thread_create(
        "serial_rx", 
        serial_thread_entry, 
        RT_NULL, 
        4096, 
        25, 
        10
    );
    rt_thread_startup(thread);
    
    return RT_EOK;
}

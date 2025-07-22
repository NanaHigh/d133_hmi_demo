/*
 * Copyright (C) 2025 ArtInChip Technology Co., Ltd.
 *
 * Data Storage Module Implementation
 */

#include "data_storage.h"
#include <rtthread.h>
#include <unistd.h>
#include <fcntl.h>

#define DATA_NAME "/sdcard/data.txt"
#define DATA_LEN 20

uint32_t my_data[DATA_LEN] = {0};

int write_to_sdcard(void)
{
    int fd;
    ssize_t bytes_written;
    
    fd = open(DATA_NAME, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0666);
    if (fd < 0) {
        rt_kprintf("Failed to open file %s for write, error: %d\n", DATA_NAME, rt_get_errno());
        return -1;
    }

    bytes_written = write(fd, my_data, sizeof(my_data));
    close(fd);
    
    if (bytes_written != sizeof(my_data)) {
        rt_kprintf("Write incomplete data: %d/%d bytes\n", bytes_written, sizeof(my_data));
        return -1;
    }
    
    return 0;
}

int read_to_sdcard(void)
{
    int fd;
    ssize_t bytes_read;
    
    if (access(DATA_NAME, 0) != 0) {
        rt_kprintf("File not exist, create new\n");
        memset(my_data, 0, sizeof(my_data));
        return write_to_sdcard();
    }
    
    fd = open(DATA_NAME, O_RDONLY | O_BINARY, 0);
    if (fd < 0) {
        rt_kprintf("Failed to open file %s for read, error: %d\n", DATA_NAME, rt_get_errno());
        return -1;
    }

    bytes_read = read(fd, my_data, sizeof(my_data));
    close(fd);
    
    if (bytes_read != sizeof(my_data)) {
        rt_kprintf("Read incomplete data: %d/%d bytes\n", bytes_read, sizeof(my_data));
        return -1;
    }
    
    return 0;
}

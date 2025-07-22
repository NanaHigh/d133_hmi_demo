/*
 * Copyright (C) 2025 ArtInChip Technology Co., Ltd.
 *
 * Screen Update Module Implementation
 */

#include "screen_update.h"
#include "data_storage.h"
#include "custom.h"

#define BUF_SIZE 256

float speed = 10.00f;
uint8_t buf[BUF_SIZE] = {0};

extern ui_manager_t ui_manager;

void update_screen() {
    sprintf(buf, "Speed: %.2f", speed + my_data[0]/100.0);
    lv_label_set_text(ui_manager.screen.label_1, (const char*)buf);
    memset(buf, 0, sizeof(buf));
}

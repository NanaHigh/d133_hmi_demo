/*
 * Copyright (C) 2025 ArtInChip Technology Co., Ltd.
 *
 * Audio Player Module Header
 */

#ifndef __AUDIO_PLAYER_H__
#define __AUDIO_PLAYER_H__

#include <rtthread.h>

#define SOUND_NAME "sound0"
#define CLICK_WAV "/sdcard/Clicked.wav"
#define MYGO_WAV "/sdcard/MyGo.wav"
#define WAV_BUF_SIZE 4096

typedef struct {
    char riff[4];
    uint32_t size;
    char wave[4];
    char fmt[4];
    uint32_t fmt_size;
    uint16_t format_tag;
    uint16_t channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char data[4];
    uint32_t data_size;
} wav_header_t;

typedef struct {
    char wav_path[128];
} wav_play_param_t;

void play_wav_async(const char *wav_path);

#endif /* __AUDIO_PLAYER_H__ */

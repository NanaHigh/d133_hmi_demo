/*
 * Copyright (C) 2025 ArtInChip Technology Co., Ltd.
 *
 * Audio Player Module Implementation
 */

#include "audio_player.h"
#include <rtthread.h>
#include <rtdevice.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

// Audio processing parameters to prevent distortion
#define VOLUME_SCALE_FACTOR 0.8f    // Volume scaling factor to reduce clipping
#define HIGH_FREQ_ATTENUATION 0.7f  // High frequency attenuation factor
#define SOFT_LIMIT_THRESHOLD 28000  // Soft limiting threshold for 16-bit audio

static rt_device_t snd_handle = RT_NULL;
static rt_thread_t wav_thread = RT_NULL;
static wav_play_param_t wav_param;
static volatile bool wav_stop_flag = false;

// Audio data processing function to prevent distortion
static void process_audio_data(uint8_t *buf, rt_size_t size, uint16_t bits_per_sample, uint16_t channels)
{
    if (bits_per_sample == 16) {
        int16_t *samples = (int16_t*)buf;
        rt_size_t sample_count = size / sizeof(int16_t);
        
        for (rt_size_t i = 0; i < sample_count; i++) {
            int32_t sample = samples[i];
            
            // 1. Volume control - prevent overload
            sample = (int32_t)(sample * VOLUME_SCALE_FACTOR);
            
            // 2. Soft limiting - prevent hard clipping
            if (sample > SOFT_LIMIT_THRESHOLD) {
                sample = SOFT_LIMIT_THRESHOLD + (sample - SOFT_LIMIT_THRESHOLD) * 0.1f;
            } else if (sample < -SOFT_LIMIT_THRESHOLD) {
                sample = -SOFT_LIMIT_THRESHOLD + (sample + SOFT_LIMIT_THRESHOLD) * 0.1f;
            }
            
            // 3. Simple high-frequency filtering (attenuate every other sample with large changes)
            if (i > 0 && (i % 2 == 0)) {
                int32_t prev_sample = samples[i-1];
                int32_t diff = sample - prev_sample;
                if (abs(diff) > 8000) {  // Detect high frequency changes
                    sample = prev_sample + diff * HIGH_FREQ_ATTENUATION;
                }
            }
            
            // 4. Ensure within valid range
            if (sample > 32767) sample = 32767;
            if (sample < -32768) sample = -32768;
            
            samples[i] = (int16_t)sample;
        }
    } else if (bits_per_sample == 8) {
        // 8-bit audio processing
        for (rt_size_t i = 0; i < size; i++) {
            int16_t sample = (int16_t)buf[i] - 128; // Convert to signed
            sample = (int16_t)(sample * VOLUME_SCALE_FACTOR);
            
            // Soft limiting for 8-bit audio
            if (sample > 100) {
                sample = 100 + (sample - 100) * 0.1f;
            } else if (sample < -100) {
                sample = -100 + (sample + 100) * 0.1f;
            }
            
            buf[i] = (uint8_t)(sample + 128); // Convert back to unsigned
        }
    }
}

static void wav_play_thread_entry(void *parameter)
{
    char *path = ((wav_play_param_t*)parameter)->wav_path;
    wav_stop_flag = false;
    int fd = -1;
    wav_header_t header;
    rt_size_t read_len;
    uint8_t *buf = RT_NULL;
    int ret = -1;

    if (snd_handle) {
        rt_device_close(snd_handle);
        snd_handle = RT_NULL;
    }

    fd = open(path, O_RDONLY | O_BINARY, 0);
    if (fd < 0) goto __exit;

    if (read(fd, &header, sizeof(header)) != sizeof(header)) goto __exit;

    if (memcmp(header.riff, "RIFF", 4) != 0 || memcmp(header.wave, "WAVE", 4) != 0) goto __exit;

    // Seek to "data" chunk in case there are extra chunks
    if (memcmp(header.data, "data", 4) != 0) {
        char chunk_id[4];
        uint32_t chunk_size;
        lseek(fd, sizeof(header.riff) + sizeof(header.size) + sizeof(header.wave), SEEK_SET);
        while (1) {
            if (read(fd, chunk_id, 4) != 4) goto __exit;
            if (read(fd, &chunk_size, 4) != 4) goto __exit;
            if (memcmp(chunk_id, "data", 4) == 0) {
                header.data_size = chunk_size;
                break;
            }
            lseek(fd, chunk_size, SEEK_CUR);
        }
    }

    snd_handle = rt_device_find(SOUND_NAME);
    if (!snd_handle) goto __exit;

    if (rt_device_open(snd_handle, RT_DEVICE_OFLAG_WRONLY | RT_DEVICE_FLAG_DMA_TX) != RT_EOK) {
        snd_handle = RT_NULL;
        goto __exit;
    }

    struct rt_audio_caps caps;
    caps.main_type = AUDIO_TYPE_OUTPUT;
    caps.sub_type  = AUDIO_DSP_PARAM;
    caps.udata.config.samplerate = header.sample_rate;
    caps.udata.config.channels   = header.channels;
    caps.udata.config.samplebits = header.bits_per_sample;
    rt_device_control(snd_handle, AUDIO_CTL_CONFIGURE, &caps);

    // Set hardware volume to prevent overload (if supported)
    caps.main_type = AUDIO_TYPE_MIXER;
    caps.sub_type = AUDIO_MIXER_VOLUME;
    caps.udata.value = 80; // Set to 80% volume to prevent overload
    rt_device_control(snd_handle, AUDIO_CTL_CONFIGURE, &caps);

    buf = rt_malloc(WAV_BUF_SIZE);
    if (!buf) goto __exit;

    uint32_t left = header.data_size;
    while (left > 0 && !wav_stop_flag) {
        read_len = read(fd, buf, left > WAV_BUF_SIZE ? WAV_BUF_SIZE : left);
        if (read_len <= 0) break;
        
        // Process audio data to prevent distortion
        process_audio_data(buf, read_len, header.bits_per_sample, header.channels);
        
        rt_device_write(snd_handle, 0, buf, read_len);
        left -= read_len;
        
        // Add small delay to allow audio buffer processing
        rt_thread_mdelay(1);
    }

__exit:
    if (buf) rt_free(buf);
    if (snd_handle) {
        rt_device_close(snd_handle);
        snd_handle = RT_NULL;
    }
    if (fd >= 0) close(fd);
    wav_thread = RT_NULL;
}

void play_wav_async(const char *wav_path)
{
    if (wav_thread != RT_NULL) {
        wav_stop_flag = true;
        while (wav_thread != RT_NULL) {
            rt_thread_mdelay(5);
        }
    }
    rt_strncpy(wav_param.wav_path, wav_path, sizeof(wav_param.wav_path) - 1);
    wav_param.wav_path[sizeof(wav_param.wav_path) - 1] = '\0';
    wav_thread = rt_thread_create("wavplay", wav_play_thread_entry, &wav_param, 4096, 20, 10);
    if (wav_thread)
        rt_thread_startup(wav_thread);
}

#include "audio.h"
#include "sounds.h"
#include "bsp/audio.h"
#include "driver/i2s_std.h"
#include <string.h>
#include <math.h>

// External BSP audio function (not in public header)
extern void bsp_audio_initialize(uint32_t rate);

// Sound trigger flags
volatile bool audio_sound_trigger[5] = { false, false, false, false, false };

// Active sound tracking
typedef struct {
    const int16_t* sample_data;
    uint32_t sample_length;
    uint32_t playback_position;
    bool active;
    float volume;
} active_sound_t;

static i2s_chan_handle_t i2s_handle = NULL;
static active_sound_t active_sounds[AUDIO_MAX_SOUNDS];

// Sound sample table
static const int16_t* sound_samples[5];
static uint32_t sound_lengths[5];

static void audio_task(void* arg) {
    int16_t output_buffer[AUDIO_FRAMES_PER_WRITE * 2];
    size_t bytes_written;

    while (1) {
        // Check for new sound triggers
        for (int i = 0; i < 5; i++) {
            if (audio_sound_trigger[i]) {
                for (int slot = 0; slot < AUDIO_MAX_SOUNDS; slot++) {
                    if (!active_sounds[slot].active ||
                        active_sounds[slot].sample_data == sound_samples[i]) {
                        active_sounds[slot].sample_data = sound_samples[i];
                        active_sounds[slot].sample_length = sound_lengths[i];
                        active_sounds[slot].playback_position = 0;
                        active_sounds[slot].volume = 0.15f;
                        active_sounds[slot].active = true;
                        break;
                    }
                }
                audio_sound_trigger[i] = false;
            }
        }

        // Mix all active sounds
        for (int frame = 0; frame < AUDIO_FRAMES_PER_WRITE; frame++) {
            float mix = 0.0f;

            for (int i = 0; i < AUDIO_MAX_SOUNDS; i++) {
                if (active_sounds[i].active) {
                    int16_t sample = active_sounds[i].sample_data[
                        active_sounds[i].playback_position
                    ];
                    mix += (sample / 32768.0f) * active_sounds[i].volume;
                    active_sounds[i].playback_position++;
                    if (active_sounds[i].playback_position >= active_sounds[i].sample_length) {
                        active_sounds[i].active = false;
                    }
                }
            }

            // Soft clip
            mix = fminf(1.0f, fmaxf(-1.0f, mix));

            int16_t out = (int16_t)(mix * 32767.0f);
            output_buffer[frame * 2] = out;
            output_buffer[frame * 2 + 1] = out;
        }

        // Write to I2S
        if (i2s_handle != NULL) {
            i2s_channel_write(i2s_handle, output_buffer,
                              sizeof(output_buffer), &bytes_written, portMAX_DELAY);
        }
    }
}

void audio_init(void) {
    // Set up sample pointers
    sound_samples[0] = sound_shoot;
    sound_lengths[0] = sound_shoot_len;
    sound_samples[1] = sound_alien_explode;
    sound_lengths[1] = sound_alien_explode_len;
    sound_samples[2] = sound_player_death;
    sound_lengths[2] = sound_player_death_len;
    sound_samples[3] = sound_mystery_appear;
    sound_lengths[3] = sound_mystery_appear_len;
    sound_samples[4] = sound_mystery_hit;
    sound_lengths[4] = sound_mystery_hit_len;

    memset(active_sounds, 0, sizeof(active_sounds));

    // Initialize BSP audio
    bsp_audio_initialize(AUDIO_SAMPLE_RATE);
    bsp_audio_get_i2s_handle(&i2s_handle);
    bsp_audio_set_amplifier(true);
    bsp_audio_set_volume(100);

    // Create audio mixing task on Core 1
    xTaskCreatePinnedToCore(
        audio_task, "audio", 4096, NULL,
        configMAX_PRIORITIES - 2, NULL, 1
    );
}

void audio_trigger(int sound_id) {
    if (sound_id >= 0 && sound_id < 5) {
        audio_sound_trigger[sound_id] = true;
    }
}

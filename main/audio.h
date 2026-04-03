#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>
#include <stdbool.h>

#define AUDIO_SAMPLE_RATE   44100
#define AUDIO_MAX_SOUNDS    5
#define AUDIO_FRAMES_PER_WRITE 64

// Initialize the audio subsystem and start the mixing task on Core 1
void audio_init(void);

// Trigger a sound effect (called from game loop on Core 0)
void audio_trigger(int sound_id);

// Sound trigger flags (set by game, cleared by audio task)
extern volatile bool audio_sound_trigger[5];

#endif // AUDIO_H

#ifndef AUDIO_FEATURES_H
#define AUDIO_FEATURES_H

#include <stdint.h>

void compute_features(float *frame, float *feature_buffer, uint32_t mel_frame_index);

#endif

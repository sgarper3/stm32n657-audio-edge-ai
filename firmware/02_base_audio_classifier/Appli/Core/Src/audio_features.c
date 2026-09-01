#include "audio_features.h"
#include "mel_filters_sparse.h"

#include "arm_math.h"

#include <math.h>

#define FRAME_SIZE   1024
#define FFT_BINS     513
#define MEL_BINS     64
#define TIME_FRAMES  50

static arm_rfft_fast_instance_f32 fft_instance;
static uint8_t fft_initialized = 0;

void compute_features(float *frame,
                      float *feature_buffer,
                      uint32_t mel_frame_index)
{
    static float fft_out[FRAME_SIZE];
    static float power_spectrum[FFT_BINS];

    if (!fft_initialized)
    {
        arm_rfft_fast_init_f32(&fft_instance, FRAME_SIZE);
        fft_initialized = 1;
    }

    arm_rfft_fast_f32(&fft_instance, frame, fft_out, 0);

    /* DC component. */
    power_spectrum[0] = fft_out[0] * fft_out[0];

    /* Positive-frequency FFT bins. */
    for (int i = 1; i < FFT_BINS - 1; i++)
    {
        float re = fft_out[2 * i];
        float im = fft_out[2 * i + 1];

        power_spectrum[i] = re * re + im * im;
    }

    /* Nyquist component. */
    power_spectrum[FFT_BINS - 1] = fft_out[1] * fft_out[1];

    uint32_t coeff_offset = 0;

    for (int m = 0; m < MEL_BINS; m++)
    {
        float sum = 0.0f;

        uint16_t start = mel_start[m];
        uint16_t len = mel_len[m];

        for (int j = 0; j < len; j++)
        {
            uint16_t k = start + j;
            sum += mel_coeffs[coeff_offset + j] * power_spectrum[k];
        }

        coeff_offset += len;

        if (!isfinite(sum) || sum < 1.0e-10f)
        {
            sum = 1.0e-10f;
        }

        /*
        * Store Mel-band power. Conversion to dB is performed globally
        * after all time frames have been computed.
        */
        feature_buffer[m * TIME_FRAMES + mel_frame_index] = sum;
    }
}
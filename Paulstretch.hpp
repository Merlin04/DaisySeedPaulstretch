#pragma once

#include "daisy_seed.h"
#include "daisysp.h"
#include "arm_math.h"

#include "consts.h"

typedef struct sp_auxdata {
    size_t size;
    void* ptr;
} sp_auxdata;

int sp_auxdata_alloc(sp_auxdata* aux, size_t size) {
    aux->ptr = malloc(size);
    if(aux->ptr == nullptr) {
        return -1;
    }

    aux->size = size;
    memset(aux->ptr, 0, size);
    return 1;
}

int sp_auxdata_free(sp_auxdata* aux) {
    free(aux->ptr);
    return 1;
}

class SPRand {
private:
    inline static uint32_t r = 0;
public:
    static constexpr uint32_t max = 2147483648;
    static uint32_t rand() {
        uint32_t val = (1103515245 * r + 12345) % max;
        r = val;
        return val;
    }
};

#define CAT_I(a,b) a##b
#define CAT(a,b) CAT_I(a, b)

#define windowsize 128
#define ARM_RFFT_FAST_INIT_F32 CAT(CAT(arm_rfft_fast_init_, windowsize), _f32)

// represents one Paulstretch computation on some audio input
class Paulstretch {
    // static constants
public:
    static constexpr size_t half_windowsize = windowsize / 2;
private:

    inline static const float* window = [] {
        sp_auxdata m_window;
        sp_auxdata_alloc(&m_window, windowsize * sizeof(float));
        auto* window = static_cast<float*>(m_window.ptr);

        // create Hann window
        for(size_t i = 0; i < half_windowsize; i++) {
            window[i] = static_cast<float>(0.5 - cos(i * 2.0 * M_PI / (windowsize - 1)) * 0.5);
        }

        return window;
    }();

    inline static const float* hinv_buf = [] {
        sp_auxdata m_hinv_buf;
        sp_auxdata_alloc(&m_hinv_buf, half_windowsize * sizeof(float));
        auto* hinv_buf = static_cast<float*>(m_hinv_buf.ptr);

        // create inverse Hann window
        auto hinv_sqrt2 = static_cast<float>((1 + sqrt(0.5)) * 0.5);
        for(size_t i = 0; i < half_windowsize; i++) {
            hinv_buf[i] = static_cast<float>(hinv_sqrt2 - (1.0 - hinv_sqrt2) * cos(i * 2.0 * M_PI / half_windowsize));
        }

        return hinv_buf;
    }();

    // instance variables
    float displace_pos_amt;
    bool wrap = true;

    sp_auxdata m_old_windowed_buf;
    float* old_windowed_buf;

    sp_auxdata m_buf;
    float* buf;

    sp_auxdata m_output; // block out

    arm_rfft_fast_instance_f32 fft, ifft;

public:
    float* output;

    float* input;
    size_t input_size;

    float start_pos = 0.0;

public:
    explicit Paulstretch(float stretch) {
        // compute the displacement inside the input file
        set_stretch(stretch);

        // allocate buffers
        sp_auxdata_alloc(&m_old_windowed_buf, windowsize * sizeof(float));
        old_windowed_buf = static_cast<float*>(m_old_windowed_buf.ptr);

        sp_auxdata_alloc(&m_buf, windowsize * sizeof(float));
        buf = static_cast<float*>(m_buf.ptr);

        sp_auxdata_alloc(&m_output, half_windowsize * sizeof(float));
        output = static_cast<float*>(m_output.ptr);

        // setup FFT
        ARM_RFFT_FAST_INIT_F32(&fft);
        ARM_RFFT_FAST_INIT_F32(&ifft);
    }

    ~Paulstretch() {
        sp_auxdata_free(&m_old_windowed_buf);
        sp_auxdata_free(&m_buf);
        sp_auxdata_free(&m_output);
    }

    void set_stretch(float stretch) {
        displace_pos_amt = static_cast<float>(windowsize * 0.5) / stretch;
    }

    void reset() {
        start_pos = 0.0;
        memset(old_windowed_buf, 0, windowsize * sizeof(float));
        memset(buf, 0, windowsize * sizeof(float));
        memset(output, 0, half_windowsize * sizeof(float));
    }

    // process one block (windowsize samples) of audio
    // you can gradually fill the input/adjust input_size correspondingly,
    // this will only read samples input[:start_pos + windowsize] (one window's worth,
    // and possibly some stuff at the start to wrap around)
    // so as long as we're filling the input from the start to the end, we'll be fine :)
    void compute_block() {
        // get the windowed buffer
        uint32_t istart_pos = floor(start_pos);

        uint32_t i = 0;
        for(; i < windowsize; i++) {
            uint32_t pos = istart_pos + i;

            if(wrap) {
                pos %= input_size;
            }

            if(pos < input_size) {
                buf[i] = input[pos] * window[i];
            } else {
                buf[i] = 0.0f;
            }
        }

        float fft_tmp[windowsize];
        arm_rfft_fast_f32(&fft, buf, fft_tmp, 0);

        // randomize phase

        // output format is a bit weird
        // the first complex number outputted from the rfft always
        // is just a real with no imaginary component! and the same holds
        // for the last one (called the Nyquist frequency). so, the real
        // part of the complex number corresponding to the Nyquist freq
        // is just stored as the imaginary part of the first complex number

        // handle the first two reals separately
        for(i = 0; i < 2; i++) {
            auto mag = fft_tmp[i];
            auto ph = static_cast<float>(((double)SPRand::rand() / (double)SPRand::max) * 2.0 * M_PI);
            fft_tmp[i] = mag * arm_cos_f32(ph);
        }

        for(; i < windowsize; i += 2) {
            auto mag = static_cast<float>(sqrt(pow(fft_tmp[i], 2) + pow(fft_tmp[i + 1], 2)));
            auto phase = static_cast<float>(((double)SPRand::rand() / (double)SPRand::max) * 2.0 * M_PI);
            float s, c;
            arm_sin_cos_f32(phase, &s, &c);
            fft_tmp[i] = mag * c; // real part
            fft_tmp[i + 1] = mag * s; // imaginary part
        }

        arm_rfft_fast_f32(&ifft, fft_tmp, buf, 1);

        // apply window and overlap
        for(i = 0; i < windowsize; i++) {
            // window again
            buf[i] *= window[i];
            if(i < half_windowsize) {
                // overlap-add the output
                output[i] = static_cast<float>(buf[i] + old_windowed_buf[half_windowsize + i]) /*/ windowsize*/;
                // remove the resulted amplitude modulation
                output[i] *= hinv_buf[i];
            }
            old_windowed_buf[i] = buf[i];
        }

        start_pos += displace_pos_amt;
    }
};
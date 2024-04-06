#include "daisy_seed.h"
#include "daisysp.h"
#include "Paulstretch.hpp"
#include "consts.h"

#include "hardware/Pedal.hpp"

using namespace daisy;
using namespace daisysp;

DaisySeed hw;
Pedal pedal(hw);

float DSY_SDRAM_BSS loop_rec_buffer[sample_rate * 16]; // 16 seconds of audio
float DSY_SDRAM_BSS stretched_buffer[sample_rate * 16 * 16]; // 16 seconds stretched up to x16 (about 12 MB)

bool is_recording = false;
size_t rec_pos = 0;
size_t stretched_read_pos = 0; // position to read a block from in the loop_rec_buffer
size_t stretched_write_pos = 0; // position to write to in the stretched_buffer

constexpr float stretch = 0.25;

Paulstretch paulstretch(stretch);

void stop_recording() {
    is_recording = false;
    pedal.SetLed(1, 0.0);
}

void AudioCallback(const AudioHandle::InputBuffer in, const AudioHandle::OutputBuffer out, const size_t size) {
    pedal.ProcessAllControls();

    if(pedal.switches[0].RisingEdge()) {
        pedal.SetAudioBypass(!pedal.audioBypass);
        pedal.SetLed(0, pedal.audioBypass ? 0.0 : 1.0);
    }

    if(pedal.switches[1].RisingEdge()) {
        is_recording = !is_recording;
        pedal.SetLed(1, is_recording ? 1.0 : 0.0);

        // clear recording buffer (for now, no loop overs)
        if(is_recording) {
            memset(loop_rec_buffer, 0, sizeof(loop_rec_buffer));
            rec_pos = 0;
            stretched_read_pos = 0;
            stretched_write_pos = 0;
            paulstretch.reset();
            paulstretch.input_size = 0;
        } else {
            stop_recording();
        }
    }

    if(is_recording) {
        memcpy(loop_rec_buffer + rec_pos, in[0], std::max(size * sizeof(float), sizeof(loop_rec_buffer) - rec_pos));
        rec_pos += size * sizeof(float);

        if(rec_pos >= sizeof(loop_rec_buffer)) {
            // stop recording
            rec_pos = sizeof(loop_rec_buffer);
            stop_recording();
        }
    }

	for (size_t i = 0; i < size; i++) {
		out[0][i] = in[0][i];
		// out[1][i] = in[1][i]; (no stereo)
	}

    pedal.UpdateLeds();
}

int main() {
    pedal.Init();
//	hw.SetAudioBlockSize(4); // number of samples handled per callback
    hw.adc.Start();
	hw.StartAudio(AudioCallback);

    paulstretch.input = loop_rec_buffer;
    paulstretch.input_size = 0;

	while(true) {
        if(rec_pos >= windowsize && stretched_read_pos <= rec_pos - windowsize && stretched_write_pos < (sizeof(stretched_buffer) - (Paulstretch::half_windowsize * sizeof(float)))) {
            paulstretch.input_size = rec_pos;
            paulstretch.compute_block();

            // copy the output to the stretched buffer
            memcpy(stretched_buffer + stretched_write_pos, paulstretch.output, Paulstretch::half_windowsize * sizeof(float));

            stretched_read_pos = floor(paulstretch.start_pos);
            stretched_write_pos += Paulstretch::half_windowsize;
        }
    }
}

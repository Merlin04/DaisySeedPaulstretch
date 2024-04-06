#pragma once

#include <vector>
#include "daisy_seed.h"

using namespace daisy;

class BaseHardwareModule {
public:
    DaisySeed& seed;

    std::vector<AnalogControl> knobs;
    std::vector<Switch> switches;
    std::vector<Led> leds;

    GPIO audioBypassTrigger;
    GPIO audioMuteTrigger;

    bool audioBypass = true;
    bool audioMute = false;

    explicit BaseHardwareModule(DaisySeed& seed) : seed(seed) {}
    virtual ~BaseHardwareModule() = default;

    /** Initialize the pedal */
    virtual void Init(bool boost = false) {
        seed.Init(boost);
        SetAudioBlockSize(48);
    }

    /** Updates the Audio Sample Rate, and reinitializes.
     ** Audio must be stopped for this to work.
     */
    void SetAudioSampleRate(SaiHandle::Config::SampleRate samplerate) {
        seed.SetAudioSampleRate(samplerate);
        SetHidUpdateRates();
    }

    /** Sets the number of samples processed per channel by the audio callback.
       \param size Audio block size
     */
    void SetAudioBlockSize(size_t size) {
        seed.SetAudioBlockSize(size);
        SetHidUpdateRates();
    }

    /** Call at the same frequency as controls are read for stable readings.*/
    void ProcessAnalogControls() {
        for(auto &knob : knobs) {
            knob.Process();
        }
    }

    /** Process digital controls */
    void ProcessDigitalControls() {
        for(auto &sw : switches) {
            sw.Debounce();
        }
    }

    /** Process Analog and Digital Controls */
    inline void ProcessAllControls() {
        ProcessAnalogControls();
        ProcessDigitalControls();
    }

    /** Get Numbers of Samples for a specified amount of time in seconds
    \param time Specified time in seconds.
    \return int number of samples at the current sample rate.
    */
    int GetNumberOfSamplesForTime(float time) {
        return (int)(seed.AudioSampleRate() * time);
    }

    /** Get Time (in seconds) for a specific Numbers of Samples
    \param samples Specified number of samples.
    \return float number of seconds at the current sample rate.
    */
    float GetTimeForNumberOfSamples(int samples) {
        return (float)samples / seed.AudioSampleRate();
    }

    /** Get value per knob.
    \param knobID Which knob to get
    \return Floating point knob position.
    */
    float GetKnobValue(uint8_t knobID) {
        if(knobID < 0 || knobID >= knobs.size()) {
            return 0.0f;
        }
        return knobs[knobID].Value();
    }

    /**
       Set Led
       \param ledID Led Index
       \param bright Brightness
     */
    void SetLed(uint8_t ledID, float bright) {
        if(ledID < 0 || ledID >= leds.size()) {
            return;
        }
        leds[ledID].Set(bright);
    }

    /** Updates all the LEDs based on their values */
    void UpdateLeds() {
        for(auto &led : leds) {
            led.Update();
        }
    }

    /** Toggle the Hardware Audio Bypass (if applicable) */
    void SetAudioBypass(bool enabled) {
        audioBypass = enabled;
        audioBypassTrigger.Write(!enabled);
    }

    /** Toggle the Hardware Audio Mute (if applicable) */
    void SetAudioMute(bool enabled) {
        audioMute = enabled;
        audioMuteTrigger.Write(!enabled);
    }

protected:
    void SetHidUpdateRates() {
        for(auto &knob : knobs) {
            knob.SetSampleRate(seed.AudioCallbackRate());
        }
        for(auto &led : leds) {
            led.SetSampleRate(seed.AudioCallbackRate());
        }
    }

    void InitKnobs(uint8_t count, Pin pins[]) {
        AdcChannelConfig cfg[count];
        for(uint8_t i = 0; i < count; i++) {
            cfg[i].InitSingle(pins[i]);
        }
        seed.adc.Init(cfg, count);

        // knobs
        for(uint8_t i = 0; i < count; i++) {
            AnalogControl k;
            k.Init(seed.adc.GetPtr(i), seed.AudioCallbackRate());
            knobs.push_back(k);
        }
    }

    void InitSwitches(uint8_t count, Pin pins[]) {
        for(uint8_t i = 0; i < count; i++) {
            Switch s;
            s.Init(pins[i], seed.AudioCallbackRate());
            switches.push_back(s);
        }
    }

    void InitLeds(uint8_t count, Pin pins[]) {
        for(uint8_t i = 0; i < count; i++) {
            Led l;
            l.Init(pins[i], false, seed.AudioCallbackRate());
            leds.push_back(l);
        }
    }

    void InitTrueBypass(Pin relayPin, Pin mutePin) {
        audioBypassTrigger.Init(relayPin, GPIO::Mode::OUTPUT);
        SetAudioBypass(true);

        audioMuteTrigger.Init(mutePin, GPIO::Mode::OUTPUT);
        SetAudioMute(false);
    }
};

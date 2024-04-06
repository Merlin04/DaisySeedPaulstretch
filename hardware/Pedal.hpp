#pragma once

#include "daisy_seed.h"
#include "BaseHardwareModule.hpp"

using namespace daisy;

class Pedal : public BaseHardwareModule {
public:
    explicit Pedal(DaisySeed& seed) : BaseHardwareModule(seed) {}
    
    void Init(bool boost = false) override {
        BaseHardwareModule::Init(boost);

        Pin knobPins[] = {seed::D15, seed::D16, seed::D17, seed::D18};
        InitKnobs(4, knobPins);

        Pin switchPins[] = {seed::D6, seed::D5};
        InitSwitches(2, switchPins);

        Pin ledPins[] = {seed::D22, seed::D23};
        InitLeds(2, ledPins);

        InitTrueBypass(seed::D1, seed::D12);
    }
};
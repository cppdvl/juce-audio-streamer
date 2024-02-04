//
// Created by Julian Guarin on 1/02/24.
//

#ifndef AUDIOSTREAMPLUGIN_METER_H
#define AUDIOSTREAMPLUGIN_METER_H

#include <juce_audio_processors/juce_audio_processors.h>

namespace DAWn::GUI
{
    class Meter : public juce::Component
    {
        float mLevel = -60.0f;

    public:
        void paint(juce::Graphics &g) override;
        void setLevel(float newLevel);
    };
}

#endif //AUDIOSTREAMPLUGIN_METER_H

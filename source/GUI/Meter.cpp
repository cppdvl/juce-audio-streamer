//
// Created by Julian Guarin on 1/02/24.
//

#include "Meter.h"



void DAWn::GUI::Meter::paint(juce::Graphics &g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour (juce::Colours::white.withAlpha(0.5f));
    g.fillRoundedRectangle(bounds, 5.0f);

    g.setColour (juce::Colours::white);
    auto scaledY = juce::jmap(mLevel, -60.0f, 6.0f, 0.0f, static_cast<float>(getHeight()));
    g.fillRoundedRectangle(bounds.removeFromBottom(scaledY), 5.0f);
}

void DAWn::GUI::Meter::setLevel(float newLevel)
{
    mLevel = newLevel;
}


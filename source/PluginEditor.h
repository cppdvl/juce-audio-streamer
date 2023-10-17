#pragma once

#include "PluginProcessor.h"
#include "BinaryData.h"
#include "melatonin_inspector/melatonin_inspector.h"

struct AtomicLabel : juce::Component, juce::Timer
{
    std::function<void(void)> displayLabel { [this]() -> void {
        std::cout << mPosition.load() << std::endl;
    }};
    //A ctor with a valid std::atomic reference
    AtomicLabel(std::atomic<double>& inletPositionRef):
        mPosition(inletPositionRef)
    {
        //Spawn the label.
        addAndMakeVisible(mLabel);
        //Tick @ 143 hz : 6.99 ms
        startTimerHz(static_cast<int>(mREFRESH_RATE));
    }

    void resized() override
    {
        //resize to parent
        mLabel.setBounds(getLocalBounds());
    }

    void timerCallback() override
    {
        auto position = mPosition.load();
        auto positionString = juce::String{position};
        mLabel.setText(positionString, juce::dontSendNotification);

        //Things to do every second
        mREFRESH_COUNT += 1; mREFRESH_COUNT %= mREFRESH_RATE;
        if (!mREFRESH_COUNT) displayLabel();
    }

    juce::Label mLabel;
    std::atomic<double>& mPosition;
    const int mREFRESH_RATE = 60;
    int mREFRESH_COUNT = 0;

};

//==============================================================================
class AudioStreamPluginEditor : public juce::AudioProcessorEditor
{
public:
    explicit AudioStreamPluginEditor (AudioStreamPluginProcessor&);
    ~AudioStreamPluginEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:

    //std::unique_ptr<melatonin::Inspector> inspector;
    //juce::TextButton inspectButton { "Inspect the UI" };


    AtomicLabel mPositionLabel;

};



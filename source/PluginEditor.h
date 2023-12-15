#pragma once

#include "PluginProcessor.h"
#include "SliderListener.h"
#include "BinaryData.h"
//#include "melatonin_inspector/melatonin_inspector.h"


struct StreamAudioView : juce::Component, juce::Timer
{
public:

    StreamAudioView(AudioStreamPluginProcessor&);
    ~StreamAudioView() override;

    juce::ToggleButton toggleMonoStereoStream;
    juce::TextButton    infoButton;
    juce::ComboBox      interfaceSelector;
    juce::TextButton    streamButton;

    void paint (juce::Graphics& g) override;

    void resized() override
    {
        auto rect = getLocalBounds();
        auto width = (int) (rect.getWidth()*0.8f);
        auto height = 32;
        auto rsz = [&rect, width, height](juce::Component* compo)
        {
            rect.removeFromTop(10);
            compo->setBounds(rect.removeFromTop(height).withSizeKeepingCentre(width, height));
        };

        std::vector<juce::Component*> components = {
            &infoButton,
            &toggleMonoStereoStream,
            &interfaceSelector,
            &streamButton
        };
        for (auto compo : components)
        {
            rsz(compo);
        }
    }

    void timerCallback() override
    {
    }
    AudioStreamPluginProcessor& processorReference;
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
    StreamAudioView streamAudioView;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioStreamPluginEditor)

};


struct AtomicLabel : juce::Component, juce::Timer
{
    std::function<void(void)> displayLabel { [this]() -> void {
        std::cout << mPosition.load() << std::endl;
    }};
    //A ctor with a valid std::atomic reference
    AtomicLabel(std::atomic<double>& inletPositionRef): mPosition(inletPositionRef)
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
    juce::ComboBox listeningAddress { "Listening Address" };


    //Refresh rate
    const int mREFRESH_RATE = 60;
    int mREFRESH_COUNT = 0;

};

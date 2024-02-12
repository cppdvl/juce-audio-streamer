#pragma once

#include "PluginProcessor.h"
#include "SliderListener.h"
#include "BinaryData.h"
//#include "melatonin_inspector/melatonin_inspector.h"
#include "GUI/AuthModal.h"
#include "GUI/Meter.h"
#include "signalsslots.h"


struct StreamAudioView : juce::Component, juce::Timer
{
public:

    StreamAudioView(AudioStreamPluginProcessor&);
    ~StreamAudioView() override;

    juce::ToggleButton  toggleMonoStereoStream;
    juce::TextButton    authButton;


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
            &authButton,
            &toggleMonoStereoStream,
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


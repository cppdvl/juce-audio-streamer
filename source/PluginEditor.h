#pragma once

#include "PluginProcessor.h"
#include "SliderListener.h"
#include "BinaryData.h"
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

    DAWn::GUI::Meter    meterL[4];
    DAWn::GUI::Meter    meterR[4];

    void timerCallback() override;

    void paint (juce::Graphics& g) override;

    void resized() override
    {
        auto rect = getLocalBounds();
        auto width = (int) (rect.getWidth()*0.8f);
        auto height = 32;
        rect.removeFromTop(30);
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

        auto rmslayout = [this, &rect](size_t size)
        {
            const int iSize = static_cast<int>(size);
            const int lrSep = 6;
            const int mtSep = 24;
            const int centerW = rect.getWidth() / 2;
            const int centerH = 180;
            const int lvlW = 18;
            const int lvlH = 72;
            const int stereoW = lvlW * 2 + lrSep;

            for (int index = 0; index < iSize; index++)
            {
                int m = (index % 2 == 0 ? -1 : 1) * (mtSep + stereoW) * (index >> 1);
                int bLeft = centerW + (index % 2 == 0 ? -1 : 1) * (mtSep >> 1) + (index % 2 == 0 ? -stereoW : 0);
                int bRight = centerW + (index % 2 == 0 ? - 1 : 1) * (mtSep >> 1) + (index % 2 == 0 ? -lvlW : lvlW + lrSep);
                this->meterL[index].setBounds(bLeft +  m, centerH - lvlH / 2, lvlW, lvlH);
                this->meterR[index].setBounds(bRight + m, centerH - lvlH / 2, lvlW, lvlH);
            }
        };

        rect.removeFromTop(30);
        rmslayout(4);
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


#pragma once

#include "PluginProcessor.h"
#include "SliderListener.h"
#include "BinaryData.h"
#include "melatonin_inspector/melatonin_inspector.h"


struct StreamAudioView : juce::Component, juce::Timer
{
public:

    StreamAudioView(AudioStreamPluginProcessor&);
    ~StreamAudioView() override;

    juce::ToggleButton toggleTone;
    juce::ToggleButton toggleOpus;
    juce::ToggleButton toggleDebug;
    juce::ToggleButton toggleMuteTrack{};
    juce::TextButton toggleStream;
    juce::TextButton infoButton;



    SliderListener frequencySlider{440.0f, 1200.0, 440.0,
        [](){ std::cout << "Freq Slider Changed" << std::endl; }
    };
    SliderListener masterGainSlider{0.0f, 1.0f, 0.01f,
        [](){ std::cout << "MasterOut Slider Changed" << std::endl; }
    };
    SliderListener streamInGainSlider{0.0f, 1.0f, 0.1f,
        [](){ std::cout << "StreamIn Slider Changed" << std::endl; }
    };
    SliderListener streamOutGainSlider{0.0f, 1.0f, 0.1f,
        [](){ std::cout << "StreamOut Slider Changed" << std::endl; }
    };

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
            &toggleTone,
            &toggleStream,
            &toggleOpus,
            &toggleDebug,
            &toggleMuteTrack,
            &frequencySlider,
            &masterGainSlider,
            &streamInGainSlider,
            &streamOutGainSlider
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

    const int mREFRESH_RATE = 60;
    int mREFRESH_COUNT = 0;

};



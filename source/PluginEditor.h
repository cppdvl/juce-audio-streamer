#pragma once

#include "PluginProcessor.h"
#include "SliderListener.h"
#include "BinaryData.h"
#include "melatonin_inspector/melatonin_inspector.h"


struct StreamAudioView : juce::Component, juce::Timer
{
public:
    StreamAudioView(AudioStreamPluginProcessor&p) : processorReference(p)
    {
        addAndMakeVisible(toggleStream);
        toggleStream.onClick = [this]() -> void {
            processorReference.streamOut = !processorReference.streamOut;
            std::cout << "StreamOut: " << processorReference.streamOut << std::endl;
            toggleStream.setButtonText(processorReference.streamOut ? "Stop" : "Stream");
        };
        toggleStream.setButtonText("Stream");
        addAndMakeVisible(toggleOpus);
        toggleOpus.setButtonText("Using Raw");
        toggleOpus.onClick = [this]() -> void {
            processorReference.useOpus = !processorReference.useOpus;
            std::cout << "UseOpus: " << processorReference.useOpus << std::endl;
            toggleOpus.setButtonText(processorReference.useOpus ? "Using Opus" : " Using Raw");
        };
        addAndMakeVisible(toggleDebug);
        toggleDebug.setButtonText("Debug");
        toggleDebug.onClick = [this]() -> void {
            processorReference.debug = !processorReference.debug;
            std::cout << "Debug: " << processorReference.debug << std::endl;
            toggleDebug.setButtonText(processorReference.debug ? "Debug On" : "Debug Off");
        };
        addAndMakeVisible(toggleMuteTrack);
        toggleMuteTrack.setButtonText("Mute Track");
        toggleMuteTrack.onClick = [this]() -> void {
            processorReference.muteTrack = !processorReference.muteTrack;
            std::cout << "Mute Track: " << processorReference.muteTrack << std::endl;
            toggleMuteTrack.setButtonText(processorReference.muteTrack ? "Unmute Track" : "Mute Track");
        };

        addAndMakeVisible(infoButton);
        infoButton.onClick = [this]() -> void {
            std::cout << "Sample Rate: " << processorReference.getSampleRate() << std::endl;
            std::cout << "BlockSz: " << processorReference.getBlockSize() << std::endl;
            std::cout << "Outport [" << processorReference.outPort << "] Inport [" << processorReference.inPort << "]" << std::endl;
            std::cout << "StreamOut: " << processorReference.streamOut << std::endl;
            std::cout << "UseOpus: " << processorReference.useOpus << std::endl;
        };
        infoButton.setButtonText("Info");
        addAndMakeVisible(frequencySlider);
        frequencySlider.onSliderChangedSlot = {
            [this]()
            {
                processorReference.frequency = frequencySlider.getValue();
                processorReference.toneGenerator.setFrequency(processorReference.frequency);
            }
        };

        addAndMakeVisible(masterGainSlider);
        masterGainSlider.onSliderChangedSlot = {
            [this]()
            {
                processorReference.masterGain = masterGainSlider.getValue();
            }
        };

        addAndMakeVisible(streamInGainSlider);
        streamInGainSlider.onSliderChangedSlot = {
            [this]()
            {
                processorReference.streamInGain = streamInGainSlider.getValue();
                std::cout << "StreamInGain: " << processorReference.streamInGain << std::endl;
            }
        };
        addAndMakeVisible(streamOutGainSlider);
        streamOutGainSlider.onSliderChangedSlot = {
            [this]()
            {
                processorReference.streamOutGain = streamOutGainSlider.getValue();
                std::cout << "StreamOutGain: " << processorReference.streamOutGain << std::endl;
            }
        };


    }

    ~StreamAudioView() override = default;

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

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colours::black);
    }

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

        /*rect.removeFromTop(10);
        toggleStream.setBounds(rect.removeFromTop(height).withSizeKeepingCentre(width, height));
        rect.removeFromTop(10);
        toggleOpus.setBounds(rect.removeFromTop(height).withSizeKeepingCentre(width, height));
        rect.removeFromTop(10);
        infoButton.setBounds(rect.removeFromTop(height).withSizeKeepingCentre(width, height));
        rect.removeFromTop(10);
        frequencySlider.setBounds(rect.removeFromTop(height).withSizeKeepingCentre(width, height));
        rect.removeFromTop(10);
        masterGainSlider.setBounds(rect.removeFromTop(height).withSizeKeepingCentre(width, height));
        rect.removeFromTop(10);
        streamInGainSlider.setBounds(rect.removeFromTop(height).withSizeKeepingCentre(width, height));
        rect.removeFromTop(10);
        streamOutGainSlider.setBounds(rect.removeFromTop(height).withSizeKeepingCentre(width, height));
         */
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



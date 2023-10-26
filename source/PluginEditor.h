#pragma once

#include "PluginProcessor.h"
#include "BinaryData.h"
#include "melatonin_inspector/melatonin_inspector.h"


struct StreamAudioView : juce::Component, juce::Timer
{
public:
    StreamAudioView(AudioStreamPluginProcessor&p) : processorReference(p)
    {
        addAndMakeVisible(selectSndRcv);
        selectSndRcv.setButtonText("Send");
        selectSndRcv.setClickingTogglesState(true);
        selectSndRcv.onClick = [this]() -> void {
            streamButton.setButtonText(selectSndRcv.getToggleState() ? "Receive" : "Send");
            selectSndRcv.setButtonText(selectSndRcv.getToggleState() ? "Receive" : "Send");
        };

        addAndMakeVisible(port);
        port.setText("8888");
        addAndMakeVisible(frequency);
        frequency.setText("440.0");
        frequency.onTextChange = [this]() -> void {
            auto frequencyValue = frequency.getText().getDoubleValue();
            frequencyValue = frequencyValue > 1200.0 ? 1200.0 : frequencyValue;
            processorReference.toneGenerator.setFrequency(frequencyValue);
            frequency.setText(juce::String(frequencyValue), juce::dontSendNotification);
        };

        addAndMakeVisible(streamButton);
        streamButton.setButtonText(selectSndRcv.getToggleState() ? "Receive" : "Send");
        streamButton.onClick = [this]() -> void {
            if (processorReference.isListening() )
            {
                std::cout << "Im the Receiver" << std::endl;
            }
            else
            {
                std::cout << "Im the Sender" << std::endl;
            }
        };

        addAndMakeVisible(infoPanel);
        infoPanel.setText("Info Panel", juce::dontSendNotification);
        infoPanel.setColour(juce::Label::ColourIds::backgroundColourId, juce::Colours::black);
        infoPanel.setColour(juce::Label::ColourIds::textColourId, juce::Colours::white);
        infoPanel.setJustificationType(juce::Justification::centred);
    }

    ~StreamAudioView() override = default;

    juce::ToggleButton selectSndRcv;
    juce::TextEditor port;
    juce::TextEditor frequency{"440.0"};
    juce::TextButton streamButton;
    juce::Label infoPanel;

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colours::black);

    }

    void resized() override
    {
        auto rect = getLocalBounds();
        auto width = (int) (rect.getWidth()*0.8f);
        auto height = 48;

        selectSndRcv.setBounds(rect.removeFromTop(height).withSizeKeepingCentre(width, height));
        rect.removeFromTop(10);
        port.setBounds(rect.removeFromTop(height).withSizeKeepingCentre(width, height));
        rect.removeFromTop(10);
        frequency.setBounds(rect.removeFromTop(height).withSizeKeepingCentre(width, height));
        rect.removeFromTop(10);
        streamButton.setBounds(rect.removeFromTop(height).withSizeKeepingCentre(width, height));
        infoPanel.setBounds(rect.removeFromTop(height).withSizeKeepingCentre(width, height));

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



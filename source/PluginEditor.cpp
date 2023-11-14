#include "PluginEditor.h"

AudioStreamPluginEditor::AudioStreamPluginEditor (AudioStreamPluginProcessor& p):
    AudioProcessorEditor (&p),
    streamAudioView(p)
{
    addAndMakeVisible(streamAudioView);
    // Plugin Widget Size
    setSize (1200, 800);

}

AudioStreamPluginEditor::~AudioStreamPluginEditor()
{
}

void AudioStreamPluginEditor::paint (juce::Graphics&g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    auto area = getLocalBounds();
    g.setColour (juce::Colours::white);
    g.setFont (12.0f);
    juce::String helloWorld = juce::String(PRODUCT_NAME_WITHOUT_VERSION) + " v" VERSION + " [ " + CMAKE_BUILD_TYPE + " ]";
    g.drawText (helloWorld, area.removeFromTop (20), juce::Justification::centred, false);

}

void AudioStreamPluginEditor::resized()
{
    auto area = getLocalBounds();

    // layout the positions of your child components here
    area.removeFromBottom(50);
    streamAudioView.setBounds (getLocalBounds().withSizeKeepingCentre(360, 720));
}

StreamAudioView::StreamAudioView(AudioStreamPluginProcessor&p) : processorReference(p)
{
    addAndMakeVisible(toggleTone);
    toggleTone.setButtonText("Tone On");
    toggleTone.onClick = [this]() -> void {
        processorReference.useTone ^= true;
        std::cout << "UseTone: " << processorReference.useTone << std::endl;
        toggleTone.setButtonText(processorReference.useTone ? "Tone Off" : "Tone On");
    };

    addAndMakeVisible(toggleStream);
    toggleStream.onClick = [this]() -> void {
        processorReference.streamOut ^= true;

        if (processorReference.useOpus && processorReference.streamOut && !processorReference.isOkToEncode()){
            std::cout << "SampRate, BlockSz : " << processorReference.mSampleRate << ", " << processorReference.mBlockSize << " -- NOT SUPPORTED -- " << std::endl;
            processorReference.streamOut = false;
            return;
        }

        std::cout << "StreamOut: " << processorReference.streamOut << std::endl;
        if (processorReference.useOpus && processorReference.streamOut)
        {
            std::cout << "Using Opus: ATTEMPT ENCODING!!!!!" << std::endl;
        }
        toggleStream.setButtonText(processorReference.streamOut ? "Stop" : "Stream");
    };
    toggleStream.setButtonText("Stream");


    addAndMakeVisible(toggleOpus);
    toggleOpus.setButtonText("Using Raw");
    toggleOpus.onClick = [this]() -> void {
        processorReference.useOpus ^= true;
        if (processorReference.useOpus && processorReference.streamOut && !processorReference.isOkToEncode()){
            std::cout << "SampRate, BlockSz : " << processorReference.mSampleRate << ", " << processorReference.mBlockSize << " -- NOT SUPPORTED -- " << std::endl;
            processorReference.useOpus = false;
        }


        std::cout << "UseOpus: " << processorReference.useOpus << std::endl;
        if (processorReference.useOpus && processorReference.streamOut)
        {
            std::cout << "Using Opus: ATTEMPT ENCODING!!!!!" << std::endl;
        }
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
        std::cout << "Sample Rate: " << processorReference.mSampleRate << std::endl;
        std::cout << "BlockSz: " << processorReference.mBlockSize << std::endl;
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

StreamAudioView::~StreamAudioView()
{
}

void StreamAudioView::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colours::blueviolet);
}
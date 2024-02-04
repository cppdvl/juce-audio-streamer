#include "PluginEditor.h"

constexpr auto COMPILATION_TIMESTAMP = __DATE__ " " __TIME__;

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

    juce::String helloWorld = juce::String(PRODUCT_NAME_WITHOUT_VERSION) + " v" VERSION + " [ " + CMAKE_BUILD_TYPE + " | " + COMPILATION_TIMESTAMP + " ]";
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
    addAndMakeVisible(toggleMonoStereoStream);
    toggleMonoStereoStream.setButtonText("Stream Mono");
    toggleMonoStereoStream.onClick = [this]() -> void {
        auto& mono = processorReference.getMonoFlagReference();
        mono ^= true;
        toggleMonoStereoStream.setButtonText(mono ? "Streaming Mono" : "Streaming Stereo");
    };

    addAndMakeVisible(infoButton);
    infoButton.onClick = [this]() -> void {
        const auto& blockSize   = processorReference.getBlockSizeReference();
        const auto& sampleRate  = processorReference.getSampleRateReference();
        const auto& mono        = processorReference.getMonoFlagReference();

        std::cout << "Sample Rate: " << sampleRate << std::endl;
        std::cout << "BlockSz: " << blockSize << std::endl;
        std::cout << "Streaming Mono: " << mono << std::endl;
    };
    infoButton.setButtonText("Info");


}

StreamAudioView::~StreamAudioView()
{
}

void StreamAudioView::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colours::blueviolet);
}
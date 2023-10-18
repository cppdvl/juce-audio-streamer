#include "PluginEditor.h"

AudioStreamPluginEditor::AudioStreamPluginEditor (AudioStreamPluginProcessor& p):
    AudioProcessorEditor (&p),
    streamAudioView(p)
{
    addAndMakeVisible(streamAudioView);
    // Plugin Widget Size
    setSize (800, 600);

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
    g.drawText (helloWorld, area.removeFromTop (150), juce::Justification::centred, false);

}

void AudioStreamPluginEditor::resized()
{
    auto area = getLocalBounds();

    // layout the positions of your child components here
    area.removeFromBottom(50);
    streamAudioView.setBounds (getLocalBounds().withSizeKeepingCentre(180, 320));
}


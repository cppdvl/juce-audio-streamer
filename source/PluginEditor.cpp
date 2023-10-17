#include "PluginEditor.h"

AudioStreamPluginEditor::AudioStreamPluginEditor (AudioStreamPluginProcessor& p):
    AudioProcessorEditor (&p),
    mPositionLabel(p.mScrubCurrentPosition)
{
    //MELATONIN INSPECTOR BUTTON AND CLICK DELEGATE
    //addAndMakeVisible (inspectButton);
    /*inspectButton.onClick = [&] {
        if (!inspector)
        {
            inspector = std::make_unique<melatonin::Inspector> (*this);
            inspector->onClose = [this]() { inspector.reset(); };
        }

        inspector->setVisible (true);
    };*/
    
    // Plugin Widget Size
    setSize (400, 300);

}

AudioStreamPluginEditor::~AudioStreamPluginEditor()
{
}

void AudioStreamPluginEditor::paint (juce::Graphics&g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    //auto area = getLocalBounds();
    //g.setColour (juce::Colours::white);
    //g.setFont (16.0f);
    //auto helloWorld = juce::String ("Hello  from ") + PRODUCT_NAME_WITHOUT_VERSION + " v" VERSION + " running in " + CMAKE_BUILD_TYPE;
    //g.drawText (helloWorld, area.removeFromTop (150), juce::Justification::centred, false);
    addAndMakeVisible(mPositionLabel);
}

void AudioStreamPluginEditor::resized()
{
    // layout the positions of your child components here
    mPositionLabel.setBounds(getLocalBounds());
    //area.removeFromBottom(50);
    //inspectButton.setBounds (getLocalBounds().withSizeKeepingCentre(100, 50));
}


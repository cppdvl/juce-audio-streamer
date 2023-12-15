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
        processorReference.streamMono ^= true;
        toggleMonoStereoStream.setButtonText(processorReference.streamMono ? "Streaming Mono" : "Streaming Stereo");
    };

    addAndMakeVisible(infoButton);
    infoButton.onClick = [this]() -> void {
        std::cout << "Sample Rate: " << processorReference.mSampleRate << std::endl;
        std::cout << "BlockSz: " << processorReference.mBlockSize << std::endl;
        std::cout << "Outport [" << processorReference.outPort << "] Inport [" << processorReference.inPort << "]" << std::endl;
        std::cout << "StreamOut: " << processorReference.streamOut << std::endl;
        std::cout << "Streaming Mono: " << processorReference.streamMono << std::endl;
    };

    infoButton.setButtonText("Info");
    addAndMakeVisible(interfaceSelector);
    auto networkEntries = Utilities::Network::getNetworkInterfaces();
    auto entriesIndexes = std::vector<size_t>(networkEntries.size(), 0); std::iota(entriesIndexes.begin(), entriesIndexes.end(), 0);
    for(auto& entryIndex : entriesIndexes)
    {
        std::cout << "Adding " << networkEntries[entryIndex] << "," << entryIndex+1 << std::endl;
        interfaceSelector.addItem(networkEntries[entryIndex], static_cast<int>(entryIndex+1));
    }
    interfaceSelector.onChange = [this](void) -> void {
        auto comboId = interfaceSelector.getSelectedItemIndex();
        std::cout << "Selected " << comboId << std::endl;
        std::cout << "Selected " << interfaceSelector.getItemText(comboId) << std::endl;
    };

    streamButton.onClick = [this]() -> void
    {
        //Get Interface Selector, selected index.
        auto selectedNetworkInterfaceID = interfaceSelector.getSelectedId();
        auto ip = interfaceSelector.getItemText(selectedNetworkInterfaceID).toStdString();
        Utilities::Network::createSession(
            processorReference.getRTP(),
            processorReference.getSessionID(),
            processorReference.getStreamOutputID(),
            processorReference.getStreamInputID(),
            processorReference.outPort,
            processorReference.inPort,
            ip);

        //Parse the IP address.
        //Get the port


        //IN REALITY WHAT WE WOULD BE DOING HERE IS TO USE A MODAL DIALOGUE
        //To Provide
        //A host for the signaling service.
        //This host will prompt for:
        //1. Authentication. SSO?. MFA? 2FA?
        //2. CODEC Configuration Settings: Stereo/Mono, Sample Rate, Block Size, Number of Channels to Stream.
        //3. A name to identify the user in the session.



    };

    streamButton.setButtonText("Create Session ");


}

StreamAudioView::~StreamAudioView()
{
}

void StreamAudioView::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colours::blueviolet);
}
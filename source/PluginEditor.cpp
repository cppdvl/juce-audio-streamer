#include "PluginEditor.h"
#include <format>


AudioStreamPluginEditor::AudioStreamPluginEditor (AudioStreamPluginProcessor& p):
    AudioProcessorEditor (&p),
    AudioProcessorEditorARAExtension (&p),
    streamAudioView(p,*this)
{

    //Set resizablity to avoid juce assertion failure
    setResizable(true, true);
    if (isARAEditorView())
    {
        editorView = getARAEditorView();
        // assign DocumentController reference to PluginProcessor
        p.setARADocumentControllerRef(editorView->getDocumentController());
    }

    addAndMakeVisible(streamAudioView);
    setSize (1200, 800);

}

AudioStreamPluginEditor::~AudioStreamPluginEditor()
{
}

void AudioStreamPluginEditor::paint (juce::Graphics&g)
{
    auto basePtr = this->getAudioProcessor();
    auto drvdPtr = basePtr == nullptr ? nullptr : dynamic_cast<AudioStreamPluginProcessor*>(basePtr);
    if (!basePtr || !drvdPtr)
    {
        std::cout << "CRITICAL: Gui thread lost reference to plugin" << std::endl;
    }
    else while (!(drvdPtr->commandStrings.empty()))
    {
        auto command = drvdPtr->commandStrings.front();
        drvdPtr->receiveWSCommand(command.c_str());
        drvdPtr->commandStrings.pop();
    }

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
    // layout the positions of your child components here
    // area.removeFromBottom(50);
    streamAudioView.setBounds (getLocalBounds().withSizeKeepingCentre(720, 360));
}

void StreamAudioView::timerCallback()
{
    auto [l, r] = processorReference.getRMSLevelsAudioBuffer();
    meterL[0].setLevel(l);
    meterR[0].setLevel(r);
    auto [l1, r1] = processorReference.getRMSLevelsJitterBuffer();
    meterR[1].setLevel(r1);
    meterL[1].setLevel(l1);

    for (size_t index = 0; index < 4; index++)
    {
        meterR[index].repaint();
        meterL[index].repaint();
    }
}

// NOTE: StreamAudioView requires a reference to PluginEditor for ARA methods
StreamAudioView::StreamAudioView(AudioStreamPluginProcessor&p, AudioStreamPluginEditor&e) : 
    processorReference(p),
    editorReference(e)
{
    startTimerHz(30);
    for (size_t index = 0; index < 4; index++)
    {
        addAndMakeVisible(meterL[index]);
        addAndMakeVisible(meterR[index]);
        meterL[index].setLevel(-60.0f);
        meterR[index].setLevel(-60.0f);
    }




    addAndMakeVisible(toggleMonoStereoStream);
    toggleMonoStereoStream.setButtonText("Stream Mono");
    toggleMonoStereoStream.onClick = [this]() -> void {
        auto& mono = processorReference.getMonoFlagReference();
        mono ^= true;
        toggleMonoStereoStream.setButtonText(mono ? "Streaming Mono" : "Streaming Stereo");
    };

    addAndMakeVisible(authButton);
    authButton.onClick = [this]() -> void {
        AuthModal* authModal = new AuthModal();
        authModal->setSecret(processorReference.getApiKey());
        juce::DialogWindow::LaunchOptions options;
        options.content.setOwned(authModal);
        options.dialogTitle = "API Key Authentication";
        options.dialogBackgroundColour = juce::Colours::black;
        options.escapeKeyTriggersCloseButton = true;
        options.useNativeTitleBar = true;
        options.resizable = false;
        options.launchAsync();
        authModal->onSecretSubmit.Connect([this](std::string secret) -> void {
            processorReference.tryApiKey(secret);
        });

    };

    authButton.setButtonText("Enter Your API KEY");

    p.sgnStatusSet.Connect(std::function<void(std::string)>{[this](std::string role) -> void {
        authButton.setButtonText(role);
    }});

    addAndMakeVisible(gainSlider);
    gainSlider.setRange(0.0f, 1.0);
    gainSlider.setValue(1.0);
    gainSlider.onValueChange = [this]() -> void {
        this->processorReference.setOutputGain(this->gainSlider.getValue());
    };

    // **** ARA Test Widgets (remove after doing testing) ****
    addAndMakeVisible(ARAHostPlayButton);
    ARAHostPlayButton.setButtonText ("Play");
    ARAHostPlayButton.onClick = [this]() -> void {
      editorReference.doARAHostPlaybackControllerPlay();
    };
    addAndMakeVisible(ARAHostStopButton);
    ARAHostStopButton.setButtonText ("Stop");
    ARAHostStopButton.onClick = [this]() -> void {
      editorReference.doARAHostPlaybackControllerStop();
    };
    addAndMakeVisible(ARAPositionLabel);
    ARAPositionLabel.setText ("Timepos:", juce::dontSendNotification);
    addAndMakeVisible (ARAPositionInput);
    ARAPositionInput.setText ("0.0", juce::dontSendNotification);
    ARAPositionInput.setColour (juce::Label::backgroundColourId, juce::Colours::darkblue);
    ARAPositionInput.setEditable (true);
    addAndMakeVisible(ARAHostPositionButton);
    ARAHostPositionButton.setButtonText ("Move");
    ARAHostPositionButton.onClick = [this]() -> void {
      double timevalue = ARAPositionInput.getText().getDoubleValue();
      editorReference.doARAHostPlaybackControllerSetPosition(timevalue);
      ARAPositionInput.setText(juce::String::formatted("%2f", timevalue), juce::dontSendNotification);
    };
    // ****
}

StreamAudioView::~StreamAudioView()
{
}

void StreamAudioView::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colours::lightslategrey);
}

// ARA Getters
ARA::PlugIn::DocumentController* AudioStreamPluginEditor::getARADocumentController() const
{
    return editorView->getDocumentController(); 
}
ARA::PlugIn::Document* AudioStreamPluginEditor::getARADocument() const
{ 
    return editorView->getDocumentController()->getDocument(); 
}

// ARA HostPlaybackController callbacks
void AudioStreamPluginEditor::doARAHostPlaybackControllerPlay() noexcept
{
    auto playbackController = editorView->getDocumentController()->getHostPlaybackController();
    std::cout << "Play: start playback!" << std::endl;
    playbackController->requestStartPlayback();

    juce::AudioProcessor* audioProcessorPtr = this->getAudioProcessor();
    if (audioProcessorPtr)
    {
        //auto pluginProcessorPtr = dynamic_cast<AudioStreamPluginProcessor*>(audioProcessorPtr);
    }
} 
void AudioStreamPluginEditor::doARAHostPlaybackControllerStop() noexcept
{
    auto playbackController = editorView->getDocumentController()->getHostPlaybackController();
    std::cout << "Play: stop playback!" << std::endl;
    playbackController->requestStopPlayback();
}

void AudioStreamPluginEditor::doARAHostPlaybackControllerSetPosition(double timePosition) noexcept
{
    auto playbackController = editorView->getDocumentController()->getHostPlaybackController();
    std::cout << "Playback: set position to " << timePosition << " seconds!" << std::endl;
    playbackController->requestSetPlaybackPosition(timePosition);
}

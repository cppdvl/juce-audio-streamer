#include "PluginEditor.h"

constexpr auto COMPILATION_TIMESTAMP = __DATE__ " " __TIME__;

AudioStreamPluginEditor::AudioStreamPluginEditor (AudioStreamPluginProcessor& p):
    AudioProcessorEditor (&p),
    streamAudioView(p)
{
    addAndMakeVisible(streamAudioView);
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

StreamAudioView::StreamAudioView(AudioStreamPluginProcessor&p) : processorReference(p)
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
}

StreamAudioView::~StreamAudioView()
{
}

void StreamAudioView::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colours::lightslategrey);
}
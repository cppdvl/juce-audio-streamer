#pragma once

#include "PluginProcessor.h"
#include "SliderListener.h"
#include "BinaryData.h"
#include "GUI/AuthModal.h"
#include "GUI/Meter.h"
#include "signalsslots.h"

class AudioStreamPluginEditor;

struct StreamAudioView : juce::Component, juce::Timer
{
public:

    StreamAudioView(AudioStreamPluginProcessor&, AudioStreamPluginEditor&);
    ~StreamAudioView() override;

    juce::ToggleButton  toggleMonoStereoStream;
    juce::TextButton    authButton;
    DAWn::GUI::Meter    meterL[4];
    DAWn::GUI::Meter    meterR[4];

    juce::TextButton    ARAHostPlayButton;
    juce::TextButton    ARAHostStopButton;
    juce::Label         ARAPositionLabel;
    juce::Label         ARAPositionInput;
    juce::TextButton    ARAHostPositionButton;
    // ****

    void timerCallback() override;

    void paint (juce::Graphics& g) override;

    void resized() override
    {
        auto rect = getLocalBounds();
        auto width = (int) (rect.getWidth()*0.8f);
        auto height = 32;
        rect.removeFromTop(30);
        auto rsz = [&rect, width, height](juce::Component* compo)
        {
            rect.removeFromTop(10);
            compo->setBounds(rect.removeFromTop(height).withSizeKeepingCentre(width, height));
        };

        std::vector<juce::Component*> components = {
            &authButton,
            &toggleMonoStereoStream,
        };
        for (auto compo : components)
        {
            rsz(compo);
        }

        auto rmslayout = [this, &rect](size_t size)
        {
            const int iSize = static_cast<int>(size);
            const int lrSep = 6;
            const int mtSep = 24;
            const int centerW = rect.getWidth() / 2;
            const int centerH = 180;
            const int lvlW = 18;
            const int lvlH = 72;
            const int stereoW = lvlW * 2 + lrSep;

            for (int index = 0; index < iSize; index++)
            {
                int m = (index % 2 == 0 ? -1 : 1) * (mtSep + stereoW) * (index >> 1);
                int bLeft = centerW + (index % 2 == 0 ? -1 : 1) * (mtSep >> 1) + (index % 2 == 0 ? -stereoW : 0);
                int bRight = centerW + (index % 2 == 0 ? - 1 : 1) * (mtSep >> 1) + (index % 2 == 0 ? -lvlW : lvlW + lrSep);
                this->meterL[index].setBounds(bLeft +  m, centerH - lvlH / 2, lvlW, lvlH);
                this->meterR[index].setBounds(bRight + m, centerH - lvlH / 2, lvlW, lvlH);
            }
        };

        rect.removeFromTop(30);
        rmslayout(4);
        
        // **** ARA Test Widgets (remove after doing testing) ****
        // layout for ARA widgets (horizontal placement)
        rect.removeFromTop(100);
        auto horizrect = rect;
        const int horiz_sep = horizrect.getWidth() / 11;

        auto horiz_rsz = [&horizrect, horiz_sep, height](juce::Component* compo)
        {
          horizrect.removeFromLeft(horiz_sep);
          compo->setBounds(horizrect.removeFromLeft(horiz_sep).withSizeKeepingCentre(horiz_sep, height));
        };

        std::vector<juce::Component*> aracomponents = {
            &ARAHostPlayButton,
            &ARAHostStopButton,
            &ARAPositionLabel,
            &ARAPositionInput,
            &ARAHostPositionButton,
        };
        for (auto compo : aracomponents)
        {
            horiz_rsz(compo);
        }
        // ****
    }

    AudioStreamPluginProcessor& processorReference;
    AudioStreamPluginEditor& editorReference;
};



//==============================================================================
class AudioStreamPluginEditor :
    public juce::AudioProcessorEditor,
    public juce::AudioProcessorEditorARAExtension
{
public:
    explicit AudioStreamPluginEditor (AudioStreamPluginProcessor&);
    ~AudioStreamPluginEditor() override;
    
    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    // ARA getters
    ARA::PlugIn::DocumentController* getARADocumentController() const;
    ARA::PlugIn::Document* getARADocument() const;
    // HostPlayback callbacks
    void doARAHostPlaybackControllerPlay() noexcept;
    void doARAHostPlaybackControllerStop() noexcept;
    void doARAHostPlaybackControllerSetPosition(double timePosition) noexcept;

private:
    StreamAudioView streamAudioView;
    juce::ARAEditorView* editorView;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioStreamPluginEditor)

};


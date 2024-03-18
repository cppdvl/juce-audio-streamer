/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for an ARA playback renderer implementation.

  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>

class ARAReaders : public juce::TimeSliceThread
{
    public:
        ARAReaders() : TimeSliceThread ("ARAAudioSourceReader thread")
        {
            startThread(Priority::high);
        }
        
        void createReader(juce::ARAPlaybackRegion * playbackRegion)
        {
            auto * audioSource = playbackRegion->getAudioModification()->getAudioSource();
            readers[audioSource] = std::make_unique<juce::BufferingAudioReader> (new juce::ARAAudioSourceReader (audioSource), *this, 48000);
        }
        
        juce::AudioFormatReader& getReader (juce::ARAPlaybackRegion * playbackRegion)
        {
            auto * audioSource = playbackRegion->getAudioModification()->getAudioSource();
            return *readers[audioSource];
        }
    private:
        std::map<juce::ARAAudioSource*, std::unique_ptr<juce::AudioFormatReader>> readers;
};

//==============================================================================
/**
*/
class AudioStreamPluginPlaybackRenderer  : public juce::ARAPlaybackRenderer
{
public:
    //==============================================================================
    using juce::ARAPlaybackRenderer::ARAPlaybackRenderer;

    //==============================================================================
    void prepareToPlay (double sampleRate,
                        int maximumSamplesPerBlock,
                        int numChannels,
                        juce::AudioProcessor::ProcessingPrecision,
                        AlwaysNonRealtime alwaysNonRealtime) override;
    void releaseResources() override;

    //==============================================================================
    bool processBlock (juce::AudioBuffer<float>& buffer,
                       juce::AudioProcessor::Realtime realtime,
                       const juce::AudioPlayHead::PositionInfo& positionInfo) noexcept override;

private:
    //==============================================================================
    double sampleRate = 44100.0;
    int maximumSamplesPerBlock = 4096;
    int numChannels = 1;
    bool useBufferedAudioSourceReader = true;

    ARAReaders readers;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioStreamPluginPlaybackRenderer)
};

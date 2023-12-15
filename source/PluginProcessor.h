#ifndef __PLUGIN_PROCESSOR__
#define __PLUGIN_PROCESSOR__

#include <juce_audio_processors/juce_audio_processors.h>

#if (MSVC)
#include "ipps.h"
#endif
#include "AudioMixerBlock.h"
#include "Utilities/Utilities.h"
#include "opusImpl.h"
#include "uvgRTP.h"
#include <deque>
#include <mutex>

class AudioStreamPluginProcessor : public juce::AudioProcessor
{
public:
    AudioStreamPluginProcessor();
    ~AudioStreamPluginProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
/************************* DAWN AUDIO STREAMING *************************/




private:

    std::mutex                          mMutexInput;
    int mBlockSize                      {0};
    int mSampleRate                     {0};
    int mChannels                       {0};
    bool mMonoSplit                     {false};
    bool listenedRenderedAudioChannel   {false};
    /*!
     * @brief Update information about buffer settings.
     * @param buffer The buffer to update.
     */
    void beforeProcessBlock(juce::AudioBuffer<float>& buffer);

    /*!
     * @brief Get the Play Head object
     * @return A pair, first is the time in millisecond, second is the time in sample index.
     */
    std::tuple<uint32_t, int64_t> getUpdatedTimePosition();

    /*!
     * @brief The AudioMixerBlock class. One Block per Channel.
     */
    std::vector<Mixer::AudioMixerBlock> mAudioMixerBlocks {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioStreamPluginProcessor)
};



#endif /* __PLUGIN_PROCESSOR__ */

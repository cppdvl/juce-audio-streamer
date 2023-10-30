#ifndef __PLUGIN_PROCESSOR__
#define __PLUGIN_PROCESSOR__

#include <juce_audio_processors/juce_audio_processors.h>

#if (MSVC)
#include "ipps.h"
#endif
#include <deque>
#include <mutex>
#include "uvgRTP.h"


class AudioStreamPluginProcessor : public juce::AudioProcessor
{
public:
    AudioStreamPluginProcessor();
    ~AudioStreamPluginProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlockStreamInNaive(juce::AudioBuffer<float>&, juce::MidiBuffer&);
    void processBlockStreamOutNaive(juce::AudioBuffer<float>&, juce::MidiBuffer&);
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

    void streamOutNaive (int remotePort, std::vector<std::byte> data);
    juce::ToneGeneratorAudioSource  toneGenerator{};
    std::mutex mMutexInput;
    std::deque<std::byte> mInputBuffer;
    double frequency {440.0};
    double masterGain {0.01};
    double streamOutGain {0.1};
    double streamInGain {0.1};
    int inPort {0};
    int outPort {0};

    inline void printPorts()
    {
            int static cnt = 0;
            cnt++;
            if (cnt%1000 == 0)
            {
                std::cout << "Inport [" << inPort << "] Outport [" << outPort << "]" << std::endl;
            }
    }
private:

    bool udpPortIsInUse (int port);
    std::atomic<double> mScrubCurrentPosition {};
    SPRTP pRTP {std::make_shared<UVGRTPWrap>()};
    uint64_t streamSessionID;
    uint64_t streamIdInput{0};
    uint64_t streamIdOutput{0};
    //A4 tone generator.
    juce::AudioSourceChannelInfo channelInfo{};
    bool gettingData {false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioStreamPluginProcessor)
};



#endif /* __PLUGIN_PROCESSOR__ */

#ifndef __PLUGIN_PROCESSOR__
#define __PLUGIN_PROCESSOR__

#include <juce_audio_processors/juce_audio_processors.h>

#if (MSVC)
#include "ipps.h"
#endif
#include <deque>
#include <mutex>
#include "uvgRTP.h"
#include "opusImpl.h"

class AudioStreamPluginProcessor : public juce::AudioProcessor
{
public:
    AudioStreamPluginProcessor();
    ~AudioStreamPluginProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    juce::AudioBuffer<float> processBlockStreamInNaive(juce::AudioBuffer<float>& inStreamBuffer, juce::MidiBuffer&);
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

    /* TBD: Stream Out Naively */
    void streamOutNaive (std::vector<std::byte> data);

    //Tone Generation Helper
    juce::ToneGeneratorAudioSource  toneGenerator{};


    std::mutex                      mMutexInput;
    std::deque<std::byte>           mInputBuffer{};

    /* Slider Controllers */
    double                          frequency{440.0};
    double                          masterGain{0.01};

    double                          streamOutGain{0.1};
    double                          streamInGain {0.1};

    /* UdpPort ports */
    int                             inPort {0};
    int                             outPort {0};

    /* Operational Flags */
    bool                            streamOut {false};
    bool                            useOpus {false};
    bool                            debug {false};
    bool                            useTone {false};
    bool                            muteTrack {false};

    //Opus Encoder/Decoder
    std::unique_ptr<RTPStreamConfig> pCodecConfig {nullptr};
    std::shared_ptr<OpusImpl::CODEC> pOpusCodec {nullptr};

    inline void printPorts()
    {
            int static cnt = 0;
            cnt++;
            if (cnt%1000 == 0)
            {
                std::cout << "Inport [" << inPort << "] Outport [" << outPort << "]" << std::endl;
            }
    }

    SPRTP getRTP() {return pRTP;}

    int mBlockSize {0};
    int mSampleRate {0};
    inline void onSampleRateChaged () {
        oldSampleRate = mSampleRate;
    }
    inline void onBlockSizeChaged (){
        oldBlockSize = mBlockSize;
    }
    inline void beforeProcessBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
        mBlockSize = buffer.getNumSamples();
        mSampleRate = static_cast<int>(getSampleRate());
        if (oldSampleRate != mSampleRate || oldBlockSize != mBlockSize)
        {
            if (oldSampleRate != mSampleRate) onSampleRateChaged();
            if (oldBlockSize != mBlockSize) onBlockSizeChaged();

            toneGenerator.prepareToPlay(mBlockSize, mSampleRate);
            channelInfo.buffer = &buffer;
            channelInfo.startSample = 0;
        }

        channelInfo.buffer = &buffer;
        channelInfo.numSamples = buffer.getNumSamples();
        if (useTone) toneGenerator.getNextAudioBlock(channelInfo); //buffer->fToneGenerator

    }


    bool isOkToEncode()
    {
        if (mSampleRate == 48000 && mBlockSize == 480)
        {
            return true;
        }
    }

private:
    int oldSampleRate {0};
    int oldBlockSize {0};
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

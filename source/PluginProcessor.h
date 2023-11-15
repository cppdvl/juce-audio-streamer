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
#include "Utilities.h"

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
    std::unique_ptr<OpusImpl::CODECConfig>  pCodecConfig    {nullptr};
    std::shared_ptr<OpusImpl::CODEC>        pOpusCodec      {nullptr};

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

        //Tone Generator
        auto totalNumInputChannels  = getTotalNumInputChannels(); /* not if 2 */ totalNumInputChannels = totalNumInputChannels > 2 ? 2 : totalNumInputChannels;
        auto totalNumOutputChannels = getTotalNumOutputChannels();

        for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        {
            buffer.clear (i, 0, buffer.getNumSamples());
        }


    }
    /*!
     * @brief Get the Play Head object
     * @return A pair, first is the time in millisecond, second is the time in sample index. If sample position is -1 then there was an error related to the AudioPlayHead pointer. If -2 then there was an error related to the PositionInfo pointer. If -3 then there was an error related to the TimeSamples. If -4 then there was an error related to the TimeMS.
     */
    inline std::tuple<uint32_t, int64_t> getUpdatedTimePosition() {
        auto [nTimeMS, nSamplePosition] = Utilities::Time::getPosInMSAndSamples(getPlayHead());
        if (nSamplePosition < 0)
        {
            auto& timePositionInfoError = nSamplePosition;
            if (mLastPositionInfoError != timePositionInfoError)
            {
                mLastPositionInfoError  = timePositionInfoError;
                std::cout << "Time Get Error! : " << mTimePositionInfoErrorMap[timePositionInfoError] << std::endl;
            }

            return std::make_tuple(static_cast<uint32_t>(0), nSamplePosition);
        }
        return std::make_tuple(nTimeMS, nSamplePosition);
    }


    inline bool isOkToEncode() { return mSampleRate == 48000 && mBlockSize == 480; }

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

    int64_t mLastPositionInfoError = 0;

    //timeposition getter error message map.
    std::map<int64_t, std::string> mTimePositionInfoErrorMap {
        {Utilities::Time::NoPlayHead, "No PlayHead"},
        {Utilities::Time::NoPositionInfo, "No Position Info"},
        {Utilities::Time::NoTimeSamples, "No Time Samples"},
        {Utilities::Time::NoTimeMS, "No Time MS"}
    };


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioStreamPluginProcessor)
};



#endif /* __PLUGIN_PROCESSOR__ */

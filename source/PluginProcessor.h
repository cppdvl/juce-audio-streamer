#ifndef __PLUGIN_PROCESSOR__
#define __PLUGIN_PROCESSOR__

#include <juce_audio_processors/juce_audio_processors.h>

#if (MSVC)
#include "ipps.h"
#endif
#include "Utilities/Utilities.h"
#include "AudioMixerBlock.h"
#include "wsclient.h"
#include "opusImpl.h"
#include "RTPWrap.h"

#include <deque>
#include <mutex>


#define LAMBDA__(m, t, p) std::function<void(t)>{[](auto p){ displayPayload(m, p); } }
#define LAMBDASTART(w, m, t, p) std::function<void(t)>{[w](auto p){
#define LAMBDAEND }}


class AudioStreamPluginProcessor : public juce::AudioProcessor
{
public:
    enum class Role
    {
        None,
        NonMixer,
        Mixer
    };
    Role mRole {Role::NonMixer};

    AudioStreamPluginProcessor();
    ~AudioStreamPluginProcessor() override;

    void prepareToPlay (double sampleRate, int uid_ts_encodedPayload) override;
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

    void setRole(Role role);
    void aboutToTransmit(std::vector<std::byte>&);

/************************* DAWN AUDIO STREAMING *************************/
    /*!
     * @brief Get a reference to bool flag indicating if mono streaming applies.
     * @return A reference to the bool flag. (Use it with cariñito)
     */
    bool& getMonoFlagReference();

    /*!
     * @brief Get a const reference to the Sampling Rate.
     * @return
     */
    const int& getSampleRateReference() const;

    /*!
     * @brief Get a const reference to the Block Size.
     * @return
     */
    const int& getBlockSizeReference() const;



private:
    /* Dawn Audio Streaming Session API KEY
     **/
    std::string                         mAPIKey;
    WebSocketApplication                mWSApp;

    std::mutex                          mMutexInput;
    int mBlockSize                      {0};
    int mSampleRate                     {0};
    const int mChannels                 {2};
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

    /*!
     * @brief The Network Interface. Also known ass sessionID
     */
    uint64_t rtpSessionID       {0};
    uint64_t rtpStreamID        {0};
    std::unique_ptr<RTPWrap>    pRtp{nullptr};

    std::map<Mixer::TUserID, OpusImpl::CODEC> opusCodecMap{};

    void commandSetHost (const char*);
    void commandSetPeer(const char*);
    void commandDisconnect(const char*);
    void peerGone(const char*);
    void peerConnected(const char*);
    void commandSendAudioSettings(const char*);
    void backendConnected(const char*);

    /******** GUI ********/
    std::pair<float, float> rmsLevels{0.0f, 0.0f}; //0 LEFT, 1 RIGHT


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioStreamPluginProcessor)
};



#endif /* __PLUGIN_PROCESSOR__ */

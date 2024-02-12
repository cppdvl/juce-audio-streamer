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
    Role mRole {Role::None};

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

    void mApiKeyAuthentication(std::string apiKey);
    std::string getApiKey() const { return mAPIKey; }

private:

    /*! @brief the ApiKey for the DAWN Audio Streaming API.*/
    std::string             mAPIKey;

    /*! @brief A Web Socket Application to handle SDA (Streaming Discovery and Announcement) and other tasks.*/
    WebSocketApplication    mWSApp;

    /*! @brief Audio Stream Parameters. Number of Samples per Block.*/
    int                     mBlockSize{0};

    /*! @brief Audio Stream Parameters. Sample Rate.*/
    int                     mSampleRate{0};

    /*! @brief Check if we are averaging the 2 channels.*/
    bool                    mMonoSplit{false};

    /*!
     * @brief Update information about buffer settings.
     * @param buffer The buffer to update.
     */
    void beforeProcessBlock(juce::AudioBuffer<float>& buffer);

    /*!
     * @brief Process Encoded Information.
     * @param uid_ts_encodedPayload
     */
    void extractDecodeAndMix(std::vector<std::byte>& uid_ts_encodedPayload);

    /*!
     * @brief Encode A vector of blocks and push them thru outlet interface
     * */
    void packEncodeAndPush(std::vector<Mixer::Block>& blocks, uint32_t timeStamp);

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
     * @brief The Transport Interface. Identifier of the Stream Session.
     */
    uint64_t mRtpSessionID {0};

    /*! @brief The Stream Identifier. */
    uint64_t mRtpStreamID {0};

    /*! @brief The BufferIdentifier in the Mixer*/
    uint32_t mUserID {0};

    /*! @brief An RTP Interface */
    std::unique_ptr<RTPWrap>    pRtp{nullptr};

    /*!
     * @brief Every Identified user in the Audio Mixer uses a particular Opus Codec.
     */
    std::map<Mixer::TUserID, OpusImpl::CODEC> mOpusCodecMap {};

    /*!
     * @brief The Opus Codec for the user ID.
     * @return A reference to the Opus Codec.
     */
     OpusImpl::CODEC& getCodecPairForUser(Mixer::TUserID);

    /*********** BACKEND COMMANDS ***********/
    /*!
     * @brief Function announcing the plugin is assigned a Host (Mixer) Role
     */
    void commandSetHost (const char*);

    /*!
     * @brief Function announcing the plugin is assigned a Peer (Non-Mixer) Role
     */
    void commandSetPeer(const char*);

    /*!
     * @brief The plugin is now disconnected from the Host. Shutdown the websocket session. Role is none now.
     */
    void commandDisconnect(const char*);

    /*!
     * @brief A fellow musician has disconected.
     */
    void peerGone(const char*);

    /*!
     * @brief A fellow musician has connected.
     */
    void peerConnected(const char*);

    /*!
     * @brief The Backend is asking your audio settings.
     */
    void commandSendAudioSettings(const char*);

    /*!
     * @brief Welcome you are connected to the backend.
     */
    void backendConnected(const char*);

    /******** GUI ********/
    std::pair<float, float> rmsLevels{0.0f, 0.0f}; //0 LEFT, 1 RIGHT

    /****** PREVENT DOUBLE EXECUTION *********************/
    std::once_flag mOnceFlag;

    /****** DEVELOPER FLAGS *******************************/
    bool mFixedRole {false};
    bool mFixedIP {false};
    bool mFixedPort {false};


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioStreamPluginProcessor)
};



#endif /* __PLUGIN_PROCESSOR__ */

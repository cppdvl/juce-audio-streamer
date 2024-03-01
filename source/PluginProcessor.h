#ifndef __PLUGIN_PROCESSOR__
#define __PLUGIN_PROCESSOR__

#include <juce_audio_processors/juce_audio_processors.h>

#if (MSVC)
#include "ipps.h"
#endif
#include "Utilities/Configuration/Configuration.h"
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

class AudioStreamPluginProcessor : public juce::AudioProcessor, public DAWn::Utilities::Configuration
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
    DAWn::Events::Signal<std::string> sgnStatusSet;
    /*!
     * @brief Get a reference to bool flag indicating if mono streaming applies.
     * @return A reference to the bool flag. (Use it with cariñito)
     */
    bool& getMonoFlagReference();


    void tryApiKey(const std::string& apiKey);
    std::string getApiKey() const { return mAPIKey; }

    /*!
     * @brief Get the rms levels of the audioBuffer.
     */
    std::pair<float, float>& getRMSLevelsAudioBuffer() { return rmsLevelsInputAudioBuffer; }
    std::pair<float, float>& getRMSLevelsJitterBuffer() { return rmsLevelsJitterBuffer; }

private:

    /*! @brief the ApiKey for the DAWN Audio Streaming API.*/
    std::string             mAPIKey;

    /*! @brief A Web Socket Application to handle SDA (Streaming Discovery and Announcement) and other tasks.*/
    WebSocketApplication    mWSApp;


    struct {
        /*! @brief Audio Stream Parameters. Number of Samples per Block in the DAW.*/
        size_t                  mDAWBlockSize{0};
        bool                    mDAWBlockSizeChanged{false};
        DAWn::Events::Signal<>  sgnDAWBlockSizeChanged;
        uint32_t                mLastTimeStamp{0};
        bool                    mLastTimeStampIsDirty{false};
        /*! @brief Audio Stream Parameters. Sample Rate.*/
        int                     mSampleRate{0};
        /*! @brief Check if we are averaging the 2 channels.*/
        bool                    mMonoSplit{false};

    } mAudioSettings;

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
     * @brief Every Identified user in the Audio Mixer uses a particular pair of Opus Codec and BlockSizeAdapter vectors
     *
     * ENCONDING 6 CHANNELS => 3 PAIRS, but the common user case is 2 channels (left right)
     *
     *     INTERLEAVED CHANNEL[PAIR0] => BSADAPTER[USERID][0]   => OPUSENCODER[USERID][0]
     *     INTERLEAVED CHANNEL[PAIR1] => BSADAPTER[USERID][2]   => OPUSENCODER[USERID][1]
     *     INTERLEAVED CHANNEL[PAIRN] => BSADAPTER[USERID][2*N] => OPUSENCODER[USERID][2]
     *
     * DECODING 6 CHANNELS => 3 PAIRS, but the common user case is 2 channels (left right)
     *
     *     OPUSDECODER[USERID][0] => BSADAPTER[USERID][1]   => INTERLEAVED CHANNEL[PAIR0]
     *     OPUSDECODER[USERID][1] => BSADAPTER[USERID][3]   => INTERLEAVED CHANNEL[PAIR1]
     *     OPUSDECODER[USERID][2] => BSADAPTER[USERID][5]   => INTERLEAVED CHANNEL[PAIRN]
     *
     */
    std::mutex mOpusCodecMapMutex;
    std::map<Mixer::TUserID, std::pair<OpusImpl::CODEC, std::vector<Utilities::Buffer::BlockSizeAdapter>>> mOpusCodecMap {};
    std::thread mOpusEncoderMapThreadManager;
    std::thread mAudioMixerThreadManager;

    /*!
     * @brief The Opus Codec for the user ID.
     * @param userID The user ID.
     * @param timeStamp In case the BSA adapter is not being used before, it will be created and will be assigned the timeStamp.
     * @return A reference to the Opus Codec.
     */
    std::pair<OpusImpl::CODEC, std::vector<Utilities::Buffer::BlockSizeAdapter>>& getCodecPairForUser(Mixer::TUserID, uint32_t timeStamp = 0);

    /*********** BACKEND COMMANDS ***********/
    void startRTP(std::string ip, int port);
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
    std::pair<float, float> rmsLevelsInputAudioBuffer {0.0f, 0.0f}; //first LEFT, second RIGHT
    std::pair<float, float> rmsLevelsJitterBuffer{0.0f, 0.0f};

    /****** PREVENT DOUBLE EXECUTION IN PREPARE TO PLAY *********************/
    std::once_flag mOnceFlag;




    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioStreamPluginProcessor)
};



#endif /* __PLUGIN_PROCESSOR__ */

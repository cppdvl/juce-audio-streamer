#ifndef __PLUGIN_PROCESSOR__
#define __PLUGIN_PROCESSOR__

#include <juce_audio_processors/juce_audio_processors.h>

constexpr auto COMPILATION_TIMESTAMP = __DATE__ " " __TIME__;

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



class AudioStreamPluginProcessor :
    public juce::AudioProcessor,
    public DAWn::Utilities::Configuration,
    public juce::AudioProcessorARAExtension
{
public:
    enum class Role
    {
        None,
        NonMixer,
        Mixer
    };

    inline static std::unordered_map<Role, std::string> mRoleMap = {
        {Role::None, "none"},
        {Role::NonMixer, "nonMixer"},
        {Role::Mixer, "mixer"}
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

    /*!@brief Necessary to shutdown the plugin when removed. Will signal the threads to stop.*/
    bool bRun {true};

    /* Playback control */
    enum playbackCommandEnum : uint32_t
    {
        kCommandPause   = 0,
        kCommandPlay    = 1,
        kCommandMove    = 2
    };

    void setARADocumentControllerRef(ARA::PlugIn::DocumentController* documentControllerRef);
    //TODO Implement these when audio settings are changed.
    void resetARAWithAudioSettings();
    //TODO Execute this when remotely a command comes in.
    void executePlaybackCommand(playbackCommandEnum command, uint32_t timeStamp = 0);


private:

    /*! @brief the ApiKey for the DAWN Audio Streaming API.*/
    std::string             mAPIKey{};

    /*! @brief A Web Socket Application to handle SDA (Streaming Discovery and Announcement) and other tasks.*/
    WebSocketApplication    mWSApp;
    bool                    webSocketStarted{false};


    struct {
        /*! @brief Audio Stream Parameters. Number of Samples per Block in the DAW.*/
        size_t                  mDAWBlockSize{0};
        DAWn::Events::Signal<>  sgnDAWBlockSizeChanged;
        /*! @brief Audio Stream Parameters. Sample Rate.*/
        int                     mSampleRate{0};
        /*! @brief Check if we are averaging the 2 channels.*/
        bool                    mMonoSplit{false};

    } mAudioSettings;

    struct
    {
        DAWn::Events::Signal<const std::string, uint32_t, uint32_t > outOfOrder;
        DAWn::Events::Signal<uint32_t> playbackPaused;
        DAWn::Events::Signal<uint32_t> playbackResumed;
        uint32_t                mLastTimeStamp{0};
        bool                    mLastTimeStampIsDirty{false};
        bool                    mPaused {false};
        bool isPaused() const
        {
            return mPaused;
        }
        void updatePlaybackLogic(int64_t now)
        {
            uint32_t ui32now = static_cast<uint32_t>(now);
            isPaused(ui32now);
            mLastTimeStamp = ui32now;
        }



    private:
        bool isPaused(uint32_t now) {

            auto wasPaused = mPaused;
            mPaused = now == mLastTimeStamp;

            if (wasPaused != mPaused && mPaused)
            {
                playbackPaused.Emit(now);
            }
            else if (wasPaused != mPaused && !mPaused)
            {
                playbackResumed.Emit(now);
            }
            return mPaused;
        }


    }playback;


    /*!
     * @brief Update information about buffer settings.
     * @param buffer The buffer to update.
     */
    void beforeProcessBlock(juce::AudioBuffer<float>& buffer);

    /*!
     * @brief Process Encoded Information.
     * @param uid_ts_encodedPayload
     */
    void extractDecodeAndMix(std::vector<std::byte> uid_ts_encodedPayload);

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
    std::thread mWebSocketSorcery;
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

    /*! @brief Set Mixers and BSAs to ZERO */
    void generalCacheReset(uint32_t timeStamp);

    /******** GUI ********/
    std::pair<float, float> rmsLevelsInputAudioBuffer {0.0f, 0.0f}; //first LEFT, second RIGHT
    std::pair<float, float> rmsLevelsJitterBuffer{0.0f, 0.0f};

    /****** PREVENT DOUBLE EXECUTION IN PREPARE TO PLAY *********************/
    std::once_flag mOnceFlag;

    /** PLAYBACK CONTROL *****/
    bool withARAactive{false};
    ARA::PlugIn::DocumentController* araDocumentController{nullptr};
    void receiveWSCommand(const char*);

    /*!
     * @brief Command Broadcast Thru Stream.
     *
     * @param command, 0=Pause, 1=Resume, 2=Move at time.
     * @param timeStamp, time in samples when command is 2.
     *
     */
    void broadcastCommand (uint32_t command, uint32_t timeStamp = 0);
    void commandPlay(){ broadcastCommand (1);}
    void commandPause(){ broadcastCommand (0);}
    void commandMoveAtTime(uint32_t timeStamp){ broadcastCommand (2, timeStamp);}


        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioStreamPluginProcessor)
};



#endif /* __PLUGIN_PROCESSOR__ */

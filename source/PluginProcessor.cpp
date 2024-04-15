#include <array>
#include <thread>
#include <cstddef>
#include <fstream>
#include <unistd.h>
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "Utilities/Configuration/Configuration.h"

//==============================================================================

AudioStreamPluginProcessor::AudioStreamPluginProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
      DAWn::Utilities::Configuration(".config/dawnaudio/init.dwn")
{
    std::transform(transport.role.begin(), transport.role.end(), transport.role.begin(), ::tolower);
    mUserID.SetRole(
            transport.role == "mixer" ? DAWn::Session::Role::Mixer :
            (transport.role == "peer" ? DAWn::Session::Role::NonMixer :
            (transport.role == "rogue" ? DAWn::Session::Role::Rogue : DAWn::Session::Role::None)));

    //Init the execution mode, options.wscommands = true, means we are using websockets for commands and authentication.
    mAPIKey = auth.key;


    std::cout << "Process ID : [" << getpid() << "] ";
    std::cout << "USER ID: " << mUserID << " ";
    if (debug.loopback) std::cout << "LOOPBACK: " << debug.loopback;



}

void AudioStreamPluginProcessor::prepareToPlay (double , int )
{
    std::call_once(mOnceFlag, [this](){
        mDAWPlaybackEvents = std::thread{[this](){
            std::chrono::milliseconds pollPeriod(eventDetection.pollPeriod);
            int64_t lastTimeStamp = 0;

            enum {
                STOPPED,
                PLAY
            } state;

            state = STOPPED;

            std::once_flag bOnce;
            while(bRun)
            {
                std::call_once(bOnce, [this](){
                   std::cout << "Running the event thread with a poll period of: " << eventDetection.pollPeriod << std::endl;
                });
                std::this_thread::sleep_for(pollPeriod);
                bool edge = state == STOPPED ^ playback.mLastTimeStamp == lastTimeStamp;
                if (edge)
                {

                    state = state == STOPPED ? PLAY : STOPPED;
                    if (state == STOPPED)
                    {
                        playback.SetPause(true);
                    }
                }
                lastTimeStamp = playback.mLastTimeStamp;
            };
        }};
        mOpusEncoderMapThreadManager = std::thread{[this]()
        {

            while (bRun)
            {
                if (!pRtp)
                {
                    continue;
                }

                for (auto& [userId, codec_bsa] : mOpusCodecMap)
                {
                    auto& [codec, bsa] = codec_bsa;
                    auto& bsaOutput = bsa[0];

                    while (bsaOutput.dataReady())
                    {
                        uint32_t timeStamp;
                        std::vector<float> interleavedAdaptedBlock(audio.bsize * 2, 0.0f);
                        bsaOutput.pop(interleavedAdaptedBlock, timeStamp);
                        auto [_r, _p, _pS] = codec.encodeChannel(interleavedAdaptedBlock.data(), 0);
                        auto& result = _r;
                        if (result != OpusImpl::Result::OK)
                        {
                            std::cout << "Encoding Error:" << _pS << std::endl;
                        }
                        auto &payload = _p;
                        pRtp->PushFrame(payload, mRtpStreamID, timeStamp);
                    }
                }
            }
            std::cout << "BYE ENCODER" << std::endl;
        }};

        mAudioMixerThreadManager = std::thread{[this](){



            while (bRun)
            {
                if (mAudioSettings.mDAWBlockSize <= 0) continue;
                if (mUserID.IsRoleSet() == false) continue;

                auto role = mUserID.GetRole();
                for (auto& [userId, codec_bsa] : mOpusCodecMap)
                {
                    //FETCH CODEC&BSA
                    auto& [codec, bsa] = codec_bsa;
                    auto& bsaInput = bsa[1];

                    //DATA
                    while (bsaInput.dataReady())
                    {
                        uint32_t timeStamp;
                        std::vector<Mixer::Block> blocks{};
                        std::vector<float> interleavedAdaptedBlock(2 * mAudioSettings.mDAWBlockSize, 0.0f);
                        bsaInput.pop(interleavedAdaptedBlock, timeStamp);
                        Utilities::Buffer::deinterleaveBlocks(blocks, interleavedAdaptedBlock);

                        int64_t realTimeStamp64;
                        int64_t timeStamp64 = static_cast<int64_t>(timeStamp);
                        if (role == DAWn::Session::Role::Rogue)
                        {
                            Mixer::AudioMixerBlock::mix(mAudioMixerBlocks, timeStamp, blocks, userId);
                        }
                        else if (role == DAWn::Session::Role::Mixer && debug.loopback == false)
                        {

                            Mixer::AudioMixerBlock::mix(mAudioMixerBlocks, timeStamp, blocks, userId);

                            auto mixedData = Mixer::AudioMixerBlock::getBlocks(mAudioMixerBlocks, timeStamp64, realTimeStamp64);
                            packEncodeAndPush(mixedData, static_cast<uint32_t> (timeStamp64));
                        }
                        else if (role == DAWn::Session::Role::NonMixer)
                        {
                            Mixer::AudioMixerBlock::replace(mAudioMixerBlocks, timeStamp, blocks, userId);
                        }
                    }
                }
            }
            std::cout << "BYE MIXER" << std::endl;
        }};

        // ARA Initialization
        //  Note: check if ARA supports changes in blocksize after this point
        double dSampleRate = mAudioSettings.mSampleRate;
        int iDAWBlockSize = static_cast<int>(mAudioSettings.mDAWBlockSize);

        withARAactive = prepareToPlayForARA(
            dSampleRate,
            iDAWBlockSize,
            getMainBusNumOutputChannels(),
            getProcessingPrecision()
            );
        std::cout << (withARAactive ? "ARA active" : "ARA not active") << std::endl;

        //INITALIZATION LIST
        //Object 0. WEBSOCKET.
        //Object 1. AUDIOMIXERS[2] => Two Audio Mixers. One for each channel. IM FORCING 2 CHANNELS. If other channels are involved Im ignoring them.
        //Object 2. RTPWRAP => RTPWrap object. This object is the one that handles the Network Interface.
        //Object 3. OpusCodecMap => Errors assoc with this map.

        if (options.wsenroll) mWSApp.OnYouAreHost.Connect(this, &AudioStreamPluginProcessor::commandSetHost);
        if (options.wsenroll) mWSApp.OnYouArePeer.Connect(this, &AudioStreamPluginProcessor::commandSetPeer);
        mWSApp.OnDisconnectCommand.Connect(this, &AudioStreamPluginProcessor::commandDisconnect);
        mWSApp.ThisPeerIsGone.Connect(this, &AudioStreamPluginProcessor::peerGone);
        mWSApp.ThisPeerIsConnected.Connect(this, &AudioStreamPluginProcessor::peerConnected);
        mWSApp.OnSendAudioSettings.Connect(this, &AudioStreamPluginProcessor::commandSendAudioSettings);
        mWSApp.AckFromBackend.Connect(this, &AudioStreamPluginProcessor::backendConnected);
        mWSApp.ApiKeyAuthFailed.Connect(std::function<void()>{[](){ std::cout << "API AUTH FAILED." << std::endl;}});
        mWSApp.ApiKeyAuthSuccess.Connect(std::function<void()>{[this](){
            webSocketStarted = true;
            std::cout << "API AUTH SUCCEEDED." << std::endl;
        }});
        mWSApp.OnCommand.Connect(std::function<void(const char*)>{[this](const char* msg)->void{
            std::string msgString{msg};
            this->commandStrings.push(msgString);
        }});
        //OBJECT 0. WEBSOCKET COMMANDS
        if ((options.wscommands || options.wsenroll) && mAPIKey.empty() == false)
        {
            std::cout << "Starting WebSocket Sorcery" << std::endl;
            mWebSocketSorcery = std::thread{[this](){
                mWSApp.Init(mAPIKey, auth.authEndpoint, auth.wsEndpoint);
            }};
            mWebSocketSorcery.detach();
        }

        //OBJECT 1. AUDIO MIXER
        mAudioMixerBlocks   = std::vector<Mixer::AudioMixerBlock>(audio.channels);

        Mixer::AudioMixerBlock::mixFinished.Connect(std::function<void(std::vector<Mixer::Block>, int64_t)>{
            [this](auto playbackHead, auto timeStamp64){
                auto& role = mUserID.GetRole();
                if (role != DAWn::Session::Role::Mixer) return;
                packEncodeAndPush (playbackHead, static_cast<uint32_t> (timeStamp64));
            }
        });

        //OBJECT 3. OPUS CODEC MAP, error handling.
        OpusImpl::CODEC::sEncoderErr.Connect(std::function<void(uint32_t, const char*, float*)>{
            [](auto uid, auto err, auto pdata){
                std::cout << "Encoder Error for UID[" << uid << "] :" << err << std::endl;
                std::cout << "@" << std::hex << pdata << std::dec << std::endl;
            }
        });
        OpusImpl::CODEC::sDecoderErr.Connect(std::function<void(uint32_t, const char*, std::byte*)>{
            [](auto uid, auto err, auto pdata){
                std::cout << "Decoder Error for UID[" << uid << "] :" << err << std::endl;
                std::cout << "@" << std::hex << pdata << std::dec << std::endl;
            }
        });

        playback.dawOriginatedPlaybackStop.Connect(std::function<void()>{
            [this](){
                std::cout << "Playback Paused" << std::endl;
                //Reset everything.
                broadcastCommand (0, 0);
                this->generalCacheReset(0);
            }
        });

        playback.dawOriginatedPlayback.Connect(std::function<void(int64_t)>{
            [this](auto timeStamp){
                std::cout << "Playback Resumed at: " << timeStamp << std::endl;
                broadcastCommand (1, static_cast<uint32_t>(timeStamp));
            }
        });

        if (mUserID.GetRole() != DAWn::Session::Role::None)
        {
            startRTP(transport.ip, transport.port);
            if (options.wscommands == false)
            {
                broadcastCommand(kCommandPing, 0);
            }
        }
    });
    std::cout << "Preparing to play " << std::endl;

}

bool& AudioStreamPluginProcessor::getMonoFlagReference()
{
    return mAudioSettings.mMonoSplit;
}

void AudioStreamPluginProcessor::tryApiKey(const std::string& secret)
{
    if (webSocketStarted)
    {
        std::cout << "WebSocket already started. To start again restart." << std::endl;
        return;
    }
    std::thread(
        [this, secret](){
            mWSApp.Init(secret, auth.authEndpoint, auth.wsEndpoint);
        }).detach();
}

std::tuple<uint32_t, int64_t> AudioStreamPluginProcessor::getUpdatedTimePosition()
{
    bool wasPaused = playback.IsPaused();
    auto [ui32nTimeMS, i64nSamplePosition] = Utilities::Time::getPosInMSAndSamples(getPlayHead());
    playback.mLastTimeStamp = playback.mNowTimeStamp;
    playback.mNowTimeStamp = i64nSamplePosition;
    auto timeStampDelta = playback.mLastTimeStamp < playback.mNowTimeStamp;
    if (wasPaused && timeStampDelta)
    {
        playback.SetPause(false);
    }
    return std::make_tuple(ui32nTimeMS, i64nSamplePosition);
}

void AudioStreamPluginProcessor::beforeProcessBlock(juce::AudioBuffer<float>& buffer, bool& shouldCancel)
{

    static uint8_t lastreason = 0;
    auto dawReportedBlockSize = buffer.getNumSamples();
    if (mAudioSettings.mDAWBlockSize != static_cast<size_t>(dawReportedBlockSize))
    {
        mAudioSettings.mDAWBlockSize = static_cast<size_t>(dawReportedBlockSize);
    }

    rmsLevelsInputAudioBuffer.first = buffer.getRMSLevel(0, 0, buffer.getNumSamples());
    rmsLevelsInputAudioBuffer.second = buffer.getRMSLevel(1, 0, buffer.getNumSamples());
    rmsLevelsInputAudioBuffer.first = juce::Decibels::gainToDecibels(rmsLevelsInputAudioBuffer.first);
    rmsLevelsInputAudioBuffer.second = juce::Decibels::gainToDecibels(rmsLevelsInputAudioBuffer.second);

    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    {
        buffer.clear (i, 0, dawReportedBlockSize);
    }

    //should Cancel?
    bool isSilent = std::max(rmsLevelsInputAudioBuffer.first, rmsLevelsInputAudioBuffer.second) < -60.0f && debug.overridermssilence == false;
    bool isRoleNotSet = mUserID.IsRoleSet() == false && debug.requiresrole == true;
    shouldCancel = (isSilent || isRoleNotSet);
    uint8_t reason = (isSilent ? 0x1 : 0) | (isRoleNotSet ? 0x2 : 0);

    if (shouldCancel && lastreason != reason)
    {
        lastreason = reason;
        std::cout << "Audio Thread Cancel Reason: " << std::endl;
        std::cout << "Silent: " << isSilent << " RoleNotSet: " << isRoleNotSet << std::endl;
    }

}

std::pair<OpusImpl::CODEC, std::vector<Utilities::Buffer::BlockSizeAdapter>>& AudioStreamPluginProcessor::getCodecPairForUser(Mixer::TUserID userID, uint32_t timeStamp)
{
    //  std::lock_guard<std::mutex> lock(mOpusCodecMapMutex);
    if (mOpusCodecMap.find(userID) == mOpusCodecMap.end())
    {
        std::cout << "Create FENCDEC and BSA for userID: " << userID << std::endl;
        auto nOfSizeAdaptersInOneDirection = (audio.channels >> 1) + (audio.channels % 2);
        mOpusCodecMap.insert(
            std::make_pair(
                userID,
                std::make_pair(
                    OpusImpl::CODEC(),
                    std::vector<Utilities::Buffer::BlockSizeAdapter>(
                        2 * nOfSizeAdaptersInOneDirection,
                        Utilities::Buffer::BlockSizeAdapter(audio.bsize, audio.channels)))));

        mOpusCodecMap[userID].first.cfg.ownerID = userID;

        jassert(mOpusCodecMap.find(userID) != mOpusCodecMap.end());
        jassert(mOpusCodecMap[userID].first.cfg.ownerID == userID);
        jassert(mOpusCodecMap[userID].first.mEncs.size() < 16);
        jassert(mOpusCodecMap[userID].first.mDecs.size() < 16);

        auto& [codec, bsa]  = mOpusCodecMap[userID];

        auto& bsaOut        = bsa[0];
        bsaOut.setTimeStamp(timeStamp, true);
        bsaOut.setChannelsAndOutputBlockSize(audio.channels, audio.bsize);

        auto& bsaIn         = bsa[1];
        bsaIn.setTimeStamp(timeStamp, true);
        bsaIn.setChannelsAndOutputBlockSize(audio.channels, mAudioSettings.mDAWBlockSize);

    }
    return mOpusCodecMap[userID];
}

void AudioStreamPluginProcessor::extractDecodeAndMix(std::vector<std::byte> uid_ts_encodedPayload)
{
    //
    //LAYOUT
    auto [result, userID, nSample, encodedPayLoad] = Utilities::Buffer::extractIncomingData(uid_ts_encodedPayload);

    if (result == false)
    {
        std::cout << "Error: Buffer Extraction" << std::endl;
    }

    if (0xdeadbee0 <= userID && userID <= 0xdeadbeef)
    {
        inboundCommandFromStream(userID, static_cast<uint32_t>(nSample));
    }

    if (mOpusCodecMap.find(userID) == mOpusCodecMap.end())
    {
        std::cout << "NEW USER IN THE STREAM" << std::endl;
    }

    //FETCH CODEC&BSA
    auto ui32nSample = static_cast<uint32_t>(nSample);
    auto& [codec, blockSzAdapters] = getCodecPairForUser(userID, ui32nSample);
    auto& bsaInput = blockSzAdapters[1]; //This is the input channel.

    //DATA DECODE
    auto [_r, _p, _pS]      = codec.decodeChannel(encodedPayLoad.data(), encodedPayLoad.size(), 0);
    auto& decodingResult    = _r;
    auto& decodedPayload    = _p;

    if (decodingResult != OpusImpl::Result::OK)
    {
        std::cout << "Decoding Error: " << _pS << std::endl;
        return;
    }

    //SEND TO MIXER THREAD
    bsaInput.push(decodedPayload, ui32nSample);

}

void AudioStreamPluginProcessor::packEncodeAndPush(std::vector<Mixer::Block>& blocks, uint32_t timeStamp)
{

    std::vector<Mixer::Block> __interleavedBlocks{};
    Utilities::Buffer::interleaveBlocks(__interleavedBlocks, blocks);
    auto& interleavedBlocks = __interleavedBlocks[0]; // This is the first pair of interleaved blocks. We are only using 2 channels.

    //FETCH CODEC&BSA
    auto& [codec, blockSzAdapters] = getCodecPairForUser(mUserID(), timeStamp);

    //SEND TO ENCODER THREAD
    blockSzAdapters[0].push(interleavedBlocks, timeStamp);

}

void AudioStreamPluginProcessor::broadcastCommand (uint32_t command, uint32_t timeStamp)
{
    if (options.wscommands && webSocketStarted)
    {
        auto ui8Command = static_cast<uint8_t>(command);
        auto j =  DAWn::Messages::PlaybackCommand(ui8Command, timeStamp);
        auto pManager = mWSApp.pSm.get();
        dynamic_cast<DAWn::WSManager *>(pManager)->Send(j);
    }
    else
    {
        //Stream the command thru the network
        auto pStrm = _rtpwrap::data::GetStream(mRtpStreamID);
        if (!pStrm || !pRtp)
        {
            std::cout << "WARNING: not connected to the stream router" << std::endl;
        }
        else
        {
            command += 0xdeadbee0;
            std::vector<std::byte> pData{};
            auto pts = reinterpret_cast<std::byte*>(&timeStamp);
            auto puid = reinterpret_cast<std::byte*>(&command);
            pData.insert(pData.begin(), pts, pts+4);
            pData.insert(pData.begin(), puid, puid+4);
            pStrm->qout_.push_back(std::make_pair(dynamic_cast<UDPRTPWrap*>(pRtp.get())->GetPeerID(), pData));
        }
    }
}


void AudioStreamPluginProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer&)
{
    // PRE PROCESS BLOCK
    bool shouldCancel = false;
    beforeProcessBlock(buffer, shouldCancel);
    if (shouldCancel)
    {
        return;
    }


    // GET TIME
    auto [nTimeMS, timeStamp64] = getUpdatedTimePosition();

    if (playback.IsPaused())
    {
        return;
    }

    // GRAB DATA FROM DAW
    std::vector<Mixer::Block> dawBufferData{};
    Utilities::Buffer::splitChannels(dawBufferData, buffer, mAudioSettings.mMonoSplit);

    // MIX AND SEND
    int64_t playbackTime64;
    auto& role = mUserID.GetRole();
    if (role == DAWn::Session::Role::Mixer)
    {
        // MIX DAW DATA into the mixer block.
        Mixer::AudioMixerBlock::mix(mAudioMixerBlocks, timeStamp64, dawBufferData, mUserID());

        // BROADCAST MIXED DATA
        auto mixedData = Mixer::AudioMixerBlock::getBlocks(mAudioMixerBlocks, timeStamp64, playbackTime64);
        packEncodeAndPush(mixedData, static_cast<uint32_t> (timeStamp64));
    }
    else if (role == DAWn::Session::Role::Rogue)
    {
        // BROADCAST DAW DATA
        packEncodeAndPush(dawBufferData, static_cast<uint32_t> (timeStamp64));

        //MIX DAW DATA
        Mixer::AudioMixerBlock::mix(mAudioMixerBlocks, timeStamp64, dawBufferData, mUserID());

    }
    else if (role == DAWn::Session::Role::NonMixer)
    {
        // BROADCAST DAW DATA
        packEncodeAndPush(dawBufferData, static_cast<uint32_t> (timeStamp64));
    }
    else
    {
        std::cout << "ERROR: Unknown Role" << std::endl;
        return;
    }

    // PLAYBACK AUDIO (origin daw buffer is modified with the contents from the mixer block)
    Utilities::Buffer::joinChannels(buffer, Mixer::AudioMixerBlock::getBlocksDelayed(mAudioMixerBlocks, timeStamp64, playbackTime64));

    // POST PROCESS BLOCK
    rmsLevelsJitterBuffer.first = buffer.getRMSLevel(0, 0, buffer.getNumSamples());
    rmsLevelsJitterBuffer.second = buffer.getRMSLevel(1, 0, buffer.getNumSamples());
    rmsLevelsJitterBuffer.first = juce::Decibels::gainToDecibels(rmsLevelsJitterBuffer.first);
    rmsLevelsJitterBuffer.second = juce::Decibels::gainToDecibels(rmsLevelsJitterBuffer.second);

    // ARA PROCESS BLOCK
    if (withARAactive) {
        processBlockForARA(buffer, isRealtime(), getPlayHead());
    }
}

void AudioStreamPluginProcessor::startRTP(std::string ip, int port)
{
    //OBJECT 2. RTWRAP
    if (!pRtp)
    {
        auto userId = mUserID();
        std::cout << "Start RTP stream: [" << ip << ":" << port << "]" << std::endl;
        pRtp = std::make_unique<UDPRTPWrap>();

        //TODO: TEMPORAL
        mRtpSessionID   = pRtp->CreateSession (ip);

        if (debug.loopback)
        {
            //dynamic cast of pRTP
            UDPRTPWrap* _udpRtp = dynamic_cast<UDPRTPWrap*> (pRtp.get());
            if (_udpRtp)
            {
                std::cout << "Creating  ** LOOPBACK ** RTP Stream for user " << userId << std::endl;
                mRtpStreamID = _udpRtp->CreateLoopBackStream(mRtpSessionID, port, static_cast<int>(userId^0x1));
            }
        }
        else
        {
            mRtpStreamID = pRtp->CreateStream(mRtpSessionID, port, static_cast<int>(userId)); //Horrible convention but if the userID is zero, CreateStream will make one of it's own.
        }


        auto pStream = _rtpwrap::data::GetStream (mRtpStreamID);
        //bind a codec to the stream
        pStream->letDataFromPeerIsReady.Connect (std::function<void (uint64_t, std::vector<std::byte>)> {
            [this] (auto, auto uid_ts_encodedPayload) {
                extractDecodeAndMix(uid_ts_encodedPayload);
            }
        });

        pStream->letOperationalError.Connect (std::function<void (uint64_t, std::string)> {
            [] (uint64_t peerId, std::string error) {
                std::cout << "Socket operational error: " << peerId << " " << error << std::endl;
            }
        });

        pStream->letThreadStarted.Connect (std::function<void (uint64_t)> {
            [] (uint64_t peerId) {
                std::cout << peerId << " Thread Started" << std::endl;
            }
        });

        pStream->run();
        if (!debug.loopback)
        {
            std::byte b{0};
            pRtp->PushFrame(std::vector<std::byte>{b}, mRtpStreamID, 0);
        }
    }
}

void AudioStreamPluginProcessor::commandSetHost(const char*)
{
    mUserID.SetRole(DAWn::Session::Role::Mixer);
    startRTP("44.205.23.6", 8899);
}

void AudioStreamPluginProcessor::commandSetPeer (const char*)
{
    mUserID.SetRole(DAWn::Session::Role::NonMixer);
    startRTP("44.205.23.6", 8899);
}

void AudioStreamPluginProcessor::commandDisconnect (const char*)
{
    std::cout << "FINISH THE PLUGIN AND THE DAW AND RESTART" << std::endl;
}

void AudioStreamPluginProcessor::peerGone (const char*)
{
    std::cout << "PEER GONE" << std::endl;
}

void AudioStreamPluginProcessor::peerConnected (const char*)
{
    std::cout << "PEER CONNECTED" << std::endl;
}

void AudioStreamPluginProcessor::commandSendAudioSettings (const char*)
{
    std::cout << "SENDING AUDIO SETTINGS:" << std::endl;
    auto j =  DAWn::Messages::AudioSettingsChanged(48000, 480, 32);
    auto pManager = mWSApp.pSm.get();
    auto settingsString = j.dump();
    dynamic_cast<DAWn::WSManager *>(pManager)->Send(j);
}

void AudioStreamPluginProcessor::backendConnected (const char*)
{
    std::cout << "ACK FROM BACKEND" << std::endl;
}

void AudioStreamPluginProcessor::generalCacheReset(uint32_t timeStamp)
{
    {
        //Set Input BSAs
        for (auto& [userId, codec_bsa] : mOpusCodecMap)
        {
            auto& [codec, bsa] = codec_bsa;
            auto& bsaInput = bsa[1];
            bsaInput.setChannelsAndOutputBlockSize(2, mAudioSettings.mDAWBlockSize);
            bsaInput.setTimeStamp(timeStamp); //This Resets the BSA.
        }
    }
    //Reset the Audio Mixer Blocks
    Mixer::AudioMixerBlock::resetMixers(mAudioMixerBlocks, mAudioSettings.mDAWBlockSize, options.delayseconds);
}

void AudioStreamPluginProcessor::inboundCommandFromStream (uint32_t command, uint32_t timeStamp)
{
    command -= 0xdeadbee0;
    uint8_t ui8Command = command & 0xff;
    std::cout << "COMMAND STREAM" << std::endl;
    auto j = DAWn::Messages::PlaybackCommand(ui8Command, timeStamp);
    auto js = j.dump();
    commandStrings.push(js);
    //receiveWSCommand(js.c_str());
}

void AudioStreamPluginProcessor::receiveWSCommand(const char* payload)
{
    if (!payload)
    {
        std::cout << "Command Null" << std::endl;
        return;
    }
    std::cout << "The command arrived: " << payload << std::endl;
    if (payload != nullptr)
    {
        std::cout << "Inbound json: ";
        auto ptrStr = const_cast<char*>(payload);
        while (*ptrStr)
        {
            std::cout << *ptrStr++;
        }
        std::cout << std::endl;
    }
    if (!withARAactive || araDocumentController == nullptr || payload == nullptr)
    {
        // No ARA available
        std::cout << "CRITICAL ARA FAILED" << std::endl;
        std::cout << "payload @" << std::hex << payload << std::dec << std::endl;
        std::cout << "araDocumentController @" << std::hex << araDocumentController << std::dec << std::endl;
        std::cout << "withARAactive: " << (withARAactive ? "true":"false")  << std::endl;
        return;
    }
    // deserialize
    nlohmann::json j;
    try {
        j = nlohmann::json::parse(payload);

        /* Sometimes commands may come in like this:
         *  {"action":"pluginMessage","data":{"Payload":{"Command":3,"TimePosition":0},"TimeStamp":"1712270952230","Type":13}}
         *
         * 1. This happens when the originator sends the command using the stream (not recommended).
         * 2. The stream doesn't filter the Payload.
         * 3. We filter that here.
         *  */
        if (j.find("data") != j.end())
        {
            j = j["data"]["Payload"];
        }


    } catch (nlohmann::json::parse_error& e) {
        std::cout << "Could not parse payload for WS command (" << e.what() << ")." << std::endl;
        return;
    }

    auto command = j["Command"].get<uint32_t>();
    ARA::ARAInt64 i64TimePosition = j["TimePosition"].get<ARA::ARAInt64>();
    ARA::ARATimePosition dAraPosition = ARA::timeAtSamplePosition(i64TimePosition, 48000);

    std::cout << "Command : " << command << std::endl;
    std::cout << "TimePosition : " << i64TimePosition << " (" << dAraPosition << ")" << std::endl;


    // get HostPlaybackController reference to interact with DAW
    ARA::PlugIn::HostPlaybackController* playbackController = araDocumentController->getHostPlaybackController();

    if (playbackController == nullptr)
    {
        std::cout << "No playback controller available." << std::endl;
        return;
    }

    // apply
    switch(command)
    {

        case kCommandPlay:
            // play command
            // do stop, then setPosition
            std::cout << "Playback from WS command: Play >> " << std::endl;
            playbackController->requestStopPlayback();
            std::cout << "Playback from WS command: set position to " << dAraPosition << " seconds" << std::endl;
            playbackController->requestSetPlaybackPosition(dAraPosition);
            // then do play only
            std::cout << "Playback from WS command: start" << std::endl;
            playbackController->requestStartPlayback();
            break;

        case kCommandStop:
            // stop command
            std::cout << "Playback from WS command: stop" << std::endl;
            playbackController->requestStopPlayback();
            //then setPosition
            std::cout << "Playback from WS command: set position to " << dAraPosition << " seconds" << std::endl;
            playbackController->requestSetPlaybackPosition(dAraPosition);
            break;

        case kCommandMove:
            std::cout << "Playback from WS command: set position to " << dAraPosition << " seconds" << std::endl;
            playbackController->requestSetPlaybackPosition(dAraPosition);
            break;

        case kCommandPing:
            std::cout << "Command Ping from stream router" << std::endl;
            break;
        default:
            std::cout << "Unknown command code (" << command << ") for WS command." << std::endl;
    }
}

//==============================================================================
bool AudioStreamPluginProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioStreamPluginProcessor::createEditor()
{
    return new AudioStreamPluginEditor (*this);
}

//==============================================================================
void AudioStreamPluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ignoreUnused (destData);
}

void AudioStreamPluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ignoreUnused (data, sizeInBytes);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioStreamPluginProcessor();
}

//==============================================================================
const juce::String AudioStreamPluginProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioStreamPluginProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool AudioStreamPluginProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool AudioStreamPluginProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double AudioStreamPluginProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioStreamPluginProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
        // so this should be at least 1, even if you're not really implementing programs.
}

int AudioStreamPluginProcessor::getCurrentProgram()
{
    return 0;
}

void AudioStreamPluginProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String AudioStreamPluginProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void AudioStreamPluginProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================


AudioStreamPluginProcessor::~AudioStreamPluginProcessor()
{
    std::cout << "Size of mOpusCodecMap : " << mOpusCodecMap.size() << std::endl;

    if (options.wscommands == false) broadcastCommand(kCommandRemove);

    bRun = false;
    mOpusCodecMap.clear();
    if (bRun == false)
    {
        if (mOpusEncoderMapThreadManager.joinable())
        {
            mOpusEncoderMapThreadManager.join();
        }
        if (mAudioMixerThreadManager.joinable())
        {
            mAudioMixerThreadManager.join();
        }
        if (mDAWPlaybackEvents.joinable())
        {
            mDAWPlaybackEvents.join();
        }
    }
    //Tear down UDP
    auto pStream = _rtpwrap::data::GetStream (mRtpStreamID);
    if (pStream)
    {
        pStream->closeAndJoin();
    }

}

void AudioStreamPluginProcessor::releaseResources()
{

    std::cout << "RELEASING RESOURCES BTW" << std::endl;
    releaseResourcesForARA();

}

bool AudioStreamPluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

        // This checks if the input layout matches the output layout
    #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    #endif

    return true;
#endif
}

void AudioStreamPluginProcessor::setARADocumentControllerRef(ARA::PlugIn::DocumentController* documentControllerRef)
{
    araDocumentController = documentControllerRef;
}

